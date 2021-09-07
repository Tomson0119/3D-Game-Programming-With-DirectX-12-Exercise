#pragma once

namespace Extension
{
	inline D3D12_RESOURCE_BARRIER ResourceBarrier(
		ID3D12Resource* resource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter)
	{
		D3D12_RESOURCE_BARRIER resourceBarrier{};
		resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		resourceBarrier.Transition.pResource = resource;
		resourceBarrier.Transition.StateBefore = stateBefore;
		resourceBarrier.Transition.StateAfter = stateAfter;
		resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		return resourceBarrier;
	}

	inline D3D12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE type)
	{
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = type;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 1;
		heapProperties.VisibleNodeMask = 1;
		return heapProperties;
	}

	inline D3D12_RESOURCE_DESC BufferResourceDesc(UINT64 bytes)
	{
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = bytes;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		return resourceDesc;
	}

	inline D3D12_DESCRIPTOR_RANGE DescriptorRange(
		D3D12_DESCRIPTOR_RANGE_TYPE type,
		UINT descCount,
		UINT shaderRegister,
		UINT registerSpace = 0)
	{
		D3D12_DESCRIPTOR_RANGE descRange{};
		descRange.RangeType = type;
		descRange.NumDescriptors = descCount;
		descRange.BaseShaderRegister = shaderRegister;
		descRange.RegisterSpace = registerSpace;
		descRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		return descRange;
	}

	inline D3D12_ROOT_PARAMETER Descriptor(
		D3D12_ROOT_PARAMETER_TYPE type,
		UINT shaderRegister,
		D3D12_SHADER_VISIBILITY visibility,
		UINT registerSpace = 0)
	{
		D3D12_ROOT_PARAMETER rootParam{};
		rootParam.ParameterType = type;
		rootParam.Descriptor.ShaderRegister = shaderRegister;
		rootParam.ShaderVisibility = visibility;
		rootParam.Descriptor.RegisterSpace = registerSpace;
		return rootParam;
	}

	inline D3D12_ROOT_PARAMETER DescriptorTable(
		UINT numDescriptor,
		const D3D12_DESCRIPTOR_RANGE* descriptorRange,
		D3D12_SHADER_VISIBILITY visibility)
	{
		D3D12_ROOT_PARAMETER rootParam{};
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam.DescriptorTable.NumDescriptorRanges = numDescriptor;
		rootParam.DescriptorTable.pDescriptorRanges = descriptorRange;
		rootParam.ShaderVisibility = visibility;
		return rootParam;
	}

	inline D3D12_STATIC_SAMPLER_DESC SamplerDesc(
		UINT shaderRegister,
		D3D12_FILTER filter,
		D3D12_TEXTURE_ADDRESS_MODE addressMode)
	{
		D3D12_STATIC_SAMPLER_DESC samplerDesc{};
		samplerDesc.Filter = filter;
		samplerDesc.AddressU = addressMode;
		samplerDesc.AddressV = addressMode;
		samplerDesc.AddressW = addressMode;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.ShaderRegister = shaderRegister;
		samplerDesc.RegisterSpace = 0;
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		return samplerDesc;
	}

	inline D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc(
		UINT numParameters,
		const D3D12_ROOT_PARAMETER* rootParameters,
		UINT numStaticSamplers,
		const D3D12_STATIC_SAMPLER_DESC* samplerDesc,
		D3D12_ROOT_SIGNATURE_FLAGS flags)
	{
		D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
		rootSigDesc.NumParameters = numParameters;
		rootSigDesc.pParameters = rootParameters;
		rootSigDesc.NumStaticSamplers = numStaticSamplers;
		rootSigDesc.pStaticSamplers = samplerDesc;
		rootSigDesc.Flags = flags;
		return rootSigDesc;
	}

	inline D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc(
		UINT numDescriptors)
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
		descriptorHeapDesc.NumDescriptors = numDescriptors;
		descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descriptorHeapDesc.NodeMask = 0;
		return descriptorHeapDesc;
	}
}