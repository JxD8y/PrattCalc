#pragma once
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <dxgi.h>
#include <d3d11.h>
#include <assert.h>
#include <iomanip>
#include <vector>
#include <ShellScalingApi.h>

using namespace std;

typedef void (*WindowPart)();
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Imeasy {
	extern int width;
	extern int height;
	void PrintErrorMessage(HRESULT hr);
	void StartRendering(string title, UINT height, UINT width);
	void AddRenderingObject(WindowPart wp);
	void RemoveRenderingObject(WindowPart wp);
	namespace {
		LRESULT WndProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp);
		int rh, rw = 0;
		bool render = true;
		vector<WindowPart> Objects;
		ID3D11Device* pdxDev = NULL;
		ID3D11DeviceContext* pdxDevCtx = NULL;
		ID3D11RenderTargetView* pRTV = NULL;
		IDXGISwapChain* pswCh = NULL;
		HWND hw;
		bool CreateDXDevice();
		bool CreateDxSwapChain(HWND hw);
		bool CreateDxRenderTargetView();
		HWND CreateDxWindow(string title,UINT height,UINT width);
		void ClearDxRenderingTarget();
	}
}
