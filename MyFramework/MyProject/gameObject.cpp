#include "../MyCommon/stdafx.h"
#include "gameObject.h"

GameObject::GameObject()
{
}

GameObject::~GameObject()
{
}

void GameObject::Update()
{
}

void GameObject::Draw()
{
}

void GameObject::SetPosition(float x, float y, float z)
{
}

void GameObject::SetPosition(XMFLOAT3 pos)
{
}

void GameObject::Move(float dx, float dy, float dz)
{
}

void GameObject::MoveStrafe(float dist)
{
}

void GameObject::MoveUp(float dist)
{
}

void GameObject::MoveForward(float fdist)
{
}

void GameObject::Rotate(float pitch, float yaw, float roll)
{
}

void GameObject::Rotate(XMFLOAT3 axis, float angle)
{
}

void GameObject::Scale(float xScale, float yScale, float zScale)
{
}

void GameObject::Scale(float scale)
{
}

ColoredObject::ColoredObject()
{
}

ColoredObject::~ColoredObject()
{
}

void ColoredObject::SetColor(XMFLOAT4 color)
{
}
