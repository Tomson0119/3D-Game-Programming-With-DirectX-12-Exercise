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
}