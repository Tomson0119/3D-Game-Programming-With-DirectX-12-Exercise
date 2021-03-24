#pragma once

#include "../../Common/myd3dUtil.h"
#include "../../Common/myMathHelper.h"

// Lightweight structure stores parameters to draw a shape.
struct RenderItem
{
	RenderItem() = default;

	// World matrix of the shape that describes the object's local space.
	// It defines the position, orientation, and scale of the object in the world.
	DirectX::XMFLOAT4X4 World = MyMathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MyMathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed
	// and we need to update the constant buffer.
	// When we modify object data we should set NumFramesDirty = gNumFrameResources
	// so that each frame resources gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU cbuffer corresponding to the objectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Count
};