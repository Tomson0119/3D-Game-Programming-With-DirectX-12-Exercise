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

void GameObject::UpdateConstants(ConstantBuffer<ObjectConstants>* objectCB)
{
	objectCB->CopyData(mCBIndex, GetObjectConstants());	
}

void GameObject::Update(float elapsedTime)
{
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
	mWorld(0, 0) = mRight.x;	mWorld(0, 1) = mRight.y;	mWorld(0, 2) = mRight.z;
	mWorld(1, 0) = mUp.x;		mWorld(1, 1) = mUp.y;		mWorld(1, 2) = mUp.z;
	mWorld(2, 0) = mLook.x;		mWorld(2, 1) = mLook.y;		mWorld(2, 2) = mLook.z;
	mWorld(3, 0) = mPosition.x, mWorld(3, 1) = mPosition.y, mWorld(3, 2) = mPosition.z;

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
	if (mBBObject)
	{
		float dx = x - mPosition.x;
		float dy = y - mPosition.y;
		float dz = z - mPosition.z;

		mBBObject->Move(dx, dy, dz);
	}
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

void BoundBoxObject::Update(float elapsedTime)
{
	GameObject::UpdateTransform();
}

void BoundBoxObject::UpdateCoordinate(const XMFLOAT4X4& world)
{
	mRight = { world(0, 0), world(0, 1), world(0, 2) };
	mUp    = { world(1, 0), world(1, 1), world(1, 2) };
	mLook  = { world(2, 0), world(2, 1), world(2, 2) };
}


///////////////////////////////////////////////////////////////////////////////
//
NonePlayerObject::NonePlayerObject(int offset, Mesh* mesh)
	: GameObject(offset, mesh)
{
}

NonePlayerObject::~NonePlayerObject()
{
}

void NonePlayerObject::SetInitialSpeed(float speed)
{
	mInitialSpeed = speed;
	mCurrSpeed = speed;
}

void NonePlayerObject::Update(float elapsedTime)
{
	if (mActive)
	{
		if (mCurrSpeed) Move(mMovingDirection, mCurrSpeed * elapsedTime);
	}
	mCurrSpeed += mAcceleration;
	GameObject::Update(elapsedTime);
}
