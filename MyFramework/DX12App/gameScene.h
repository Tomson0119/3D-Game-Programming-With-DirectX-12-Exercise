#pragma once

#include "mesh.h"
#include "pipeline.h"
#include "player.h"
#include "shader.h"
#include "frameResource.h"


class GameScene
{
public:
	GameScene();
	GameScene(const GameScene& rhs) = delete;
	GameScene& operator=(const GameScene& rhs) = delete;
	virtual ~GameScene();

	void BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

	void Resize(float aspect);
	void UpdateConstants();
	void Update(const GameTimer& timer);
	void Draw(ID3D12GraphicsCommandList* cmdList, const GameTimer& timer);

	void OnProcessMouseDown(HWND hwnd, WPARAM buttonState);
	void OnProcessMouseUp(WPARAM buttonState);
	void OnProcessMouseMove(WPARAM buttonState) { }
	void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void OnKeyboardInput(const GameTimer& timer);
	void OnMouseInput(const GameTimer& timer);

	XMFLOAT4 GetFrameColor() const { return mFrameColor; }

private:
	void BuildRootSignature(ID3D12Device* device);
	void BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void BuildConstantBuffers(ID3D12Device* device);
	void BuildShadersAndPSOs(ID3D12Device* device);

private:
	XMFLOAT4 mFrameColor = (XMFLOAT4)Colors::LightSkyBlue;

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	static const UINT mFrameCount = 3;	

	ComPtr<ID3D12RootSignature> mRootSignature;
	
	std::unordered_map<std::string, std::unique_ptr<Mesh>> mMeshes;
	std::unordered_map<std::string, std::unique_ptr<Shader>> mShaders;
	std::unordered_map<std::string, std::unique_ptr<Pipeline>> mPipelines;

	std::vector<std::unique_ptr<GameObject>> mGameObjects;

	POINT mLastMousePos;
};