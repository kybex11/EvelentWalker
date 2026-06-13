// evw_gui — Dear ImGui front-end (Win32 + Direct3D 11) for the EvelentWalker
// C++ core. DX11 matches CodeWalker's renderer so the original HLSL shaders can
// be reused for the future 3D viewport (rendered to a D3D11 texture and shown
// in ImGui via ImTextureID = ShaderResourceView).
//
// Build: cmake -S . -B build -DEVW_BUILD_GUI=ON && cmake --build build --target evw_gui
#include <d3d11.h>
#include <tchar.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdio>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "evw/app/explorer.h"
#include "evw/app/render_mesh.h"
#include "evw/gamefiles/drawable.h"
#include "evw/gamefiles/resource_data.h"
#include "viewport.h"

// ---- D3D11 device/swapchain state ----
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer)
    {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}
static void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

static bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags, levels, 2,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (hr != S_OK) return false;
    CreateRenderTarget();
    return true;
}
static void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

// ---- Explorer UI state ----
namespace
{
    evw::app::ExplorerModel g_model;
    char g_folder[1024] = "";
    char g_search[256] = "";
    std::vector<std::string> g_results;
    evw::app::FilePreview g_preview;
    std::string g_status = "Enter a GTA V folder path and click Scan.";

    evw::gui::Viewport g_viewport;
    bool g_viewportReady = false;
    bool g_hasModel = false;
    float g_yaw = 0.7f, g_pitch = 0.4f, g_distScale = 1.0f;

    void onSelect(const std::string& path)
    {
        g_preview = g_model.openFile(path);
        g_hasModel = false;
        if (g_viewportReady && g_preview.kind == evw::app::PreviewKind::Drawable)
        {
            auto rmodel = g_model.buildDrawableRenderModel(path);
            g_viewport.setModel(rmodel);
            g_hasModel = g_viewport.hasGeometry();
            g_yaw = 0.7f; g_pitch = 0.4f; g_distScale = 1.0f;
        }
    }

    void refreshSearch()
    {
        if (g_model.isInited()) g_results = g_model.search(g_search, 500);
    }

    void drawUI()
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("EvelentWalker", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Text("EvelentWalker (C++ / ImGui + D3D11)  -  RPF Explorer");
        ImGui::Separator();

        ImGui::SetNextItemWidth(700);
        ImGui::InputText("##folder", g_folder, sizeof(g_folder));
        ImGui::SameLine();
        if (ImGui::Button("Scan"))
        {
            try
            {
                g_model.init(g_folder);
                char buf[128];
                std::snprintf(buf, sizeof(buf), "Indexed %zu archives, %zu entries.",
                              g_model.rpfCount(), g_model.entryCount());
                g_status = buf;
                refreshSearch();
            }
            catch (const std::exception& ex)
            {
                g_status = std::string("Scan failed: ") + ex.what();
            }
        }
        ImGui::TextUnformatted(g_status.c_str());
        ImGui::Separator();

        ImGui::SetNextItemWidth(400);
        if (ImGui::InputText("Search", g_search, sizeof(g_search))) refreshSearch();

        ImGui::Columns(2, "split");

        ImGui::BeginChild("entries");
        for (const auto& path : g_results)
            if (ImGui::Selectable(path.c_str(), g_preview.path == path))
                onSelect(path);
        ImGui::EndChild();
        ImGui::NextColumn();

        ImGui::BeginChild("preview");
        if (!g_preview.path.empty())
        {
            ImGui::TextWrapped("%s", g_preview.path.c_str());
            ImGui::Separator();
            ImGui::TextWrapped("%s", g_preview.summary.c_str());
            ImGui::Text("Size: %zu bytes", g_preview.dataSize);
            ImGui::Separator();

            if (g_hasModel)
            {
                ImGui::Text("3D model: %zu triangles", g_viewport.triangleCount());
                ImVec2 avail = ImGui::GetContentRegionAvail();
                int vw = (int)avail.x, vh = (int)(avail.y - 8);
                if (vw > 16 && vh > 16)
                {
                    ID3D11ShaderResourceView* srv = g_viewport.render(vw, vh, g_yaw, g_pitch, g_distScale);
                    if (srv)
                    {
                        ImVec2 imgPos = ImGui::GetCursorScreenPos();
                        ImGui::Image((ImTextureID)srv, ImVec2((float)vw, (float)vh));
                        // Orbit with left-drag over the image, zoom with wheel.
                        if (ImGui::IsItemHovered())
                        {
                            ImGuiIO& io = ImGui::GetIO();
                            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                            {
                                g_yaw -= io.MouseDelta.x * 0.01f;
                                g_pitch += io.MouseDelta.y * 0.01f;
                                if (g_pitch > 1.5f) g_pitch = 1.5f;
                                if (g_pitch < -1.5f) g_pitch = -1.5f;
                            }
                            if (io.MouseWheel != 0.0f)
                            {
                                g_distScale *= (io.MouseWheel > 0 ? 0.9f : 1.1f);
                                if (g_distScale < 0.05f) g_distScale = 0.05f;
                                if (g_distScale > 20.0f) g_distScale = 20.0f;
                            }
                            (void)imgPos;
                        }
                    }
                }
            }
            else
            {
                for (const auto& item : g_preview.items)
                    ImGui::TextUnformatted(item.c_str());
            }
        }
        else
        {
            ImGui::TextDisabled("Select a file to preview.");
        }
        ImGui::EndChild();
        ImGui::Columns(1);

        ImGui::End();
    }
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam),
                                        DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int)
{
    if (lpCmdLine && lpCmdLine[0] != '\0')
        std::snprintf(g_folder, sizeof(g_folder), "%s", lpCmdLine);

    WNDCLASSEXW wc = {sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr,
                      nullptr, nullptr, L"EvelentWalker", nullptr};
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"EvelentWalker (C++ / ImGui + D3D11)",
                                WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800,
                                nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    g_viewportReady = g_viewport.init(g_pd3dDevice, g_pd3dDeviceContext);

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        drawUI();

        ImGui::Render();
        const float clear[4] = {0.10f, 0.10f, 0.12f, 1.0f};
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    g_viewport.shutdown();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
