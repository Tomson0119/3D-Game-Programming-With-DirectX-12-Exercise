#pragma once

class Camera
{
public:
	Camera();
	Camera(const Camera& rhs) = delete;
	Camera& operator=(const Camera& rhs) = delete;
	virtual ~Camera();

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
	void Move(XMFLOAT3& dir, float dist);

	void Strafe(float dist);
	void Walk(float dist);
	void Upward(float dist);
	
	void Pitch(float angle);
	void RotateY(float angle);

	virtual void UpdateViewMatrix();

	bool IsInFrustum(BoundingOrientedBox& boundBox);

protected:
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
	XMFLOAT4X4 mInvView = Matrix4x4::Identity4x4();

	BoundingFrustum mFrustumView;
	BoundingFrustum mFrustumWorld;
};

//class TopViewCamera : public Camera
//{
//public:
//	TopViewCamera(float radius, float theta, float phi);
//	TopViewCamera(const Camera& rhs) = delete;
//	TopViewCamera& operator=(const Camera& rhs) = delete;
//	virtual ~TopViewCamera();
//
//	void SetTarget(const XMFLOAT3& target);
//	void RotateTheta(float dx);
//	void RotatePhi(float dy);
//
//	virtual void UpdateViewMatrix();
//
//private:
//	XMFLOAT3 mTarget = Vector3::Zero();
//
//	float mRadius = 0.0f;
//	float mPhi = 0.0f;
//	float mTheta = 0.0f;
//};