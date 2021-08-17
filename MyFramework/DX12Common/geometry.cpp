#include "stdafx.h"
#include "geometry.h"


void GeometryMesh::CreateBox(float width, float height, float depth, uint32_t numSubdivisions)
{
	float hw = 0.5f * width;
	float hh = 0.5f * height;
	float hd = 0.5f * depth;

	mVertices.resize(24);

	// Front face
	mVertices[0] = Vertex(-hw, -hh, -hd, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	mVertices[1] = Vertex(-hw, +hh, -hd, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	mVertices[2] = Vertex(+hw, +hh, -hd, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	mVertices[3] = Vertex(+hw, -hh, -hd, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// Back face
	mVertices[4] = Vertex(-hw, -hh, +hd, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
	mVertices[5] = Vertex(-hw, +hh, +hd, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	mVertices[6] = Vertex(+hw, +hh, +hd, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	mVertices[7] = Vertex(+hw, -hh, +hd, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);

	// Left face
	mVertices[8]  = Vertex(-hw, -hh, +hd, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	mVertices[9]  = Vertex(-hw, +hh, +hd, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	mVertices[10] = Vertex(-hw, +hh, -hd, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	mVertices[11] = Vertex(-hw, -hh, -hd, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Right face
	mVertices[12] = Vertex(+hw, -hh, -hd, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	mVertices[13] = Vertex(+hw, +hh, -hd, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	mVertices[14] = Vertex(+hw, +hh, +hd, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	mVertices[15] = Vertex(+hw, -hh, +hd, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Top face
	mVertices[16] = Vertex(-hw, +hh, -hd, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	mVertices[17] = Vertex(-hw, +hh, +hd, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mVertices[18] = Vertex(+hw, +hh, +hd, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
	mVertices[19] = Vertex(+hw, +hh, -hd, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);

	// Bottom face
	mVertices[20] = Vertex(+hw, -hh, -hd, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);
	mVertices[21] = Vertex(+hw, -hh, +hd, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);
	mVertices[22] = Vertex(-hw, -hh, +hd, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);
	mVertices[23] = Vertex(-hw, -hh, -hd, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f);

	//
	// Create the indices.
	//
	mIndices.resize(36);

	// Front face
	mIndices[0] = 0, mIndices[1] = 1, mIndices[2] = 2;
	mIndices[3] = 0, mIndices[4] = 2, mIndices[5] = 3;

	// Back face
	mIndices[6] = 4, mIndices[7]  = 6, mIndices[8]  = 5;
	mIndices[9] = 4, mIndices[10] = 7, mIndices[11] = 6;

	// Left face
	mIndices[12] = 8, mIndices[13] = 9,  mIndices[14] = 10;
	mIndices[15] = 8, mIndices[16] = 10, mIndices[17] = 11;

	// Right face
	mIndices[18] = 12, mIndices[19] = 13, mIndices[20] = 14;
	mIndices[21] = 12, mIndices[22] = 14, mIndices[23] = 15;

	// Top face
	mIndices[24] = 16, mIndices[25] = 17, mIndices[26] = 18;
	mIndices[27] = 16, mIndices[28] = 18, mIndices[29] = 19;

	// Bottom face
	mIndices[30] = 20, mIndices[31] = 21, mIndices[32] = 22;
	mIndices[33] = 20, mIndices[34] = 22, mIndices[35] = 23;

	numSubdivisions = std::min<uint32_t>(numSubdivisions, 6U);
	for (uint32_t i = 0; i < numSubdivisions; ++i)
		Subdivide();
}

void GeometryMesh::CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount)
{
	//
	// Compute the vertices from top pole.
	//

	Vertex topVertex(0.0f, +radius, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);

	mVertices.push_back(topVertex);

	float phiStep = XM_PI / stackCount;
	float thetaStep = 2.0f * XM_PI / sliceCount;

	// Compute vertices for each stack ring ( do not count the poles as rings ).
	for (uint32_t i = 1; i < stackCount; ++i)
	{
		float phi = i * phiStep;

		// Vertices of ring.
		for (uint32_t j = 0; j <= sliceCount; ++j)
		{
			float theta = j * thetaStep;

			Vertex v;

			// spherical to cartesian.
			v.Position.x = radius * sinf(phi) * cosf(theta);
			v.Position.y = radius * cosf(phi);
			v.Position.z = radius * sinf(phi) * sinf(theta);

			v.Normal = Vector3::Normalize(v.Position);

			v.TexCoord.x = theta / XM_2PI;
			v.TexCoord.y = phi / XM_PI;

			mVertices.push_back(v);
		}
	}

	mVertices.push_back(bottomVertex);

	//
	// Compute indices for top stack.
	//

	for (uint32_t i = 1; i <= sliceCount; ++i)
	{
		mIndices.push_back(0);
		mIndices.push_back(i + 1);
		mIndices.push_back(i);
	}

	// 
	// Compute indices for inner stacks.
	//

	// Offset the indices to the index of the first vertex in the first ring.
	// This is just skipping the top pole vertex.
	uint32_t baseIndex = 1;
	uint32_t ringVertexCount = sliceCount + 1;
	for (uint32_t i = 0; i < stackCount - 2; ++i)
	{
		for (uint32_t j = 0; j < sliceCount; ++j)
		{
			mIndices.push_back(baseIndex + i * ringVertexCount + j);
			mIndices.push_back(baseIndex + i * ringVertexCount + j + 1);
			mIndices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

			mIndices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			mIndices.push_back(baseIndex + i * ringVertexCount + j + 1);
			mIndices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}

	//
	// Compute indices for bottom stack.
	//

	// South pole Vertex was added last.
	uint32_t southPoleIndex = (uint32_t)mVertices.size() - 1;

	// Offset the indices to the index of the first vertex in the last ring.
	baseIndex = southPoleIndex - ringVertexCount;

	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		mIndices.push_back(southPoleIndex);
		mIndices.push_back(baseIndex + i);
		mIndices.push_back(baseIndex + i + 1);
	}
}

void GeometryMesh::CreateGeosphere(float radius, uint32_t numSubdivisions)
{
	// Approximate a sphere by tessellating an icosahedron.
	const float X = 0.525731f;
	const float Z = 0.850651f;

	XMFLOAT3 pos[12] =
	{
		XMFLOAT3(-X, 0.0f, Z), XMFLOAT3(X, 0.0f, Z),
		XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),
		XMFLOAT3(0.0f, Z, X), XMFLOAT3(0.0f, Z, -X),
		XMFLOAT3(0.0f, -Z, X), XMFLOAT3(0.0f, -Z, -X),
		XMFLOAT3(Z, X, 0.0f), XMFLOAT3(-Z, X, 0.0f),
		XMFLOAT3(Z, -X, 0.0f), XMFLOAT3(-Z, -X, 0.0f)
	};

	uint32_t k[60] =
	{
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
	};

	mVertices.resize(12);
	mIndices.assign(&k[0], &k[60]);

	for (uint32_t i = 0; i < 12; ++i)
		mVertices[i].Position = pos[i];

	numSubdivisions = std::min<uint32_t>(numSubdivisions, 6u);
	for (uint32_t i = 0; i < numSubdivisions; ++i)
		Subdivide();

	// Project vertices onto sphere and scale.
	for (uint32_t i = 0; i < mVertices.size(); ++i)
	{
		mVertices[i].Normal = Vector3::Normalize(mVertices[i].Position);
		mVertices[i].Position = Vector3::Multiply(radius, mVertices[i].Normal);

		// Derive texture coordinates from spherical coordinates.
		float theta = atan2f(mVertices[i].Position.z, mVertices[i].Position.x);

		// Put in [0, 2pi].
		if (theta < 0.0f) theta += XM_2PI;

		float phi = acosf(mVertices[i].Position.y / radius);

		mVertices[i].TexCoord.x = theta / XM_2PI;
		mVertices[i].TexCoord.y = phi / XM_PI;
	}
}

void GeometryMesh::CreateGrid(float width, float depth, uint32_t m, uint32_t n)
{
	uint32_t vertexCount = m * n;
	uint32_t faceCount = (m - 1) * (n - 1) * 2;

	//
	// Create vertices
	//

	float hw = 0.5f * width;
	float hd = 0.5f * depth;

	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);

	mVertices.resize(vertexCount);
	for (uint32_t i = 0; i < m; ++i)
	{
		float z = hd - i * dz;
		for (uint32_t j = 0; j < n; ++j)
		{
			float x = -hw + j * dx;

			mVertices[i * n + j].Position = XMFLOAT3(x, 0.0f, z);
			mVertices[i * n + j].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			mVertices[i * n + j].TexCoord.x = j * du;
			mVertices[i * n + j].TexCoord.y = i * dv;
		}
	}

	//
	// Create the indices.
	//

	mIndices.resize((int)faceCount * 3);

	// Iterate over each quad and compute indices.
	uint32_t k = 0;
	for (uint32_t i = 0; i < m - 1; ++i)
	{
		for (uint32_t j = 0; j < n - 1; ++j)
		{
			mIndices[k] = i * n + j;
			mIndices[k + 1] = i * n + j + 1;
			mIndices[k + 2] = (i + 1) * n + j;

			mIndices[k + 3] = (i + 1) * n + j;
			mIndices[k + 4] = i * n + j + 1;
			mIndices[k + 5] = (i + 1) * n + j + 1;

			k += 6;
		}
	}
}

void GeometryMesh::CreateCylinder(float bottomRadius, float topRadius, float height,
	uint32_t sliceCount, uint32_t stackCount)
{
	//
	// Build Stacks.
	//

	float stackHeight = height / stackCount;

	// Amount to increment radius.
	float radiusStep = (topRadius - bottomRadius) / stackCount;
	uint32_t ringCount = stackCount + 1;

	// Compute vertices for each stack ring.
	for (uint32_t i = 0; i < ringCount; ++i)
	{
		float y = -0.5f * height + i * stackHeight;
		float r = bottomRadius + i * radiusStep;

		// vertices of ring
		float dTheta = 2.0f * XM_PI / sliceCount;

		for (uint32_t j = 0; j <= sliceCount; ++j)
		{
			Vertex vertex;

			float c = cosf(j * dTheta);
			float s = sinf(j * dTheta);

			vertex.Position = XMFLOAT3(r * c, y, r * s);
			vertex.TexCoord.x = (float)j / sliceCount;
			vertex.TexCoord.y = 1.0f - (float)i / stackCount;

			XMFLOAT3 tangent = XMFLOAT3(-s, 0.0f, c);

			float dr = bottomRadius - topRadius;
			XMFLOAT3 bitangent(dr * c, -height, dr * s);

			XMFLOAT3 N = Vector3::Normalize(Vector3::Cross(tangent, bitangent));
			vertex.Normal = N;

			mVertices.push_back(vertex);
		}
	}

	// Add one because we duplicate the first and las vertex per ring
	// since the texture coordinates are different.
	uint32_t ringVertexCount = sliceCount + 1;

	// Compute indices for each stack.
	for (uint32_t i = 0; i < stackCount; ++i)
	{
		for (uint32_t j = 0; j < sliceCount; ++j)
		{
			mIndices.push_back(i * ringVertexCount + j);
			mIndices.push_back((i + 1) * ringVertexCount + j);
			mIndices.push_back((i + 1) * ringVertexCount + j + 1);

			mIndices.push_back(i * ringVertexCount + j);
			mIndices.push_back((i + 1) * ringVertexCount + j + 1);
			mIndices.push_back(i * ringVertexCount + j + 1);
		}
	}

	BuildCylinderTopCap(bottomRadius, topRadius, height, sliceCount, stackCount);
	BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount);
}

void GeometryMesh::BuildCylinderTopCap(
	float bottomRadius, float topRadius, float height,
	uint32_t sliceCount, uint32_t stackCount)
{
	uint32_t baseIndex = (uint32_t)mVertices.size();

	float y = 0.5f * height;
	float dTheta = 2.0f * XM_PI / sliceCount;

	// Duplicate cap ring vertices because the texture coordinates and normals differ.
	for (uint32_t i = 0; i <= sliceCount; ++i)
	{
		float x = topRadius * cosf(i * dTheta);
		float z = topRadius * sinf(i * dTheta);

		// Scale down by the height to try and make top cap texture coord area
		// proportional to base.
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		mVertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, u, v));
	}

	mVertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f));

	uint32_t centerIndex = (uint32_t)mVertices.size() - 1;

	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		mIndices.push_back(centerIndex);
		mIndices.push_back(baseIndex + i + 1);
		mIndices.push_back(baseIndex + i);
	}
}

void GeometryMesh::BuildCylinderBottomCap(
	float bottomRadius, float topRadius, float height,
	uint32_t sliceCount, uint32_t stackCount)
{
	uint32_t baseIndex = (uint32_t)mVertices.size();

	float y = -0.5f * height;
	float dTheta = 2.0f * XM_PI / sliceCount;

	// Duplicate cap ring vertices because the texture coordinates and normals differ.
	for (uint32_t i = 0; i <= sliceCount; ++i)
	{
		float x = bottomRadius * cosf(i * dTheta);
		float z = bottomRadius * sinf(i * dTheta);

		// Scale down by the height to try and make top cap texture coord area
		// proportional to base.
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		mVertices.push_back(Vertex(x, y, z, 0.0f, -1.0f, 0.0f, u, v));
	}

	mVertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 0.5f, 0.5f));

	uint32_t centerIndex = (uint32_t)mVertices.size() - 1;

	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		mIndices.push_back(centerIndex);
		mIndices.push_back(baseIndex + i);
		mIndices.push_back(baseIndex + i + 1);
	}
}

void GeometryMesh::CreateQuad(float x, float y, float w, float h, float depth)
{
	mVertices.resize(4);
	mIndices.resize(6);

	// Position coordinates specified in NDC space.

	mVertices[0] = Vertex(
		x, y - h, depth,
		0.0f, 0.0f, -1.0f,
		0.0f, 1.0f);

	mVertices[1] = Vertex(
		x, y, depth,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f);

	mVertices[2] = Vertex(
		x + w, y, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f);

	mVertices[3] = Vertex(
		x + w, y - h, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 1.0f);

	mIndices[0] = 0;
	mIndices[1] = 1;
	mIndices[2] = 2;

	mIndices[3] = 0;
	mIndices[4] = 2;
	mIndices[5] = 3;
}

void GeometryMesh::Subdivide()
{
	// Save a copy of vertices and indices.
	auto copyVertices = mVertices;
	auto copyIndices = mIndices;

	mVertices.resize(0);
	mIndices.resize(0);

	//       v1
	//       *
	//      / \
	//     /   \
	//  m0*-----*m1
	//   / \   / \
	//  /   \ /   \
	// *-----*-----*
	// v0    m2    v2

	uint32_t numTris = (uint32_t)copyIndices.size() / 3;
	for (uint32_t i = 0; i < numTris; ++i)
	{
		Vertex v0 = copyVertices[copyIndices[i * 3 + 0]];
		Vertex v1 = copyVertices[copyIndices[i * 3 + 1]];
		Vertex v2 = copyVertices[copyIndices[i * 3 + 2]];

		// Generate the midpoints.
		Vertex m0 = MidPoint(v0, v1);
		Vertex m1 = MidPoint(v1, v2);
		Vertex m2 = MidPoint(v0, v2);

		// Add new geometry.
		mVertices.push_back(v0); // 0
		mVertices.push_back(v1); // 1
		mVertices.push_back(v2); // 2
		mVertices.push_back(m0); // 3
		mVertices.push_back(m1); // 4
		mVertices.push_back(m2); // 5

		mIndices.push_back(i * 6 + 0);
		mIndices.push_back(i * 6 + 3);
		mIndices.push_back(i * 6 + 5);

		mIndices.push_back(i * 6 + 3);
		mIndices.push_back(i * 6 + 1);
		mIndices.push_back(i * 6 + 4);

		mIndices.push_back(i * 6 + 5);
		mIndices.push_back(i * 6 + 4);
		mIndices.push_back(i * 6 + 2);

		mIndices.push_back(i * 6 + 3);
		mIndices.push_back(i * 6 + 4);
		mIndices.push_back(i * 6 + 5);
	}
}

Vertex GeometryMesh::MidPoint(Vertex& v0, Vertex& v1)
{
	Vertex v;
	v.Position = Vector3::Multiply(0.5f, Vector3::Add(v0.Position, v1.Position));
	v.Normal = Vector3::Normalize(Vector3::Multiply(0.5f, Vector3::Add(v0.Normal, v1.Normal)));
	v.TexCoord = Vector2::Multiply(0.5f, Vector2::Add(v0.TexCoord, v1.TexCoord));
	
	return v;
}
