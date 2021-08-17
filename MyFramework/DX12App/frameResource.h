#pragma once

class FrameResource
{
public:
	FrameResource(ID3D12Device* device, UINT objectCount, UINT cameraCount, UINT lightCount);
	FrameResource(const FrameResource&) = delete;
	FrameResource& operator=(const FrameResource&) = delete;
	~FrameResource();

	// 프레임마다 가지고 있는 고유 커맨드 할당자.
	// CPU가 할당자를 재설정하지 못하도록 프레임이 소유한다.
	ComPtr<ID3D12CommandAllocator> mCmdAlloc;

	// 프레임마다 상수 버퍼들을 각자 가지고 있는다.
	// CPU가 상수버퍼를 재설정하지 않도록 프레임 객체가 갖고 있는다.
	std::unique_ptr<ConstantBuffer<ObjectConstants>> mObjectCB;
	std::unique_ptr<ConstantBuffer<CameraConstants>> mCameraCB;
	std::unique_ptr<ConstantBuffer<LightConstants>> mLightCB;

	UINT mFence = 0;
};