#pragma once

class FrameResource
{
public:
	FrameResource(ID3D12Device* device, UINT objectCount, UINT cameraCount, UINT lightCount);
	FrameResource(const FrameResource&) = delete;
	FrameResource& operator=(const FrameResource&) = delete;
	~FrameResource();

	// �����Ӹ��� ������ �ִ� ���� Ŀ�ǵ� �Ҵ���.
	// CPU�� �Ҵ��ڸ� �缳������ ���ϵ��� �������� �����Ѵ�.
	ComPtr<ID3D12CommandAllocator> mCmdAlloc;

	// �����Ӹ��� ��� ���۵��� ���� ������ �ִ´�.
	// CPU�� ������۸� �缳������ �ʵ��� ������ ��ü�� ���� �ִ´�.
	std::unique_ptr<ConstantBuffer<ObjectConstants>> mObjectCB;
	std::unique_ptr<ConstantBuffer<CameraConstants>> mCameraCB;
	std::unique_ptr<ConstantBuffer<LightConstants>> mLightCB;

	UINT mFence = 0;
};