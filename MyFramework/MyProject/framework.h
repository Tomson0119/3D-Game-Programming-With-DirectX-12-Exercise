#pragma once

#include "../MyCommon/d3dFramework.h"

class GameFramework : public D3DFramework
{
public:
	GameFramework();
	GameFramework(const GameFramework& rhs) = delete;
	GameFramework& operator=(const GameFramework& rhs) = delete;
	virtual ~GameFramework();

	virtual bool InitFramework() override;

private:
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndLayouts();
	void BuildMeshObjects();
	void BuildPSO();

private:
	virtual void Update(const GameTimer& timer) override;
	virtual void Draw(const GameTimer& timer) override;

};