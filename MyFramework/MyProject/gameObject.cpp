#include "../MyCommon/stdafx.h"
#include "gameObject.h"

GameObject::GameObject(int offset, Mesh* mesh)
{
	mCBIndex = (UINT)offset;
	mMesh = mesh;
}

GameObject::~GameObject()
{
}

void GameObject::Update()
{
	
}

void GameObject::Draw(ID3D12GraphicsCommandList* cmdList)
{
	if (mMesh) {
		mMesh->Draw(cmdList);
	}
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

void GameObject::Move(float dx, float dy, float dz)
{
	if (dx != 0.0f) MoveStrafe(dx);
	if (dy != 0.0f) MoveUp(dy);
	if (dz != 0.0f) MoveForward(dz);
}

void GameObject::MoveStrafe(float dist)
{
	XMFLOAT3 position = GetPosition();
	XMFLOAT3 right = GetRight();
	position = Vector3::Add(position, right, dist);
	SetPosition(position);
}

void GameObject::MoveUp(float dist)
{
	XMFLOAT3 position = GetPosition();
	XMFLOAT3 up = GetUp();
	position = Vector3::Add(position, up, dist);
	SetPosition(position);
}

void GameObject::MoveForward(float dist)
{
	XMFLOAT3 position = GetPosition();
	XMFLOAT3 look = GetLook();
	position = Vector3::Add(position, look, dist);
	SetPosition(position);
}

void GameObject::Rotate(float pitch, float yaw, float roll)
{
	XMMATRIX rotate = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(pitch), XMConvertToRadians(yaw), XMConvertToRadians(roll));
	mWorld = Matrix4x4::Multiply(rotate, mWorld);
}

void GameObject::Rotate(const XMFLOAT3* axis, float angle)
{
	XMMATRIX rotate = XMMatrixRotationAxis(XMLoadFloat3(axis), XMConvertToRadians(angle));
	mWorld = Matrix4x4::Multiply(rotate, mWorld);
}

void GameObject::Scale(float xScale, float yScale, float zScale)
{
}

void GameObject::Scale(float scale)
{
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