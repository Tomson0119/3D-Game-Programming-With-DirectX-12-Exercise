#include "stdafx.h"
#include "shader.h"


Shader::Shader()
{
}

ComPtr<ID3DBlob> Shader::CompileShader(
	const std::wstring& fileName,
	const std::string& entry,
	const std::string& target,
	const D3D_SHADER_MACRO* defines)
{
	UINT compileFlags = 0;
#ifdef _DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> codeBlob;
	ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3DCompileFromFile(fileName.c_str(), defines,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entry.c_str(), target.c_str(), compileFlags, NULL,
		&codeBlob, &errorBlob);

	if (errorBlob)
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	ThrowIfFailed(hr);

	return codeBlob;
}


///////////////////////////////////////////////////////////////////////
//
DefaultShader::DefaultShader(const std::wstring& path)
	: Shader()
{
	Compile(path);
	BuildInputLayout();
}

void DefaultShader::Compile(const std::wstring& path)
{
	VS = Shader::CompileShader(path, "VS", "vs_5_0");
	PS = Shader::CompileShader(path, "PS", "ps_5_0");
}

void DefaultShader::BuildInputLayout()
{
	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}


///////////////////////////////////////////////////////////////////////////////////////////
//
TerrainShader::TerrainShader(const std::wstring& path)
	: Shader()
{
	Compile(path);
	BuildInputLayout();
}

void TerrainShader::Compile(const std::wstring& path)
{
	VS = Shader::CompileShader(path, "VS", "vs_5_1");
	PS = Shader::CompileShader(path, "PS", "ps_5_1");
	DS = Shader::CompileShader(path, "DS", "ds_5_1");
	HS = Shader::CompileShader(path, "HS", "hs_5_1");
}

void TerrainShader::BuildInputLayout()
{
	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 32,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}


///////////////////////////////////////////////////////////////////////////////////////////
//
BillboardShader::BillboardShader(const std::wstring& path)
	: Shader()
{
	Compile(path);
	BuildInputLayout();
}

void BillboardShader::Compile(const std::wstring& path)
{
	VS = Shader::CompileShader(path, "VS", "vs_5_1");
	GS = Shader::CompileShader(path, "GS", "gs_5_1");
	PS = Shader::CompileShader(path, "PS", "ps_5_1");
}

void BillboardShader::BuildInputLayout()
{
	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}


///////////////////////////////////////////////////////////////////////////////////////////
//
CubeMapShader::CubeMapShader(const std::wstring& path)
	: Shader()
{
	Compile(path);
	BuildInputLayout();
}

void CubeMapShader::Compile(const std::wstring& path)
{
	VS = Shader::CompileShader(path, "VS", "vs_5_1");
	PS = Shader::CompileShader(path, "PS", "ps_5_1");
}

void CubeMapShader::BuildInputLayout()
{
	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}
