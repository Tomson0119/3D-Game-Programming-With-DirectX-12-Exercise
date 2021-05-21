#pragma once

class Shader
{
public:
	Shader();
	Shader(const Shader& rhs) = delete;
	Shader& operator=(const Shader& rhs) = delete;
	virtual ~Shader() { }

	virtual void Compile() = 0;
	virtual void BuildInputLayout() = 0;

	std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout() const { return mInputLayout; }
	
	ID3DBlob* GetVS() const { return VS.Get(); }
	ID3DBlob* GetGS() const { return GS.Get(); }
	ID3DBlob* GetPS() const { return PS.Get(); }

protected:
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	ComPtr<ID3DBlob> VS = nullptr;
	ComPtr<ID3DBlob> GS = nullptr;
	ComPtr<ID3DBlob> PS = nullptr;

	ComPtr<ID3DBlob> CompileShader(
		const std::wstring& fileName,
		const std::string& entry,
		const std::string& target,
		const D3D_SHADER_MACRO* defines = nullptr);
};

class ColorShader : public Shader
{
public:
	ColorShader();
	ColorShader(const ColorShader& rhs) = delete;
	ColorShader& operator=(const ColorShader& rhs) = delete;
	virtual ~ColorShader() { }

	virtual void Compile() override;
	virtual void BuildInputLayout() override;

private:
	std::wstring mShaderPath = L"Shaders\\color.hlsl";
};