#pragma once

#include <Windows.h>

template<class T>
class BaseWin
{
protected:
	HWND m_hwnd = NULL;

private:
	ATOM MyRegisterClass()
	{
		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = T::WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = GetModuleHandle(NULL);
		wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = ClassName();
		wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

		return RegisterClassEx(&wcex);
	}

protected:
	BaseWin() { }

	BOOL InitWindow(PCWSTR lpWndName, int clientWidth, int clientHeight)
	{
		if (!MyRegisterClass())
		{
			MessageBox(m_hwnd, L"Failed to register class.", 0, MB_OK);
			return FALSE;
		}

		RECT rc = { 0, 0, clientWidth, clientHeight };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

		m_hwnd = CreateWindow(ClassName(), lpWndName,
			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			rc.right - rc.left, rc.bottom - rc.top, 0, 0,
			GetModuleHandle(NULL), this);

		if (!m_hwnd)
		{
			MessageBox(m_hwnd, L"Failed to create window.", 0, MB_OK);
			return FALSE;
		}

		ShowWindow(m_hwnd, SW_SHOW);
		UpdateWindow(m_hwnd);

		return TRUE;
	}

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		T* pThis = NULL;

		if (uMsg == WM_CREATE) // Make new pointer of window
		{
			CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
			pThis = reinterpret_cast<T*>(cs->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
			pThis->m_hwnd = hwnd;
		}
		else
		{
			// Get current pointer of window 
			pThis = reinterpret_cast<T*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		if (pThis) // Call message processing function of this window
			return pThis->OnProcessMessage(uMsg, wParam, lParam);
		else
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}	

protected:
	HWND Window() const { return m_hwnd; }

	virtual PCWSTR ClassName() const = 0;
	virtual LRESULT OnProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
};