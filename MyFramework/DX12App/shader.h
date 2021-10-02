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

public:
	static ComPtr<ID3DBlob> CompileShader(
		const std::wstring& fileName,
		const std::string& entry,
		const std::string& target,
		const D3D_SHADER_MACRO* defines = nullptr);

protected:
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	ComPtr<ID3DBlob> VS = nullptr;
	ComPtr<ID3DBlob> GS = nullptr;
	ComPtr<ID3DBlob> PS = nullptr;
};

///////////////////////////////////////////////////////////////////////////////////////////
//
class DefaultShader : public Shader
{
public:
	DefaultShader(const std::wstring& path);
	DefaultShader(const DefaultShader& rhs) = delete;
	DefaultShader& operator=(const DefaultShader& rhs) = delete;
	virtual ~DefaultShader() { }

	virtual void Compile(const std::wstring& path) override;
	virtual void BuildInputLayout() override;
};

///////////////////////////////////////////////////////////////////////////////////////////
//
class TerrainShader : public Shader
{
public:
	TerrainShader(const std::wstring& path);
	TerrainShader(const TerrainShader& rhs) = delete;
	TerrainShader& operator=(const TerrainShader& rhs) = delete;
	virtual ~TerrainShader() { }

	virtual void Compile(const std::wstring& path) override;
	virtual void BuildInputLayout() override;
};