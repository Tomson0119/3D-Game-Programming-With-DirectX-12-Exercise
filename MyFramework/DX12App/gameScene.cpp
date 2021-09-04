#include "common.h"
#include "gameScene.h"
#include "gameObject.h"
#include "mesh.h"
#include "shader.h"
#include "pipeline.h"

using namespace std;

GameScene::GameScene()
{
}

GameScene::~GameScene()
{
}

void GameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	BuildRootSignature(device);
	BuildShadersAndPSOs(device);
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

void GameScene::OnKeyboardInput(const GameTimer& timer)
{
}

void GameScene::OnMouseInput(const GameTimer& timer)
{
}

void GameScene::BuildRootSignature(ID3D12Device* device)
{
	D3D12_DESCRIPTOR_RANGE descriptorRange[1];
	descriptorRange[0] = Extension::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	D3D12_ROOT_PARAMETER rootParameter[4];
	rootParameter[0] = Extension::DescriptorTable(1, &descriptorRange[0], D3D12_SHADER_VISIBILITY_PIXEL);	   // Texture
	rootParameter[1] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 0, D3D12_SHADER_VISIBILITY_ALL);   // Camera
	rootParameter[2] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 1, D3D12_SHADER_VISIBILITY_PIXEL); // Light
	rootParameter[3] = Extension::Descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, 2, D3D12_SHADER_VISIBILITY_ALL);   // Object

	D3D12_STATIC_SAMPLER_DESC samplerDesc = Extension::SamplerDesc(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = Extension::RootSignatureDesc(
		_countof(rootParameter), rootParameter, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	ThrowIfFailed(D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		rootSigBlob.GetAddressOf(),
		errorBlob.GetAddressOf()));

	ThrowIfFailed(device->CreateRootSignature(
		0,
		rootSigBlob->GetBufferPointer(),
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
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
