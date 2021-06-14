#include "stdafx.h"
#include "heightMapImage.h"

HeightMapImage::HeightMapImage(const std::wstring& path,
	int width, int depth, const XMFLOAT3& scale)
	: mWidth(width), mDepth(depth), mScale(scale)
{
	HANDLE file = CreateFile(path.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY, NULL);

	BYTE* pixels = new BYTE[mWidth * mDepth];
	DWORD read;
	assert(ReadFile(file, pixels, (mWidth * mDepth), &read, NULL));		
	CloseHandle(file);

	mPixels.resize(mWidth * mDepth);
	for (int z = 0; z < mDepth; ++z)
	{
		for (int x = 0; x < mWidth; ++x)
			mPixels[x + ((mDepth - 1 - z) * mWidth)] = pixels[x + (z * mWidth)];
	}
	if (pixels) delete[] pixels;
}

HeightMapImage::~HeightMapImage()
{
}

float HeightMapImage::GetHeight(float fx, float fz, bool reverse)
{
	fx = fx / mScale.x;
	fz = fz / mScale.z;

	if (fx < 0.0f || fz < 0.0f || fx >= mWidth || fz >= mDepth)
		return 0.0f;

	int x = (int)fx, z = (int)fz;
	float xPercent = fx - x, zPercent = fz - z;

	float bl = (float)mPixels[x + (z * mWidth)];
	float br = (float)mPixels[x + 1 + (z * mWidth)];
	float tl = (float)mPixels[x + ((z + 1) * mWidth)];
	float tr = (float)mPixels[x + 1 + ((z + 1) * mWidth)];
	
	if (reverse)
	{
		if (zPercent >= xPercent)
			br = bl + (tr - tl);
		else
			tl = tr + (bl - br);
	}
	else
	{
		if (zPercent < 1.0f - xPercent)
			tr = tl + (br - bl);
		else
			bl = tl + (br - tr);
	}

	float topHeight = tl * (1.0f - xPercent) + tr * xPercent;
	float bottomHeight = bl * (1.0f - xPercent) + br * xPercent;
	float height = bottomHeight * (1.0f - zPercent) + topHeight * zPercent;

	return height * mScale.y;
}

XMFLOAT3 HeightMapImage::GetNormal(int x, int z)
{
	if (x < 0.0f || z < 0.0f || x >= mWidth || z >= mDepth)
		return XMFLOAT3(0.0f, 1.0f, 0.0f);

	int index = x + (z * mWidth);
	int xAdd = (x < (mWidth - 1)) ? 1 : -1;
	int zAdd = (z < (mDepth - 1)) ? mWidth : -mWidth;

	float y1 = (float)mPixels[index] * mScale.y;
	float y2 = (float)mPixels[index + xAdd] * mScale.y;
	float y3 = (float)mPixels[index + zAdd] * mScale.y;

	XMFLOAT3 edge1 = XMFLOAT3(0.0f, y3 - y2, mScale.z);
	XMFLOAT3 edge2 = XMFLOAT3(mScale.x, y2 - y1, 0.0f);

	return Vector3::Cross(edge1, edge2);
}
