#pragma once

#include "gameObject.h"

enum class STATE : int {
	NORMAL = 0,
	INVINCIBLE,
	GIANT
};

class Player : public GameObject
{
public:
	Player(int offset, Mesh* mesh);
	Player(const Player& rhs) = delete;
	Player& operator=(const Player& rhs) = delete;
	virtual ~Player();

	virtual void SetMaterial(XMFLOAT4 color, XMFLOAT3 frenel, float roughness);
	void Update(float elapsedTime);

	void SetScalingSpeed(float speed) { mScalingSpeed = speed; }
	
	void AdjustCoordinate(float elapsedTime);
	void UpdateInvincibleState(float elapsedTime);
	void UpdateGiantState(float elapsedTime);

	void MakeBigger() { mCurrState = STATE::GIANT; }
	void MakeInvincible() { mCurrState = STATE::INVINCIBLE; }

	STATE GetState() const { return mCurrState; }

private:
	XMFLOAT3 mScaledSize = { 1.0f,1.0f,1.0f };
	float mScalingSpeed = 0.0f;

	XMFLOAT4 mOriginalColor = {};
	XMFLOAT4 mColor = {};
	XMFLOAT4 mIvcColor = (XMFLOAT4)Colors::Gray;

	const float mRotateFriction = 800.0f;

	STATE mCurrState = STATE::NORMAL;
};