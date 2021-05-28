#include "../MyCommon/stdafx.h"
#include "shader.h"


Shader::Shader()
{	
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
