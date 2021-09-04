#include "common.h"
#include "gameObject.h"
#include "mesh.h"


GameObject::GameObject(int offset)
	: mCBIndex((UINT)offset)
{
}

GameObject::GameObject(int offset, Mesh* mesh)
	: GameObject(offset)
{
	if (mesh) SetMesh(mesh);
}

GameObject::~GameObject()
{
}

void GameObject::UpdateConstants(ConstantBuffer<ObjectConstants>* objectCB)
{
	objectCB->CopyData(mCBIndex, GetObjectConstants());	
}

void GameObject::Update(float elapsedTime, XMFLOAT4X4* parent)
{
	mLook = Vector3::Normalize(mLook);
	mUp = Vector3::Normalize(Vector3::Cross(mLook, mRight));
	mRight = Vector3::Cross(mUp, mLook);
	
	UpdateTransform(parent);
	UpdateBoudingBox();

	if (mChild) mChild->Update(elapsedTime, &mWorld);
	if (mSibling) mSibling->Update(elapsedTime, parent);
}

void GameObject::Draw(ID3D12GraphicsCommandList* cmdList)
{
	if(mMesh)
		mMesh->Draw(cmdList);
}

void GameObject::UpdateTransform(XMFLOAT4X4* parent)
{
	mWorld(0, 0) = mScaling.x * mRight.x; 
	mWorld(0, 1) = mRight.y;	
	mWorld(0, 2) = mRight.z;

	mWorld(1, 0) = mUp.x;		
	mWorld(1, 1) = mScaling.y * mUp.y;		
	mWorld(1, 2) = mUp.z;

	mWorld(2, 0) = mLook.x;		
	mWorld(2, 1) = mLook.y;		
	mWorld(2, 2) = mScaling.z * mLook.z;

	mWorld(3, 0) = mPosition.x;
	mWorld(3, 1) = mPosition.y;
	mWorld(3, 2) = mPosition.z;

	mWorld = (parent) ? Matrix4x4::Multiply(mWorld, *parent) : mWorld;
}

void GameObject::UpdateBoudingBox()
{
	if (mMesh)
	{
		mMesh->mOOBB.Transform(mOOBB, XMLoadFloat4x4(&mWorld));
		XMStoreFloat4(&mOOBB.Orientation, XMQuaternionNormalize(XMLoadFloat4(&mOOBB.Orientation)));
	}
}

void GameObject::SetChild(GameObject* child)
{
	if (child)
		child->mParent = this;
	if (mChild)
	{
		if (child) child->mSibling = mChild->mSibling;
		mChild->mSibling = child;
	}
	else
		mChild = child;
}

void GameObject::SetPosition(float x, float y, float z)
{
	mPosition = { x,y,z };
}

void GameObject::SetPosition(XMFLOAT3 pos)
{
	SetPosition(pos.x, pos.y, pos.z);
}

void GameObject::SetMaterial(XMFLOAT4 color, XMFLOAT3 frenel, float roughness)
{
	mMaterial.Color = color;
	mMaterial.Frenel = frenel;
	mMaterial.Roughness = roughness;
}

void GameObject::SetLook(XMFLOAT3& look)
{
	mLook = look;
	GameObject::Update(1.0f, nullptr);
}

void GameObject::Move(float dx, float dy, float dz)
{
	mPosition.x += dx;
	mPosition.y += dy;
	mPosition.z += dz;
}

void GameObject::Move(XMFLOAT3& dir, float dist)
{
	XMFLOAT3 shift = { 0.0f,0.0f,0.0f };
	shift = Vector3::Add(shift, dir, dist);
	Move(shift.x, shift.y, shift.z);
}

void GameObject::Strafe(float dist, bool local)
{
	XMFLOAT3 right = (local) ? mRight : XMFLOAT3(1.0f, 0.0f, 0.0f);
	Move(right, dist);
}

void GameObject::Upward(float dist, bool local)
{
	XMFLOAT3 up = (local) ? mUp : XMFLOAT3(0.0f, 1.0f, 0.0f);
	Move(up, dist);
}

void GameObject::Walk(float dist, bool local)
{
	XMFLOAT3 look = (local) ? mLook : XMFLOAT3(0.0f, 0.0f, 1.0f);
	Move(look, dist);
}

void GameObject::Rotate(float pitch, float yaw, float roll)
{
	if (pitch != 0.0f)
	{
		XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mRight), XMConvertToRadians(pitch));
		mUp = Vector3::TransformNormal(mUp, R);
		mLook = Vector3::TransformNormal(mLook, R);
	}
	if (yaw != 0.0f)
	{
		XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mUp), XMConvertToRadians(yaw));
		mLook = Vector3::TransformNormal(mLook, R);
		mRight = Vector3::TransformNormal(mRight, R);
	}
	if (roll != 0.0f)
	{
		XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mLook), XMConvertToRadians(roll));
		mUp = Vector3::TransformNormal(mUp, R);
		mRight = Vector3::TransformNormal(mRight, R);
	}
}

void GameObject::Rotate(const XMFLOAT3& axis, float angle)
{
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&axis), XMConvertToRadians(angle));
	mRight = Vector3::TransformNormal(mRight, R);
	mUp = Vector3::TransformNormal(mUp, R);
	mLook = Vector3::TransformNormal(mLook, R);
}

void GameObject::RotateY(float angle)
{
	XMMATRIX R = XMMatrixRotationY(XMConvertToRadians(angle));
	mRight = Vector3::TransformNormal(mRight, R);
	mUp = Vector3::TransformNormal(mUp, R);
	mLook = Vector3::TransformNormal(mLook, R);
}

void GameObject::Pitch(float angle)
{
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mRight), XMConvertToRadians(angle));
	mUp = Vector3::TransformNormal(mUp, R);
	mLook = Vector3::TransformNormal(mLook, R);
}

void GameObject::Scale(float xScale, float yScale, float zScale)
{
	mScaling = { xScale, yScale, zScale };
}

void GameObject::Scale(const XMFLOAT3& scale)
{
	Scale(scale.x, scale.y, scale.z);
}

void GameObject::Scale(float scale)
{
	Scale(scale, scale, scale);
}

ObjectConstants GameObject::GetObjectConstants()
{
	ObjectConstants objCnst = {};
	objCnst.World = Matrix4x4::Transpose(mWorld);
	objCnst.Mat = mMaterial;
	return objCnst;
}