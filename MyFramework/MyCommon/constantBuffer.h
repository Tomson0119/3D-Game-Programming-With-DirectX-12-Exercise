#pragma once

#include "stdafx.h"

template<typename Cnst>
class ConstantBuffer
{
public:
	ConstantBuffer(ID3D12Device* device, UINT count)
	{	
		// 바이트 크기는 항상 256의 배수가 되어야 한다.
		mByteSize = (sizeof(Cnst) + 255) & ~255;

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mByteSize * count),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&mUploadBuffer)));

		ThrowIfFailed(mUploadBuffer->Map(0, nullptr, (void**)(&mData)));
	}
	ConstantBuffer(const ConstantBuffer& rhs) = delete;
	ConstantBuffer& operator=(const ConstantBuffer& rhs) = delete;
	~ConstantBuffer()
	{
		if (mUploadBuffer)
			mUploadBuffer->Unmap(0, nullptr);
		mData = nullptr;
	}

	ID3D12Resource* Resource() const
	{
		return mUploadBuffer.Get();
	}

	void CopyData(int index, const Cnst& data)
	{
		memcpy(&mData[index * mByteSize], &data, sizeof(Cnst));
	}

	UINT GetByteSize() const
	{
		return mByteSize;
	}
	
private:
	ComPtr<ID3D12Resource> mUploadBuffer = nullptr;
	BYTE* mData = nullptr;
	UINT mByteSize = 0;
};