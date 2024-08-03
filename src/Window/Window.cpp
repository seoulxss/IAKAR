#include "Window.h"
#include <DirectXPackedVector.h>
#include <stdexcept>
#include <dwmapi.h>

#include "../../resource.h"

#pragma comment(lib, "dwmapi.lib")

Window::~Window()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    ::DestroyWindow(m_HWND);
    ::UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);
}

MSG& Window::msg()
{
    return m_Msg;
}

void Window::Render()
{
    using RGBA = float[4];

    // clear the back buffer to a deep blue
    m_pDeviceContext->ClearRenderTargetView(m_pBackbuffer.Get(), RGBA{ 0.0f, 0.2f, 0.4f, 1.0f });


    DrawImGui();

	m_pSwapChain->Present(1, 0); // VSync enabled

}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Window::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch (message)
    {
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, message, wParam, lParam);
}

void Window::InitWindow()
{
    try
    {
        // clear out the window class for use
        ZeroMemory(&m_wc, sizeof(WNDCLASSEX));

        // fill in the struct with the needed information
        m_wc.cbSize = sizeof(WNDCLASSEX);
        m_wc.style = CS_HREDRAW | CS_VREDRAW;
        m_wc.lpfnWndProc = &WindowProc;
        m_wc.hInstance = m_Instance;
        m_wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        m_wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
        m_wc.lpszClassName = m_ClassName.c_str();
        m_wc.hIcon = LoadIconW(m_Instance, MAKEINTRESOURCE(IDI_ICON1));
        m_wc.hIconSm = LoadIconW(m_Instance, MAKEINTRESOURCE(IDI_ICON1));

        // register the window class
        RegisterClassExW(&m_wc);

        RECT rect = { 0, 0, m_Width, m_Height };
        if (!AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, false))
            throw std::runtime_error("Could not adjust the window size!");

        int x = GetSystemMetrics(SM_CXFULLSCREEN);
        int y = GetSystemMetrics(SM_CYFULLSCREEN);



        // create the window and use the result as the handle
        m_HWND = CreateWindowExW(NULL,
            m_ClassName.c_str(),    // name of the window class
            m_WindowName.c_str(),   // title of the window
            WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,    // window style
            (x - (rect.right - rect.left)) / 2,    // x-position of the window
            (y - (rect.bottom - rect.top)) / 2,    // y-position of the window
            rect.right - rect.left,    // width of the window
            rect.bottom - rect.top,    // height of the window
            nullptr,    // we have no parent window, NULL
            nullptr,    // we aren't using menus, NULL
            m_Instance,    // application handle
            nullptr);    // used with multiple windows, NULL

        if (!m_HWND)
            throw std::runtime_error("Couldn't create the window!");

        // display the window on the screen
        if (ShowWindow(m_HWND, SW_SHOW))
            throw std::runtime_error("Could not show window!");

        UpdateWindow(m_HWND); // Update the window
        
        /* ---------------------------------------------------------------------------------- */

        DXGI_SWAP_CHAIN_DESC scd;

        // clear out the struct for use
        ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

        // fill the swap chain description struct
        scd.BufferCount = 1;                                    // one back buffer
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
        scd.OutputWindow = m_HWND;                                // the window to be used
        scd.SampleDesc.Count = 4;                               // how many multisamples
        scd.Windowed = TRUE;                                    // windowed/full-screen mode

        // create a device, device context and swap chain using the information in the scd struct
        if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            NULL,
            NULL,
            D3D11_SDK_VERSION,
            &scd,
            &m_pSwapChain,
            &m_pDevice,
            NULL,
            &m_pDeviceContext)))
            throw std::runtime_error("Failed to create the DXD11 Window!");

        // get the address of the back buffer
        Microsoft::WRL::ComPtr<ID3D11Texture2D> ptextureBuffer;
        auto hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(ptextureBuffer.GetAddressOf()));
        if (FAILED(hr))
            throw std::runtime_error("Failed to get the swap chain back buffer!");

        // Use the back buffer address to create the render target
        hr = m_pDevice->CreateRenderTargetView(ptextureBuffer.Get(), nullptr, m_pBackbuffer.ReleaseAndGetAddressOf());
        if (FAILED(hr))
            throw std::runtime_error("Failed to create render target view!");

        // Set the render target as the back buffer
        m_pDeviceContext->OMSetRenderTargets(1, m_pBackbuffer.GetAddressOf(), nullptr);

        // Set the viewport
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = static_cast<float>(rect.right - rect.left);
        viewport.Height = static_cast<float>(rect.bottom - rect.top);

        m_pDeviceContext->RSSetViewports(1, &viewport);

        /* ------------------------------------------------------------------------------------------------------------------------*/

        BOOL USE_DARK_MODE = true;
        BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
            m_HWND, DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE,
            &USE_DARK_MODE, sizeof(USE_DARK_MODE)));

        UpdateWindow(m_HWND);

        InitImGui();
    }

    catch (std::exception& e)
    {
        MessageBoxA(nullptr, e.what(), "Error!", MB_OKCANCEL | MB_ICONERROR);
        exit(-1);
    }



}

void Window::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.Fonts->AddFontDefault();

    ImGui::StyleColorsDark();
    if (!ImGui_ImplWin32_Init(m_HWND))
        throw std::runtime_error("Failed to load ImGui");

    if (!ImGui_ImplDX11_Init(m_pDevice.Get(), m_pDeviceContext.Get()))
        throw std::runtime_error("Failed to connect ImGui with Dx11");
}

void Window::DrawImGui()
{
    static int selectedIndex = 0;
    static int selectedDetected = 0;
    static bool once = false;
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (!once)
    {
        ImGui::SetNextWindowPos({ -1,-1 });
        ImGui::SetNextWindowSize({ static_cast<float>(m_Width) + 2, static_cast<float>(m_Height) + 2 });
        once = true;
    }

    ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGui::PushFont(m_ExoFont->m_pFont);
        ImGui::PushFont(m_ExoFontBig->m_pFont);

        auto size = ImGui::CalcTextSize("All loaded drivers");
        ImGui::SetCursorPosX((400 - size.x) / 2);
        ImGui::Text("All loaded drivers");
        ImGui::SameLine();

        auto secondSize = ImGui::CalcTextSize("Anti-Cheats detected");
        ImGui::SetCursorPosX((800 - size.x) / 1.25f);
        ImGui::Text("Anti-Cheats detected");

        ImGui::PopFont();
        if (ImGui::BeginListBox("###box", {400, 480})) // Set size for ListBox
        { 
            int index = 0;
            for (const auto& driver : m_IAKAR.GetAllDriversW())
            {
                std::string driverStr(driver.begin(), driver.end());
                bool isSelected = (index == selectedIndex);

                if (ImGui::Selectable(driverStr.c_str(), isSelected))
                {
                    selectedIndex = index; 
                }

                index++;
            }

            ImGui::EndListBox();
        }
        ImGui::SameLine();
        if (ImGui::BeginListBox("###Detected", { 400, 480 })) // Set size for ListBox
        {
            int index = 0;
            for (const auto& driver : m_IAKAR.DetectedAC())
            {
                std::string driverStr(driver.begin(), driver.end());
                bool isSelected = (index == selectedDetected);

                if (ImGui::Selectable(driverStr.c_str(), isSelected))
                {
                    selectedDetected = index;
                }

                index++;
            }

            ImGui::EndListBox();
        }
        static std::wstring detectedDriverPath = L"";
        static std::wstring driverPath = L"";
        if(ImGui::Button("Get Path", {80, 40}))
        {
            driverPath.clear();
            driverPath = m_IAKAR.GetDriverPath(m_IAKAR.GetAllDriversW()[selectedIndex]);
            for (auto& entry : driverPath)
            {
                if (entry == '\0')
                    driverPath.pop_back();
            }

            detectedDriverPath.clear();
            detectedDriverPath = m_IAKAR.GetDriverPath(m_IAKAR.GetDetectedACW().at(selectedDetected));

        }
        ImGui::SameLine();
        if (ImGui::Button("Update", {80, 40}))
        {
            m_IAKAR.Update();
        }

        ImGui::SameLine();
        if (ImGui::Button("Dump all drivers", { 150, 40 }))
        {
            m_IAKAR.DumpAll();
        }

        ImGui::SameLine();
        if (ImGui::Button("Dump all detected", { 150, 40 }))
        {
            m_IAKAR.DumpDetectd();
        }

        ImGui::Text(std::string(driverPath.begin(), driverPath.end()).c_str());
        ImGui::SameLine();
        ImGui::Text("     ||     ");
        ImGui::SameLine();
        ImGui::TextColored({ 255, 0, 0, 1 }, std::string(detectedDriverPath.begin(), detectedDriverPath.end()).c_str());
        ImGui::PopFont();
    }
    ImGui::End();

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Window::embraceTheDarkness()
{
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
    colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);

    colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(8.00f, 8.00f);
    style.FramePadding = ImVec2(5.00f, 2.00f);
    style.CellPadding = ImVec2(6.00f, 6.00f);
    style.ItemSpacing = ImVec2(6.00f, 6.00f);
    style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
    style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
    style.IndentSpacing = 25;
    style.ScrollbarSize = 15;
    style.GrabMinSize = 10;
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 1;
    style.TabBorderSize = 1;
    style.WindowRounding = 7;
    style.ChildRounding = 4;
    style.FrameRounding = 3;
    style.PopupRounding = 4;
    style.ScrollbarRounding = 9;
    style.GrabRounding = 3;
    style.LogSliderDeadzone = 4;
    style.TabRounding = 4;
}
