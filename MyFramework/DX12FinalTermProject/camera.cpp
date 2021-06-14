#include "stdafx.h"
#include "camera.h"

Camera::Camera()
{
	SetLens(0.25f * Math::PI, 1.0f, 1.0f, 1000.0f);
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

void Camera::LookAt(XMFLOAT3& pos, XMFLOAT3& target, XMFLOAT3& up)
{
	mPosition = pos;
	mLook = Vector3::Normalize(Vector3::Subtract(target, pos));
	mRight = Vector3::Normalize(Vector3::Cross(up, mLook));
	mUp = Vector3::Cross(mLook, mRight);

	mViewDirty = true;
}

XMFLOAT4X4 Camera::GetView() const
{
	assert(!mViewDirty && "Camera -> mView is not updated!!");
	return mView;
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
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mRight), angle);
	mUp = Vector3::TransformNormal(mUp, R);
	mLook = Vector3::TransformNormal(mLook, R);

	mViewDirty = true;
}

void Camera::RotateY(float angle)
{
	XMMATRIX R = XMMatrixRotationY(angle);
	mRight = Vector3::TransformNormal(mRight, R);
	mUp = Vector3::TransformNormal(mUp, R);
	mLook = Vector3::TransformNormal(mLook, R);

	mViewDirty = true;
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


/////////////////////////////////////////////////////////////////////////////
//
//TopViewCamera::TopViewCamera(float radius, float theta, float phi)
//	: Camera(), mRadius(radius), mTheta(theta), mPhi(phi)
//{
//	SetLens(0.25f * Math::PI, 1.0f, 1.0f, 1000.0f);
//	mViewDirty = true;
//}
//
//TopViewCamera::~TopViewCamera()
//{
//
//}
//
//void TopViewCamera::SetTarget(const XMFLOAT3& target)
//{
//	mTarget = target;
//	mViewDirty = true;
//}
//
//void TopViewCamera::RotateTheta(float dx)
//{
//	mTheta += dx;
//	mViewDirty = true;
//}
//
//void TopViewCamera::RotatePhi(float dy)
//{
//	mPhi += dy;
//	mViewDirty = true;
//}
//
//void TopViewCamera::UpdateViewMatrix()
//{
//	if (mViewDirty)
//	{
//		mPosition.x = mRadius * sinf(mPhi) * cosf(mTheta);
//		mPosition.z = mRadius * sinf(mPhi) * sinf(mTheta);
//		mPosition.y = mRadius * cosf(mPhi);
//
//		LookAt(mPosition, mTarget, mUp);
//		Camera::UpdateViewMatrix();
//	}
//}