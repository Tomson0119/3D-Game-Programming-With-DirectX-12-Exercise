#include "stdafx.h"
#include "heightMapImage.h"

HeightMapImage::HeightMapImage(const std::wstring& path,
	int width, int depth, const XMFLOAT3& scale)
	: mWidth(width), mDepth(depth), mScale(scale)
{
	const int imgSize = width * depth;

	std::ifstream file{ path, std::ios::in | std::ios::binary };
	assert(file.is_open() && "No such file in directory.");
	std::vector<BYTE> pixels(imgSize);
	file.read(reinterpret_cast<char*>(pixels.data()), imgSize);
	
	mPixels.resize(imgSize);
	for (size_t z = 0; z < mDepth; z++)
	{
		for (size_t x = 0; x < mWidth; x++)
		{
			mPixels[x + ((mDepth - 1 - z) * mWidth)] = pixels[x + (z * mWidth)];
		}
	}
}

HeightMapImage::~HeightMapImage()
{
}

float HeightMapImage::GetHeight(float fx, float fz, const XMFLOAT3& scale) const
{
	fx /= scale.x;
	fz /= scale.z;

	if (fx < 0.0f || fz < 0.0f || fx >= (mWidth-1) || fz >= (mDepth-1))
		return 0.0f;

	const size_t x = (size_t)fx;
	const size_t z = (size_t)fz;
	const float xPercent = fx - x;
	const float zPercent = fz - z;

	float bl = (float)mPixels[x + (z * mWidth)];
	float br = (float)mPixels[(x + 1) + (z * mWidth)];
	float tl = (float)mPixels[x + ((z + 1) * mWidth)];
	float tr = (float)mPixels[(x + 1) + ((z + 1) * mWidth)];
	
	const bool reverse = !(z & 1);

	if (reverse)
	{
		if (zPercent >= xPercent) br = bl + (tr - tl);
		else tl = tr + (bl - br);
	}
	else
	{
		if (zPercent < 1.0f - xPercent)	tr = tl + (br - bl);
		else bl = tl + (br - tr);
	}

	const float topHeight = std::lerp(tl, tr, xPercent);
	const float bottomHeight = std::lerp(bl, br, xPercent);
	const float height = std::lerp(bottomHeight, topHeight, zPercent);

	return height * scale.y;
}

XMFLOAT3 HeightMapImage::GetNormal(int x, int z) const
{
	if (x < 0.0f || z < 0.0f || x >= (mWidth-1) || z >= (mDepth-1))
		return XMFLOAT3(0.0f, 1.0f, 0.0f);

	const int index = x + (z * mWidth);
	const int xAdd = (x < (mWidth - 1)) ? 1 : -1;
	const int zAdd = (z < (mDepth - 1)) ? mWidth : -mWidth;

	const float y1 = (float)mPixels[index] * mScale.y;
	const float y2 = (float)mPixels[index + xAdd] * mScale.y;
	const float y3 = (float)mPixels[index + zAdd] * mScale.y;

	XMFLOAT3 edge1 = XMFLOAT3(0.0f, y3 - y1, mScale.z);
	XMFLOAT3 edge2 = XMFLOAT3(mScale.x, y2 - y1, 0.0f);

	return Vector3::Normalize(Vector3::Cross(edge1, edge2));
}
