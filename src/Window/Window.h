#pragma once
#include <exception>
#include <Windows.h>
#include <string>
#include <wrl.h>

#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")

#include <memory>

#include "../../ext/ImGui/imgui.h"
#include "../../ext/ImGui/imgui_impl_dx11.h"
#include "../../ext/ImGui/imgui_impl_win32.h"
#include "../Font/Exo2.h"
#include "../Scanner/IAKAR.h"

struct Font
{
	Font(const unsigned char* data, size_t dataSize, float FontSize)
	{
		m_pFontData = std::make_unique<unsigned char[]>(dataSize);
		std::memcpy(m_pFontData.get(), data, dataSize);

		m_Config.FontDataOwnedByAtlas = false;
		auto& io = ImGui::GetIO();
		m_pFont = io.Fonts->AddFontFromMemoryTTF(m_pFontData.get(), static_cast<int>(dataSize), FontSize, &m_Config);
	}

	std::unique_ptr<unsigned char[]> m_pFontData = nullptr;
	ImFont* m_pFont = nullptr;
	ImFontConfig m_Config = {};
};

class Window
{
public:
	Window() = delete;
	~Window();
	explicit Window(std::wstring_view name, HINSTANCE instance, int Width, int Height) : m_WindowName(name), m_Width(Width), m_Height(Height), m_Instance(instance)
	{
		m_IAKAR.Update();
		InitWindow();
		m_ExoFont = std::make_unique<Font>(FontData::Exo, sizeof(FontData::Exo), 18.f);
		m_ExoFontBig = std::make_unique<Font>(FontData::Exo, sizeof(FontData::Exo), 24.f);
		embraceTheDarkness();
	}

	MSG& msg();
	void Render();


	static LRESULT CALLBACK  WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


private:
	void InitWindow(); 
	void InitImGui();
	void DrawImGui();
	void embraceTheDarkness();
	//Window
	HWND m_HWND = nullptr;
	MSG m_Msg = {};
	WNDCLASSEX m_wc = {};
	HINSTANCE m_Instance = nullptr;
	std::wstring m_ClassName = { L"IAKAR" };
	std::wstring m_WindowName = { L"" };
	int m_Width;
	int m_Height;

	//Dx11
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_pSwapChain = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pDeviceContext = nullptr;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pBackbuffer = nullptr;

	//ImGui
	std::unique_ptr<Font> m_ExoFont = nullptr;
	std::unique_ptr<Font> m_ExoFontBig = nullptr;


	//IAKAR
	IAKAR m_IAKAR;
};

