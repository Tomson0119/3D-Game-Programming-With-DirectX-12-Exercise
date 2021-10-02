#pragma once

#include "stdafx.h"

struct Vertex
{
	Vertex() = default;
	Vertex(float x, float y, float z,
		float nx, float ny, float nz,
		float u, float v)
		: Position(x, y, z), Normal(nx, ny, nz), TexCoord(u, v) { }

	Vertex(const XMFLOAT3& pos,
		const XMFLOAT3& normal,
		const XMFLOAT2& texC)
		: Position(pos), Normal(normal), TexCoord(texC) { }

	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 TexCoord;
};

struct TerrainVertex
{
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 TexCoord0;
	XMFLOAT2 TexCoord1;
};