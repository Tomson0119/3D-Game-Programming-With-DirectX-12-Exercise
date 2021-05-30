#pragma once

class Camera
{
public:
	Camera();
	Camera(float fovY, float aspect, float zn, float zf);
	Camera(const Camera& rhs) = delete;
	Camera& operator=(const Camera& rhs) = delete;
	~Camera();

	XMFLOAT3 GetPosition() const { return mPosition; }
	XMFLOAT3 GetRight() const { return mRight; }
	XMFLOAT3 GetUp() const { return mUp; }
	XMFLOAT3 GetLook() const { return mLook; }

	float GetNearZ() const { return mNearZ; }
	float GetFarZ() const { return mFarZ; }
	float GetAspect() const { return mAspect; }
	
	XMFLOAT2 GetFov() const { return mFov; }
	XMFLOAT2 GetNearWindow() const { return mNearWindow; }
	XMFLOAT2 GetFarWindow() const { return mFarWindow; }

	void SetPosition(float x, float y, float z);
	void SetPosition(const XMFLOAT3& pos);

	void SetLens(float fovY, float aspect, float zn, float zf);
	void LookAt(XMFLOAT3& pos, XMFLOAT3& target, XMFLOAT3& up);

	XMFLOAT4X4 GetView() const;
	XMFLOAT4X4 GetProj() const { return mProj; }

	void Move(float dx, float dy, float dz);

	void Pitch(float angle);
	void RotateY(float angle);

	void UpdateViewMatrix();

private:
	bool mViewDirty = false;

	XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };

	float mFarZ = 0.0f;
	float mNearZ = 0.0f;
	float mAspect = 0.0f;
	
	XMFLOAT2 mFov = { 0.0f, 0.0f };
	XMFLOAT2 mNearWindow = { 0.0f, 0.0f };
	XMFLOAT2 mFarWindow = { 0.0f, 0.0f };	

	XMFLOAT4X4 mView = Matrix4x4::Identity4x4();
	XMFLOAT4X4 mProj = Matrix4x4::Identity4x4();
};