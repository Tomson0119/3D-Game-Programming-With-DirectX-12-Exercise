#include "stdafx.h"

ComPtr<ID3D12Resource> CreateBufferResource(
    ID3D12Device* device, 
    ID3D12GraphicsCommandList* cmdList,
    const void* initData, UINT64 byteSize,
    ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultResource;

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COPY_DEST, NULL,
        IID_PPV_ARGS(&defaultResource)));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL, IID_PPV_ARGS(&uploadBuffer)));

	// 기본 버퍼에 넣을 데이터를 서술하는 구조체
	D3D12_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pData = initData;
	subresourceData.RowPitch = byteSize;
	subresourceData.SlicePitch = byteSize;

	UpdateSubresources(cmdList, defaultResource.Get(), 
		uploadBuffer.Get(), 0, 0, 1, &subresourceData);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		defaultResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	return defaultResource;
}


ComPtr<ID3DBlob> CompileShader(
	const std::wstring& fileName,
	const std::string& entry,
	const std::string& target,
	const D3D_SHADER_MACRO* defines)
{
	UINT compileFlags = 0;
#ifdef _DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> codeBlob;
	ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3DCompileFromFile(fileName.c_str(), defines, NULL,
		entry.c_str(), target.c_str(), compileFlags, NULL,
		&codeBlob, &errorBlob);

	if (errorBlob)
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	ThrowIfFailed(hr);

	return codeBlob;
}