#include "../MyCommon/stdafx.h"
#include "player.h"

Player::Player(int offset, Mesh* mesh)
	: GameObject(offset, mesh)
{
}

Player::~Player()
{
}

void Player::SetMaterial(XMFLOAT4 color, XMFLOAT3 frenel, float roughness)
{
	GameObject::SetMaterial(color, frenel, roughness);
	mOriginalColor = color;
	mColor = color;
}

void Player::Update(float elapsedTime)
{
	GameObject::Update(elapsedTime);

	mMaterial.Color = mColor;
	AdjustCoordinate(elapsedTime);
	UpdateInvincibleState(elapsedTime);
	UpdateGiantState(elapsedTime);
}

void Player::AdjustCoordinate(float elapsedTime)
{
	XMFLOAT3 look(0.0f, 0.0f, 1.0f);

	float angle = acos(Vector3::Dot(look, mLook));

	// 시계 혹은 반시계 방향인지 확인한다.
	XMFLOAT3 up = Vector3::Cross(look, mLook);
	if (up.y > 0.0f) // Clockwised
		angle *= -1.0f;

	Rotate(0.0f, angle * elapsedTime * mRotateFriction, 0.0f);
}

void Player::UpdateInvincibleState(float elapsedTime)
{
	if (mCurrState != STATE::INVINCIBLE)
		return;

	static float totalTime = 0.0f;
	static float colorChangeTime = 0.0f;

	totalTime += elapsedTime;
	colorChangeTime += elapsedTime;

	if (totalTime > 2.0f)
	{
		mColor = mOriginalColor;
		totalTime = 0.0f;
		mCurrState = STATE::NORMAL;
	}
	else if (colorChangeTime > 0.3f)
	{
		mColor = (Vector4::Equal(mColor, mOriginalColor)) ? mIvcColor : mOriginalColor;
		colorChangeTime = 0.0f;
	}
}

void Player::UpdateGiantState(float elapsedTime)
{
	if (mCurrState != STATE::GIANT)
		return;

	// 차가 무한히 커지거나 작아지는 것을 방지하기 하기 위해
	// 최소 크기와 최대 크기를 설정해놓는다.
	mScaledSize = Vector3::ClampFloat3(
		mScaledSize, XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.8f, 1.8f, 1.8f));

	if (Vector4::Equal(mColor, mIvcColor))
		mColor = mOriginalColor;

	// 축소하는 과정에서 객체의 크기가 1.0으로 맞춰지면 Giant 상태를 해제한다.
	if (mScalingSpeed < 0.0f && Vector3::Equal(mScaledSize, XMFLOAT3(1.0f, 1.0f, 1.0f)))
	{
		mCurrState = STATE::NORMAL;
		mScalingSpeed = 0.0f;
		return;
	}

	static float totalTime = 0.0f;
	totalTime += elapsedTime;

	if (totalTime > 5.0f)
	{
		mScalingSpeed *= -1.0f;
		totalTime = 0.0f;
	}
}
