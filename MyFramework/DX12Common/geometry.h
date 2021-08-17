#pragma once

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

class GeometryMesh
{
public:
	GeometryMesh() = default;

	void CreateBox(float width, float height, float depth, uint32_t numSubdivisions);
	void CreateSphere(float radius, uint32_t slices, uint32_t stacks);
	void CreateGeosphere(float radius, uint32_t numSubdivisions);
	void CreateGrid(float width, float depth, uint32_t m, uint32_t n);

	void CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t slices, uint32_t stacks);
	void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount);
	void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount);	

	void CreateQuad(float x, float y, float w, float h, float depth);

	void Subdivide();
	Vertex MidPoint(Vertex& v0, Vertex& v1);

private:
	std::vector<Vertex> mVertices;
	std::vector<UINT> mIndices;
};