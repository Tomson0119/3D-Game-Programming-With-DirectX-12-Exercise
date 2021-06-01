#pragma once

class Shader
{
public:
	Shader();
	Shader(const Shader& rhs) = delete;
	Shader& operator=(const Shader& rhs) = delete;
	virtual ~Shader() { }

	virtual void Compile(const std::wstring& path) = 0;
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
};

class ColorShader : public Shader
{
public:
	ColorShader(const std::wstring& path);
	ColorShader(const ColorShader& rhs) = delete;
	ColorShader& operator=(const ColorShader& rhs) = delete;
	virtual ~ColorShader() { }

	virtual void Compile(const std::wstring& path) override;
	virtual void BuildInputLayout() override;
};