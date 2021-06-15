#pragma once

class HeightMapImage
{
public:
	HeightMapImage(
		const std::wstring& path, 
		int width, 
		int depth, 
		const XMFLOAT3& scale);
	virtual ~HeightMapImage();

	float GetHeight(float fx, float fz, bool reverse);
	XMFLOAT3 GetNormal(float fx, float fz, bool reverse);

	std::vector<BYTE> GetPixels() const { return mPixels; }

	XMFLOAT3 GetScale() const { return mScale; }
	int GetWidth() const { return mWidth; }
	int GetDepth() const { return mDepth; }


private:
	std::vector<BYTE> mPixels;
	XMFLOAT3 mScale;

	int mWidth;
	int mDepth;
};