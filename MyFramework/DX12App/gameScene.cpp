#include "common.h"
#include "gameScene.h"

GameScene::GameScene()
{

}

GameScene::~GameScene()
{
}

void GameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
}

void GameScene::Resize(float aspect)
{
}

void GameScene::UpdateConstants()
{
}

void GameScene::Update(const GameTimer& timer)
{
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList, const GameTimer& timer)
{
}

void GameScene::OnProcessMouseDown(HWND hwnd, WPARAM buttonState)
{
}

void GameScene::OnProcessMouseUp(WPARAM buttonState)
{	
}

void GameScene::OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
}

void GameScene::OnKeyboardInput(const GameTimer& timer)
{
}

void GameScene::OnMouseInput(const GameTimer& timer)
{
}

void GameScene::BuildRootSignature(ID3D12Device* device)
{
}

void GameScene::BuildGameObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
}

void GameScene::BuildConstantBuffers(ID3D12Device* device)
{
}

void GameScene::BuildShadersAndPSOs(ID3D12Device* device)
{
}
