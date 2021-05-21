#include "../MyCommon/stdafx.h"
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
	HRESULT hr = D3DCompileFromFile(fileName.c_str(), defines, NULL,
		entry.c_str(), target.c_str(), compileFlags, NULL,
		&codeBlob, &errorBlob);

	if (errorBlob)
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	ThrowIfFailed(hr);

	return codeBlob;
}


///////////////////////////////////////////////////////////////////////
//


ColorShader::ColorShader()
	: Shader()
{
	Compile();
	BuildInputLayout();
}

void ColorShader::Compile()
{
	VS = CompileShader(mShaderPath, "VS", "vs_5_0");
	PS = CompileShader(mShaderPath, "PS", "ps_5_0");
}

void ColorShader::BuildInputLayout()
{
	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}
