#pragma once

#include "stdafx.h"

struct Vertex
{
	Vertex() = default;
	Vertex(float x, float y, float z,
		float nx, float ny, float nz,
		float tx, float ty, float tz,
		float u, float v)
		: Position(x, y, z), Normal(nx, ny, nz), TangentU(tx,ty,tz), TexCoord(u, v) { }

	Vertex(const XMFLOAT3& pos,
		const XMFLOAT3& normal,
		const XMFLOAT3& tangent,
		const XMFLOAT2& texC)
		: Position(pos), Normal(normal), TangentU(tangent), TexCoord(texC) { }

	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT3 TangentU;
	XMFLOAT2 TexCoord;
};

struct TerrainVertex
{
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 TexCoord0;
	XMFLOAT2 TexCoord1;
};

struct BillboardVertex
{
	XMFLOAT3 Position;
	XMFLOAT2 Size;
	XMFLOAT3 Direction;
	XMFLOAT2 Age;
	float	 Speed;
	UINT     Type;
};