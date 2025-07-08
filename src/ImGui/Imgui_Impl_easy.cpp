#include "ImGui_Impl_easy.h"
#include <algorithm>
#include <iostream>

using namespace std;

namespace Imeasy {
	bool TerminateOnClose = true;

	void PrintErrorMessage(HRESULT hr) {
		LPVOID lpMsgBuf;
		DWORD dw = FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&lpMsgBuf,
			0, nullptr);

		if (dw) {
			std::wcerr << L"Error: " << (LPWSTR)lpMsgBuf << std::endl;
			LocalFree(lpMsgBuf);
		}
		else {
			std::cerr << "Unknown error occurred." << std::endl;
		}
	}

	void AddRenderingObject(WindowPart wp) {
		Imeasy::Objects.push_back(wp);
	}

	void RemoveRenderingObject(WindowPart wp) {
		Imeasy::Objects.erase(std::remove(Imeasy::Objects.begin(), Imeasy::Objects.end(), wp));
	}

	namespace {
		LRESULT WndProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp) {
			if (msg == WM_SIZE) {
				if (wp == SIZE_MINIMIZED)
					return 0;
				Imeasy::rw = (UINT)LOWORD(lp);
				Imeasy::rh = (UINT)HIWORD(lp);
				return 0;
			}
			if (msg == WM_CLOSE)
				if (Imeasy::TerminateOnClose) {
					HANDLE hProc = GetCurrentProcess();
					TerminateProcess(hProc, 0);
				}
			if (ImGui_ImplWin32_WndProcHandler(hw, msg, wp, lp))
				return true;
			return DefWindowProcA(hw, msg, wp, lp);
		}
		bool CreateDXDevice() {
			HRESULT hr = D3D11CreateDevice(
				NULL,
				D3D_DRIVER_TYPE_HARDWARE,
				NULL,
				0,
				NULL,
				0,
				D3D11_SDK_VERSION,
				&Imeasy::pdxDev,
				NULL,
				&Imeasy::pdxDevCtx
			);
			if (hr != S_OK)
			{
				cout << "Couldn't create DX device" << endl;
				PrintErrorMessage(hr);
				return false;
			}
			return true;
		}

		bool CreateDxSwapChain(HWND hw) {
			IDXGIFactory* factory = NULL;
			HRESULT hr = CreateDXGIFactory(__uuidof(factory), (void**)(&factory));
			if (hr != S_OK) {
				cout << "Cannot create DX factory" << endl;
				PrintErrorMessage(hr);
				return false;
			}
			DXGI_SWAP_CHAIN_DESC pdDx;
			ZeroMemory(&pdDx, sizeof(pdDx));
			pdDx.BufferCount = 2;
			pdDx.BufferDesc.Width = 0;
			pdDx.BufferDesc.Height = 0;
			pdDx.BufferDesc.RefreshRate.Numerator = 120;
			pdDx.BufferDesc.RefreshRate.Denominator = 1;
			pdDx.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			pdDx.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			pdDx.SampleDesc.Count = 1;
			pdDx.SampleDesc.Quality = 0;
			pdDx.OutputWindow = hw;
			pdDx.Windowed = TRUE;
			pdDx.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			pdDx.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

			hr = factory->CreateSwapChain(Imeasy::pdxDev, &pdDx, &Imeasy::pswCh);
			if (hr != S_OK) {
				cout << "Cannot create the swap chain" << std::hex << hr << endl;
				PrintErrorMessage(hr);
				return false;
			}

			factory->Release();
			return true;
		}

		bool CreateDxRenderTargetView() {
			ID3D11Texture2D* bBuffer = NULL;
			HRESULT hr = Imeasy::pswCh->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(&bBuffer));
			if (hr != S_OK) {
				cout << "Cannot get the back buffer from swap chain" << endl;
				PrintErrorMessage(hr);
				return false;
			}

			hr = Imeasy::pdxDev->CreateRenderTargetView(bBuffer, NULL, &Imeasy::pRTV);
			if (hr != S_OK) {
				cout << "Cannot create RTV" << endl;
				PrintErrorMessage(hr);
				return false;
			}
			bBuffer->Release();
			return true;
		}

		HWND CreateDxWindow(string title,UINT height,UINT width) {
			WNDCLASSA wcx = {};
			wcx.hInstance = GetModuleHandleA(NULL);
			wcx.lpszClassName = title.c_str();
			wcx.lpfnWndProc = WndProc;
			RegisterClassA(&wcx);
			HWND hw = CreateWindowExA(0, wcx.lpszClassName, title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, wcx.hInstance, NULL);
			return hw;
		}

		void ClearDxRenderingTarget() {
			pRTV->Release();
			pRTV = NULL;
		}
	}
	
	void StartRendering(string title,UINT height,UINT width) {
		if (!Imeasy::CreateDXDevice())
		{
			throw exception("failed to create dx dev"); 
		}
		HWND hw = CreateDxWindow(title.c_str(), height, width);
		if (hw == NULL)
		{
			throw exception("failed to create window");
		}
		ShowWindow(hw, SW_SHOW);
		if (!CreateDxSwapChain(hw)) {
			throw exception("failed to create swap chain");
		}
		if (!CreateDxRenderTargetView()) {
			throw exception("failed to create dx render target view");
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		ImGui::StyleColorsDark();
		ImGui_ImplWin32_Init(hw);
		ImGui_ImplDX11_Init(Imeasy::pdxDev, Imeasy::pdxDevCtx);
		bool run = true;
		const float cColor[4] = { 0,0,0,0 };
		while (run) {
			MSG msg;
			while (PeekMessageA(&msg, Imeasy::hw, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
				if (msg.message == WM_CLOSE)
				{
					run = false;
				}
			}

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			for (auto f : Imeasy::Objects) {
				f();
			}
			if (Imeasy::rw != 0 && Imeasy::rh != 0)
			{
				Imeasy::ClearDxRenderingTarget();
				Imeasy::pswCh->ResizeBuffers(0, Imeasy::rw, Imeasy::rh, DXGI_FORMAT_UNKNOWN, 0);
				Imeasy::rw = Imeasy::rh = 0;
				CreateDxRenderTargetView();
			}

			ImGui::Render();
			Imeasy::pdxDevCtx->OMSetRenderTargets(1, &Imeasy::pRTV, NULL);
			Imeasy::pdxDevCtx->ClearRenderTargetView(Imeasy::pRTV, cColor);
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			Imeasy::pswCh->Present(1, 0);

		}
	}
}