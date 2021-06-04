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
	UpdateBoudingBox();
}

void GameObject::Draw(ID3D12GraphicsCommandList* cmdList)
{
	if (mMesh) mMesh->Draw(cmdList);
}

void GameObject::UpdateBoudingBox()
{
	if (mMesh)
	{
		mMesh->mOOBB.Transform(mOOBB, XMLoadFloat4x4(&mWorld));
		XMStoreFloat4(&mOOBB.Orientation, XMQuaternionNormalize(XMLoadFloat4(&mOOBB.Orientation)));
	}
	if (mBBObject) mBBObject->UpdateBoudingBox();
}

void GameObject::SetPosition(float x, float y, float z)
{
	mWorld._41 = x;
	mWorld._42 = y;
	mWorld._43 = z;
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
	if (dx != 0.0f) MoveStrafe(dx);
	if (dy != 0.0f) MoveUp(dy);
	if (dz != 0.0f) MoveForward(dz);
	
	if (mBBObject) mBBObject->Move(dx, dy, dz);
}

void GameObject::MoveStrafe(float dist)
{
	XMFLOAT3 position = GetPosition();
	XMFLOAT3 right = GetRight();
	position = Vector3::Add(position, right, dist);
	SetPosition(position);

	if (mBBObject) mBBObject->MoveStrafe(dist);
}

void GameObject::MoveUp(float dist)
{
	XMFLOAT3 position = GetPosition();
	XMFLOAT3 up = GetUp();
	position = Vector3::Add(position, up, dist);
	SetPosition(position);

	if (mBBObject) mBBObject->MoveUp(dist);
}

void GameObject::MoveForward(float dist)
{
	XMFLOAT3 position = GetPosition();
	XMFLOAT3 look = GetLook();
	position = Vector3::Add(position, look, dist);
	SetPosition(position);

	if (mBBObject) mBBObject->MoveForward(dist);
}

void GameObject::Rotate(float pitch, float yaw, float roll)
{
	XMMATRIX rotate = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(pitch), XMConvertToRadians(yaw), XMConvertToRadians(roll));
	mWorld = Matrix4x4::Multiply(rotate, mWorld);

	if (mBBObject) mBBObject->Rotate(pitch, yaw, roll);
}

void GameObject::Rotate(const XMFLOAT3* axis, float angle)
{
	XMMATRIX rotate = XMMatrixRotationAxis(XMLoadFloat3(axis), XMConvertToRadians(angle));
	mWorld = Matrix4x4::Multiply(rotate, mWorld);

	if (mBBObject) mBBObject->Rotate(axis, angle);
}

void GameObject::Scale(float xScale, float yScale, float zScale)
{

	if (mBBObject) mBBObject->Scale(xScale, yScale, zScale);
}

void GameObject::Scale(float scale)
{

	if (mBBObject) mBBObject->Scale(scale);
}

void GameObject::EnableBoundBoxRender(UINT offset, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	mBBObject = new BoundBoxObject(device, cmdList, offset, mOOBB);
	mBBObject->SetPosition(mOOBB.Center);
	mBBObject->SetMaterial(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), {}, 0.0f);
	
	mBBObject->Move(mWorld._41, mWorld._42, mWorld._43);  // 본 물체의 위치로 옮긴다.
}

XMFLOAT3 GameObject::GetPosition()
{
	return XMFLOAT3(mWorld._41, mWorld._42, mWorld._43);
}

XMFLOAT3 GameObject::GetLook()
{
	return Vector3::Normalize(XMFLOAT3(mWorld._31, mWorld._32, mWorld._33));
}

XMFLOAT3 GameObject::GetUp()
{
	return Vector3::Normalize(XMFLOAT3(mWorld._21, mWorld._22, mWorld._23));
}

XMFLOAT3 GameObject::GetRight()
{
	return Vector3::Normalize(XMFLOAT3(mWorld._11, mWorld._12, mWorld._13));
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

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
ColorObject::ColorObject(int offset, Mesh* mesh)
	: GameObject(offset, mesh)
{	
}

ColorObject::~ColorObject()
{
}




