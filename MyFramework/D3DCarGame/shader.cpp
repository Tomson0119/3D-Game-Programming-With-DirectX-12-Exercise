#include "stdafx.h"
#include "shader.h"


Shader::Shader()
{	
}


///////////////////////////////////////////////////////////////////////
//
ColorShader::ColorShader(const std::wstring& path)
	: Shader()
{
	Compile(path);
	BuildInputLayout();
}

void ColorShader::Compile(const std::wstring& path)
{
	VS = CompileShader(path, "VS", "vs_5_0");
	PS = CompileShader(path, "PS", "ps_5_0");
}

void ColorShader::BuildInputLayout()
{
	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}
