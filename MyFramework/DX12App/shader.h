#pragma once

class Shader
{
public:
	Shader();
	virtual ~Shader() { }

	virtual void Compile(const std::wstring& path) = 0;
	virtual void BuildInputLayout() = 0;

	std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout() const { return mInputLayout; }
	
	ID3DBlob* GetVS() const { return VS.Get(); }
	ID3DBlob* GetGS() const { return GS.Get(); }
	ID3DBlob* GetPS() const { return PS.Get(); }
	ID3DBlob* GetDS() const { return DS.Get(); }
	ID3DBlob* GetHS() const { return HS.Get(); }

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
	ComPtr<ID3DBlob> DS = nullptr;
	ComPtr<ID3DBlob> HS = nullptr;
};

///////////////////////////////////////////////////////////////////////////////////////////
//
class DefaultShader : public Shader
{
public:
	DefaultShader(const std::wstring& path);
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
	virtual ~TerrainShader() { }

	virtual void Compile(const std::wstring& path) override;
	virtual void BuildInputLayout() override;
};

///////////////////////////////////////////////////////////////////////////////////////////
//
class BillboardShader : public Shader
{
public:
	BillboardShader(const std::wstring& path, bool soActive=false);
	virtual ~BillboardShader() { }

	virtual void Compile(const std::wstring& path) override;
	virtual void BuildInputLayout() override;

private:
	bool mSOActive;
};

///////////////////////////////////////////////////////////////////////////////////////////
//
class CubeMapShader : public Shader
{
public:
	CubeMapShader(const std::wstring& path);
	virtual ~CubeMapShader() { }

	virtual void Compile(const std::wstring& path) override;
	virtual void BuildInputLayout() override;
};

///////////////////////////////////////////////////////////////////////////////////////////
//
class ComputeShader : public Shader
{
public:
	ComputeShader(const std::wstring& path);
	virtual ~ComputeShader() { }

	virtual void Compile(const std::wstring& path) override;
	virtual void BuildInputLayout() override { }

	ID3DBlob* GetCS() const { return CS.Get(); }

private:
	ComPtr<ID3DBlob> CS;
};