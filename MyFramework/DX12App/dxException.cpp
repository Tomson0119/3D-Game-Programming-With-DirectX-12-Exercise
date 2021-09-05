#include "stdafx.h"
#include "dxException.h"

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& fileName, int lineNumber)
	: mError(hr), mFuncName(functionName), mFileName(fileName), mLineNumber(lineNumber)
{
}

std::wstring DxException::ToString() const
{
	_com_error error(mError);
	std::wstring msg = error.ErrorMessage();
	return mFuncName + L" failed in " + mFileName + L";  line "
		+ std::to_wstring(mLineNumber) + L";  Error : " + msg;
}
