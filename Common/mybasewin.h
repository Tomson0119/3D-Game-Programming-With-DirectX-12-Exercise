#pragma once

#include <Windows.h>

template <class T>
class BaseWindow
{
protected:

	BaseWindow() : m_hwnd(NULL) { }

	static LRESULT CALLBACK 
	WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		T* pThis = NULL;

		if (uMsg == WM_CREATE)
		{
			CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
			pThis = reinterpret_cast<T*>(cs->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
			pThis->m_hwnd = hwnd;
		}
		else
			pThis = reinterpret_cast<T*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (pThis)		
			return pThis->HandleMessage(uMsg, wParam, lParam);
		else
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	BOOL InitWindow(
		PCWSTR lpWndName,
		int mClientWidth, int mClientHeight)
	{
		WNDCLASS wc;
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = T::WindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = GetModuleHandle(NULL);
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		wc.lpszMenuName = 0;
		wc.lpszClassName = ClassName();

		if (!RegisterClass(&wc))
		{
			MessageBox(m_hwnd, L"Failed to register class.", 0, MB_OK);
			return FALSE;
		}

		RECT R = { 0,0, mClientWidth, mClientHeight };
		AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, FALSE);
		int width = R.right - R.left;
		int height = R.bottom - R.top;

		m_hwnd = CreateWindow(ClassName(), lpWndName,
			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			width, height, 0, 0, GetModuleHandle(0), this);

		if (!m_hwnd)
		{
			MessageBox(m_hwnd, L"Failed to create window.", 0, MB_OK);
			return FALSE;
		}

		ShowWindow(m_hwnd, SW_SHOW);
		UpdateWindow(m_hwnd); // WM_PAINT 메세지 처리가 지연 될 경우
							  // 즉시 처리하기 위해 UpdateWindow 를 호출

		return TRUE;
	}

protected:

	HWND m_hwnd;
	HWND Window() const { return m_hwnd; }

	virtual PCWSTR ClassName() const = 0;
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
};