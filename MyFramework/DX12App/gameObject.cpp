#include "stdafx.h"
#include "gameObject.h"
#include "gameScene.h"


GameObject::GameObject()
	: mMaterial{XMFLOAT4(1.0f,1.0f,1.0f,1.0f), XMFLOAT3(0.01f,0.01f,0.01f), 0.25f}
{
}

GameObject::~GameObject()
{
}

void GameObject::Update(float elapsedTime, XMFLOAT4X4* parent)
{
	mLook = Vector3::Normalize(mLook);
	mUp = Vector3::Normalize(Vector3::Cross(mLook, mRight));
	mRight = Vector3::Cross(mUp, mLook);

	Animate(elapsedTime);

	UpdateTransform(parent);
	UpdateBoudingBox();

	if (mChild) mChild->Update(elapsedTime, &mWorld);
	if (mSibling) mSibling->Update(elapsedTime, parent);
}

void GameObject::ExecuteSO(ID3D12GraphicsCommandList* cmdList)
{
	for (const auto& mesh : mMeshes) {
		mesh->PrepareBufferViews(cmdList, true);
		mesh->Draw(cmdList, true);
	}
}

void GameObject::Draw(ID3D12GraphicsCommandList* cmdList)
{
	for (const auto& mesh : mMeshes) {
		mesh->PrepareBufferViews(cmdList, false);
		mesh->Draw(cmdList);
	}
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
	for(const auto& mesh :mMeshes)
	{
		mesh->mOOBB.Transform(mOOBB, XMLoadFloat4x4(&mWorld));
		XMStoreFloat4(&mOOBB.Orientation, XMQuaternionNormalize(XMLoadFloat4(&mOOBB.Orientation)));
	}
}

void GameObject::Animate(float elapsedTime)
{
	if (!Vector3::Equal(mMoveDirection, Vector3::Zero()))
		Move(mMoveDirection, mMoveSpeed * elapsedTime);

	if (!Vector3::Equal(mRotationAxis, Vector3::Zero()))
		Rotate(mRotationAxis, mRotationSpeed * elapsedTime);
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

void GameObject::SetPosition(const XMFLOAT3& pos)
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

void GameObject::SetRotation(XMFLOAT3& axis, float speed)
{
	mRotationAxis = axis;
	mRotationSpeed = speed;
}

void GameObject::SetMovement(XMFLOAT3& dir, float speed)
{
	mMoveDirection = dir;
	mMoveSpeed = speed;
}

void GameObject::SetReflected(XMFLOAT4& plane)
{
	mReflected = true;
	mReflectMatrix = Matrix4x4::Reflect(plane);
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
	if (mReflected)
		objCnst.World = Matrix4x4::Transpose(Matrix4x4::Multiply(mWorld, mReflectMatrix));
	else
		objCnst.World = Matrix4x4::Transpose(mWorld);

	objCnst.Mat = mMaterial;
	return objCnst;
}


///////////////////////////////////////////////////////////////////////////////
//
TerrainObject::TerrainObject(int width, int depth, const XMFLOAT3& scale)
	: GameObject(), mWidth(width), mDepth(depth), mTerrainScale(scale)
{
	mMaterial = { XMFLOAT4(0.7f,0.7f,0.7f,1.0f),XMFLOAT3(0.1f,0.1f,0.1f),0.25f };
}

TerrainObject::~TerrainObject()
{
}

void TerrainObject::BuildHeightMap(const std::wstring& path)
{
	mHeightMapImage = std::make_unique<HeightMapImage>(path, mWidth, mDepth, mTerrainScale);
}

void TerrainObject::BuildTerrainMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, int blockWidth, int blockDepth)
{	
	int xBlocks = (mWidth - 1) / (blockWidth - 1);
	int zBlocks = (mDepth - 1) / (blockDepth - 1);

	for (int z = 0, zStart = 0; z < zBlocks; z++)
	{
		for (int x = 0, xStart = 0; x < xBlocks; x++)
		{
			xStart = x * (blockWidth - 1);
			zStart = z * (blockDepth - 1);
			auto gridMesh = std::make_shared<HeightMapPatchListMesh>(
				device, cmdList, xStart, zStart, blockWidth, blockDepth, mTerrainScale, mHeightMapImage.get());
			SetMesh(gridMesh);
		}
	}	
}

float TerrainObject::GetHeight(float x, float z) const
{
	assert(mHeightMapImage && "HeightMapImage doesn't exist");
	return mHeightMapImage->GetHeight(x, z, mTerrainScale);
}

XMFLOAT3 TerrainObject::GetNormal(float x, float z) const
{
	assert(mHeightMapImage && "HeightMapImage doesn't exist");
	return mHeightMapImage->GetNormal((int)(x / mTerrainScale.x), (int)(z / mTerrainScale.z));
}


///////////////////////////////////////////////////////////////////////////////
//
Billboard::Billboard(float width, float height)
	: mDurationTime(0)
{
	mWidth = width;
	mHeight = height;
}

Billboard::~Billboard()
{
}

void Billboard::AppendBillboard(const XMFLOAT3& pos)
{
	BillboardVertex vertex = { pos, XMFLOAT2(mWidth, mHeight) };
	
	mVertices.push_back(vertex);
	mIndices.push_back((UINT)mIndices.size());
}

void Billboard::BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	auto mesh = std::make_shared<Mesh>();
	mesh->CreateResourceInfo(device, cmdList,
		sizeof(BillboardVertex), sizeof(UINT), D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
		mVertices.data(), (UINT)mVertices.size(), mIndices.data(), (UINT)mIndices.size());
	SetMesh(mesh);
}

void Billboard::SetDurationTime(std::chrono::milliseconds& time)
{
	mCreationTime = std::chrono::steady_clock::now();
	mDurationTime = time;
}

bool Billboard::IsTimeOver(std::chrono::steady_clock::time_point& currentTime)
{
	if (mDurationTime == std::chrono::milliseconds(0)) return false;
	return std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - mCreationTime) > mDurationTime;
}


///////////////////////////////////////////////////////////////////////////////
//
DynamicCubeMapObject::DynamicCubeMapObject(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, LONG cubeMapSize)
	: mCubeMapSize(cubeMapSize), mRtvCPUDescriptorHandles{}, mDsvCPUDescriptorHandle{}
{
	mViewPort = { 0.0f, 0.0f, float(cubeMapSize), float(cubeMapSize), 0.0f, 1.0f };
	mScissorRect = { 0, 0, cubeMapSize, cubeMapSize };

	for (std::unique_ptr<Camera>& camera : mCameras)
	{
		camera = std::make_unique<Camera>();
		camera->SetLens(0.5f*Math::PI, 1.0f, 0.1f, 5000.0f);
	}
}

DynamicCubeMapObject::~DynamicCubeMapObject()
{
}

void DynamicCubeMapObject::BuildDsvRtvView(
	ID3D12Device* device,
	ID3D12Resource* rtvResource,
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle, 
	D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUHandle)
{
	assert(mCubeMapSize > 0 && "CubeMapSize is not assigned.");

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	mDepthStencilBuffer = CreateTexture2DResource(
		device,
		mCubeMapSize, mCubeMapSize,
		1, 1, DXGI_FORMAT_D24_UNORM_S8_UINT,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue);

	mDsvCPUDescriptorHandle = dsvCPUHandle;
	device->CreateDepthStencilView(mDepthStencilBuffer.Get(), NULL, mDsvCPUDescriptorHandle);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.MipSlice = 0;
	rtvDesc.Texture2DArray.PlaneSlice = 0;
	rtvDesc.Texture2DArray.ArraySize = 1;

	for (int i = 0; i < RtvCounts; i++)
	{
		mRtvCPUDescriptorHandles[i] = rtvCPUHandle;
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		device->CreateRenderTargetView(rtvResource, &rtvDesc, mRtvCPUDescriptorHandles[i]);
		rtvCPUHandle.ptr += gRtvDescriptorSize;
	}
}

void DynamicCubeMapObject::BuildCameras()
{
	static XMFLOAT3 lookAts[RtvCounts] =
	{
		XMFLOAT3(+1.0f, 0.0f,  0.0f),
		XMFLOAT3(-1.0f, 0.0f,  0.0f),
		XMFLOAT3(0.0f, +1.0f,  0.0f),
		XMFLOAT3(0.0f, -1.0f,  0.0f),
		XMFLOAT3(0.0f,  0.0f, +1.0f),
		XMFLOAT3(0.0f,  0.0f, -1.0f)
	};

	static XMFLOAT3 ups[RtvCounts] =
	{
		XMFLOAT3(0.0f, +1.0f,  0.0f),
		XMFLOAT3(0.0f, +1.0f,  0.0f),
		XMFLOAT3(0.0f,  0.0f, -1.0f),
		XMFLOAT3(0.0f,  0.0f, +1.0f),
		XMFLOAT3(0.0f, +1.0f,  0.0f),
		XMFLOAT3(0.0f, +1.0f,  0.0f)
	};

	XMFLOAT3 pos = GetPosition();
	for (int i = 0; i < RtvCounts; i++)
	{
		mCameras[i]->SetPosition(pos);
		mCameras[i]->LookAt(pos, Vector3::Add(lookAts[i], pos), ups[i]);
		mCameras[i]->UpdateViewMatrix();
	}
}

void DynamicCubeMapObject::PreDraw(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* rtvResource, GameScene* scene)
{
	BuildCameras();

	cmdList->RSSetViewports(1, &mViewPort);
	cmdList->RSSetScissorRects(1, &mScissorRect);

	// resource barrier
	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		rtvResource, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	FLOAT clearValue[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	for (int i = 0; i < RtvCounts; i++)
	{
		cmdList->ClearRenderTargetView(mRtvCPUDescriptorHandles[i], clearValue, 0, NULL);
		cmdList->ClearDepthStencilView(mDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
		cmdList->OMSetRenderTargets(1, &mRtvCPUDescriptorHandles[i], TRUE, &mDsvCPUDescriptorHandle);

		scene->UpdateCameraConstant(i + 1, mCameras[i].get());
		scene->RenderPipelines(cmdList, i + 1);
	}

	// resource barrier
	cmdList->ResourceBarrier(1, &Extension::ResourceBarrier(
		rtvResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

