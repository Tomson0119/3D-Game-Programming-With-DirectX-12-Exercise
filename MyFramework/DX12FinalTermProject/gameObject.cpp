#include "stdafx.h"
#include "gameObject.h"


GameObject::GameObject(int offset)
	: mCBIndex((UINT)offset)
{
}

GameObject::GameObject(int offset, Mesh* mesh)
	: GameObject(offset)
{
	if(mesh) mMeshes.emplace_back(mesh);
	UpdateBoudingBox();
}

GameObject::~GameObject()
{
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
	for (const auto& mesh : mMeshes)
		mesh->Draw(cmdList);
}

void GameObject::UpdateTransform()
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
}

void GameObject::UpdateBoudingBox()
{
	for(const auto& mesh : mMeshes) {
		mesh->mOOBB.Transform(mOOBB, XMLoadFloat4x4(&mWorld));
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

void GameObject::RotateY(float angle, bool local)
{
	XMMATRIX R = XMMatrixRotationY(XMConvertToRadians(angle));
	mRight = Vector3::TransformNormal(mRight, R);
	mUp = Vector3::TransformNormal(mUp, R);
	mLook = Vector3::TransformNormal(mLook, R);
}

void GameObject::Pitch(float angle, bool local)
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


///////////////////////////////////////////////////////////////////////////////
//
TerrainObject::TerrainObject(int offset)
	: GameObject(offset, nullptr)
{

}

TerrainObject::~TerrainObject()
{
	for (const auto& mesh : mMeshes)
		delete mesh;
	mMeshes.clear();
}

void TerrainObject::BuildTerrainMeshes(
	ID3D12Device* device, 
	ID3D12GraphicsCommandList* cmdList, 
	int width, int depth, int blockWidth, int blockDepth,
	const XMFLOAT3& scale, 
	XMFLOAT4& color, 
	const std::wstring& path)
{
	mWidth = width;
	mDepth = depth;
	mScale = scale;

	int xQuadPerBlock = blockWidth - 1;
	int zQuadPerBlock = blockDepth - 1;

	mHeightMapImage = std::make_unique<HeightMapImage>(path, mWidth, mDepth, mScale);

	long xBlocks = (mWidth - 1) / xQuadPerBlock;
	long zBlocks = (mDepth - 1) / zQuadPerBlock;

	for (const auto& mesh : mMeshes)
		delete mesh;
	mMeshes.clear();
	
	HeightMapGridMesh* gridMesh = nullptr;
	for (int z = 0, zStart = 0; z < zBlocks; ++z)
	{
		for (int x = 0, xStart = 0; x < xBlocks; ++x)
		{
			xStart = x * (blockWidth - 1);
			zStart = z * (blockDepth - 1);

			gridMesh = new HeightMapGridMesh(device, cmdList, xStart, zStart,
				blockWidth, blockDepth, scale, color, mHeightMapImage.get());
			SetMesh(gridMesh);
		}
	}
}

float TerrainObject::GetHeight(float x, float z) const
{
	assert(mHeightMapImage && "HeightMapImage doesn't exist");
	return mHeightMapImage->GetHeight(x / mScale.x, z / mScale.z) * mScale.y;
}

XMFLOAT3 TerrainObject::GetNormal(float x, float z) const
{
	assert(mHeightMapImage && "HeightMapImage doesn't exist");
	return mHeightMapImage->GetNormal((int)(x / mScale.x), (int)(z / mScale.z));
}
