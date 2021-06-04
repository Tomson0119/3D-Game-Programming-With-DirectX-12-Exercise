#include "../MyCommon/stdafx.h"
#include "gameObject.h"


GameObject::GameObject(int offset, Mesh* mesh)
{
	mCBIndex = (UINT)offset;
	mMesh = mesh;
	UpdateBoudingBox();
}

GameObject::~GameObject()
{
	if (mBBObject) delete mBBObject;
}

void GameObject::Update(ConstantBuffer<ObjectConstants>* objectCB)
{
	objectCB->CopyData(mCBIndex, GetObjectConstants());

	mLook = Vector3::Normalize(mLook);
	mUp = Vector3::Normalize(Vector3::Cross(mLook, mRight));
	mRight = Vector3::Cross(mUp, mLook);

	UpdateTransform();
	UpdateBoudingBox();
}

void GameObject::Draw(ID3D12GraphicsCommandList* cmdList)
{
	if (mMesh) mMesh->Draw(cmdList);
}

void GameObject::UpdateTransform()
{
	mWorld._11 = mRight.x;	  mWorld._12 = mRight.y;	mWorld._13 = mRight.z;
	mWorld._21 = mUp.x;		  mWorld._22 = mUp.y;		mWorld._23 = mUp.z;
	mWorld._31 = mLook.x;	  mWorld._32 = mLook.y;		mWorld._33 = mLook.z;
	mWorld._41 = mPosition.x, mWorld._42 = mPosition.y, mWorld._43 = mPosition.z;

	if (mBBObject) mBBObject->UpdateCoordinate(mWorld);
}

void GameObject::UpdateBoudingBox()
{
	if (mMesh)
	{
		mMesh->mOOBB.Transform(mOOBB, XMLoadFloat4x4(&mWorld));
		XMStoreFloat4(&mOOBB.Orientation, XMQuaternionNormalize(XMLoadFloat4(&mOOBB.Orientation)));
	}
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

void GameObject::Move(float dx, float dy, float dz)
{
	mPosition.x += dx;
	mPosition.y += dy;
	mPosition.z += dz;

	if (mBBObject) mBBObject->Move(dx, dy, dz);
}

void GameObject::Move(XMFLOAT3& dir, float dist)
{
	XMFLOAT3 shift = { 0.0f,0.0f,0.0f };
	shift = Vector3::Add(shift, dir, dist);
	Move(shift.x, shift.y, shift.z);
}

void GameObject::MoveStrafe(float dist, bool local)
{
	XMFLOAT3 right = (local) ? mRight : XMFLOAT3(1.0f, 0.0f, 0.0f);
	Move(right, dist);
}

void GameObject::MoveUp(float dist, bool local)
{
	XMFLOAT3 up = (local) ? mUp : XMFLOAT3(0.0f, 1.0f, 0.0f);
	Move(up, dist);
}

void GameObject::MoveForward(float dist, bool local)
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

void GameObject::Scale(float xScale, float yScale, float zScale)
{

}

void GameObject::Scale(float scale)
{

}

void GameObject::EnableBoundBoxRender(UINT offset, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	mBBObject = new BoundBoxObject(device, cmdList, offset, mOOBB);
	mBBObject->SetPosition(mOOBB.Center);
	mBBObject->SetMaterial(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), {}, 0.0f);
	mBBObject->Move(mPosition.x, mPosition.y, mPosition.z);
}

ObjectConstants GameObject::GetObjectConstants()
{
	ObjectConstants objCnst = {};
	objCnst.World = Matrix4x4::Transpose(mWorld);
	objCnst.Mat = mMaterial;
	return objCnst;
}

BoundBoxObject* GameObject::GetBoundBoxObject() const
{
	if (mBBObject) return mBBObject;
	return nullptr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
BoundBoxObject::BoundBoxObject(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	UINT offset, const BoundingOrientedBox& bb)
	: GameObject(offset, nullptr)
{
	float width = 2.0f * bb.Extents.x;
	float height = 2.0f * bb.Extents.y;
	float depth = 2.0f * bb.Extents.z;

	mMesh = new BoxMesh(device, cmdList, width, height, depth);
}

BoundBoxObject::~BoundBoxObject()
{
	if (mMesh) delete mMesh;
}

void BoundBoxObject::UpdateCoordinate(const XMFLOAT4X4& world)
{
	mWorld._11 = world._11; mWorld._12 = world._12; mWorld._13 = world._13;
	mWorld._21 = world._21; mWorld._22 = world._22; mWorld._23 = world._23;
	mWorld._31 = world._31; mWorld._32 = world._32; mWorld._33 = world._33;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
ColorObject::ColorObject(int offset, Mesh* mesh)
	: GameObject(offset, mesh)
{	
}

ColorObject::~ColorObject()
{
}




