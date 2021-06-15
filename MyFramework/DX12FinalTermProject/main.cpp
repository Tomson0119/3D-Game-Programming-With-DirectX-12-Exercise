#include "stdafx.h"
#include "framework.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int mCmdShow)
{
	try
	{
		GameFramework app(1000, 800);
		if (!app.InitFramework())
			return 0;		
		app.Run();
	}
	catch (DxException& error)
	{
		MessageBox(nullptr, error.ToString().c_str(), L"HR ERROR", MB_OK);
		return 0;
	}
}