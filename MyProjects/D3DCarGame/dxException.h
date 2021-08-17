#pragma once

class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName,
		const std::wstring& fileName, int lineNumber);

	std::wstring ToString() const;

	HRESULT mError = S_OK;
	std::wstring mFuncName;
	std::wstring mFileName;
	int mLineNumber = -1;
};