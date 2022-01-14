#include "stdafx.h"
#include "camera.h"
#include "player.h"


Camera::Camera()
{
}

Camera::~Camera()
{
}

void Camera::SetPosition(float x, float y, float z)
{
	XMFLOAT3 pos = { x,y,z };
	SetPosition(pos);
}

void Camera::SetPosition(const XMFLOAT3& pos)
{
	mPosition = pos;
	mViewDirty = true;
}

void Camera::SetLook(const XMFLOAT3& look)
{
	mLook = look;
	mViewDirty = true;
}

void Camera::SetUp(const XMFLOAT3& up)
{
	mUp = up;
	mViewDirty = true;
}

void Camera::SetLens(float aspect)
{
	SetLens(mFov.y, aspect, mNearZ, mFarZ);
}

void Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
	mNearWindow.y = 2.0f * tanf(fovY * 0.5f) * zn;
	mNearWindow.x = aspect * mNearWindow.y;

	mFarWindow.y = 2.0f * tanf(fovY * 0.5f) * zf;
	mFarWindow.x = aspect * mFarWindow.y;

	mFov.x = 2.0f * atan(mNearWindow.x * 0.5f / mNearZ);
	mFov.y = fovY;

	mAspect = aspect;
	mNearZ = zn;
	mFarZ = zf;

	XMMATRIX P = XMMatrixPerspectiveFovLH(mFov.y, mAspect, mNearZ, mFarZ);
	XMStoreFloat4x4(&mProj, P);
	BoundingFrustum::CreateFromMatrix(mFrustumView, P);
}

void Camera::SetOrthographicLens(XMFLOAT3& center, float range)
{
	UpdateViewMatrix();

	XMFLOAT3 C = Vector3::TransformCoord(center, mView);
	XMMATRIX P = XMMatrixOrthographicOffCenterLH(
		C.x - range, C.x + range,
		C.y - range, C.y + range,
		C.z - range, C.z + range);
	XMStoreFloat4x4(&mProj, P);
	BoundingFrustum::CreateFromMatrix(mFrustumView, P);
}

void Camera::LookAt(XMFLOAT3& pos, XMFLOAT3& target, XMFLOAT3& up)
{
	mPosition = pos;
	mLook = Vector3::Normalize(Vector3::Subtract(target, pos));
	mRight = Vector3::Normalize(Vector3::Cross(up, mLook));
	mUp = Vector3::Cross(mLook, mRight);

	mViewDirty = true;
}

void Camera::LookAt(XMFLOAT3& target)
{
	LookAt(mPosition, target, GetUp());
}

void Camera::LookAt(float x, float y, float z)
{
	LookAt(mPosition, XMFLOAT3(x,y,z), GetUp());
}

XMFLOAT4X4 Camera::GetView() const
{
	assert(!mViewDirty && "Camera -> mView is not updated!!");
	return mView;
}

CameraConstants Camera::GetConstants() const
{
	CameraConstants cameraCnst;
	cameraCnst.View = Matrix4x4::Transpose(GetView());
	cameraCnst.Proj = Matrix4x4::Transpose(GetProj());
	cameraCnst.ViewProj = Matrix4x4::Transpose(Matrix4x4::Multiply(GetView(), GetProj()));
	cameraCnst.CameraPos = GetPosition();
	cameraCnst.Aspect = mAspect;
	return cameraCnst;
}

void Camera::Move(float dx, float dy, float dz)
{
	mPosition = Vector3::MultiplyAdd(dx, mRight, mPosition);
	mPosition = Vector3::MultiplyAdd(dy, mUp, mPosition);
	mPosition = Vector3::MultiplyAdd(dz, mLook, mPosition);

	mViewDirty = true;
}

void Camera::Move(XMFLOAT3& dir, float dist)
{
	mPosition = Vector3::MultiplyAdd(dist, dir, mPosition);
	mViewDirty = true;
}

void Camera::Strafe(float dist)
{
	Move(dist, 0.0f, 0.0f);
}

void Camera::Walk(float dist)
{
	Move(0.0f, 0.0f, dist);
}

void Camera::Upward(float dist)
{
	Move(XMFLOAT3(0.0f, 1.0f, 0.0f), dist);
}

void Camera::Pitch(float angle)
{
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mRight), XMConvertToRadians(angle));
	mUp = Vector3::TransformNormal(mUp, R);
	mLook = Vector3::TransformNormal(mLook, R);

	mViewDirty = true;
}

void Camera::RotateY(float angle)
{
	XMMATRIX R = XMMatrixRotationY(XMConvertToRadians(angle));
	mRight = Vector3::TransformNormal(mRight, R);
	mUp = Vector3::TransformNormal(mUp, R);
	mLook = Vector3::TransformNormal(mLook, R);

	mViewDirty = true;
}

void Camera::Update(const float elapsedTime)
{
	UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
	if (mViewDirty)
	{
		mLook = Vector3::Normalize(mLook);
		mUp = Vector3::Normalize(Vector3::Cross(mLook, mRight));
		mRight = Vector3::Cross(mUp, mLook);

		mView(0, 0) = mRight.x; mView(0, 1) = mUp.x; mView(0, 2) = mLook.x;
		mView(1, 0) = mRight.y; mView(1, 1) = mUp.y; mView(1, 2) = mLook.y;
		mView(2, 0) = mRight.z; mView(2, 1) = mUp.z; mView(2, 2) = mLook.z;
		mView(3, 0) = -Vector3::Dot(mPosition, mRight);		
		mView(3, 1) = -Vector3::Dot(mPosition, mUp);		
		mView(3, 2) = -Vector3::Dot(mPosition, mLook);

		XMMATRIX viewMat = XMLoadFloat4x4(&mView);
		XMMATRIX invViewMat = XMMatrixInverse(&XMMatrixDeterminant(viewMat), viewMat);
		XMStoreFloat4x4(&mInvView, invViewMat);

		mFrustumView.Transform(mFrustumWorld, XMLoadFloat4x4(&mInvView));

		mViewDirty = false;
	}
}

bool Camera::IsInFrustum(BoundingOrientedBox& boundBox)
{
	return mFrustumWorld.Intersects(boundBox);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
FirstPersonCamera::FirstPersonCamera()
	: Camera()
{
	mMode = CameraMode::FIRST_PERSON_CAMERA;
}

FirstPersonCamera::~FirstPersonCamera()
{
}

void FirstPersonCamera::Update(const float elapsedTime)
{
	if (mPlayer)
	{
		mPosition = Vector3::Add(mPlayer->GetPosition(), GetOffset());
		mLook = mPlayer->GetLook();
		mRight = mPlayer->GetRight();
		mUp = mPlayer->GetUp();
	}
	Camera::Update(elapsedTime);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
ThirdPersonCamera::ThirdPersonCamera()
	: Camera()
{
	mMode = CameraMode::THIRD_PERSON_CAMERA;
}

ThirdPersonCamera::~ThirdPersonCamera()
{
}

void ThirdPersonCamera::Update(float elapsedTime)
{
	if (mPlayer)
	{
		XMFLOAT4X4 R = Matrix4x4::Identity4x4();
		XMFLOAT3 right = mPlayer->GetRight();
		XMFLOAT3 up = mPlayer->GetUp();
		XMFLOAT3 look = mPlayer->GetLook();

		R(0, 0) = right.x;  R(0, 1) = right.y;  R(0, 2) = right.z;
		R(1, 0) = up.x;		R(1, 1) = up.y;		R(1, 2) = up.z;
		R(2, 0) = look.x;	R(2, 1) = look.y;	R(2, 2) = look.z;

		XMFLOAT3 offset = Vector3::TransformCoord(mOffset, R);
		XMFLOAT3 position = Vector3::Add(mPlayer->GetPosition(), offset);
		XMFLOAT3 direction = Vector3::Subtract(position, mPosition);

		float length = Vector3::Length(direction);
		direction = Vector3::Normalize(direction);
		float timeLagScale = (mTimeLag) ? elapsedTime * (1.0f / mTimeLag) : 1.0f;
		float distance = length * timeLagScale;

		if (distance > length) distance = length;
		if (length < 0.01f) distance = length;
		if (distance > 0.0f)
		{
			mPosition = Vector3::Add(mPosition, direction, distance);
			ThirdPersonCamera::LookAt(mPlayer->GetPosition());
		}
	}
	Camera::Update(elapsedTime);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
TopDownCamera::TopDownCamera()
	: Camera()
{
	mMode = CameraMode::TOP_DOWN_CAMERA;
}

TopDownCamera::~TopDownCamera()
{

}

void TopDownCamera::Update(float elapsedTime)
{
	if (mPlayer)
	{
		XMFLOAT3 offset = Vector3::Add(mPlayer->GetPosition(), GetOffset());
		SetPosition(offset);
		LookAt(mPlayer->GetPosition());
	}
	Camera::Update(elapsedTime);
}