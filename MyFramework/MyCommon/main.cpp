#include "stdafx.h"
#include "d3dFramework.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, int ShowCmd)
{
	D3DFramework d3dApp;
	d3dApp.InitFramework();
	d3dApp.Run();
}