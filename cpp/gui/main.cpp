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
#include <cctype>
#include <fstream>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "evw/app/explorer.h"
#include "evw/app/render_mesh.h"
#include "evw/gamefiles/ddsio.h"
#include "evw/gamefiles/drawable.h"
#include "evw/gamefiles/resource_data.h"
#include "evw/gamefiles/texture.h"
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

// ---- Preview texture (for .ytd image display) ----
static ID3D11ShaderResourceView* g_previewTexSRV = nullptr;
static void ReleasePreviewTex()
{
    if (g_previewTexSRV) { g_previewTexSRV->Release(); g_previewTexSRV = nullptr; }
}
static void CreatePreviewTex(const uint8_t* rgba, int w, int h)
{
    ReleasePreviewTex();
    if (!rgba || w <= 0 || h <= 0 || !g_pd3dDevice) return;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = rgba; sd.SysMemPitch = static_cast<UINT>(w) * 4;
    ID3D11Texture2D* tex = nullptr;
    if (SUCCEEDED(g_pd3dDevice->CreateTexture2D(&desc, &sd, &tex)) && tex)
    {
        g_pd3dDevice->CreateShaderResourceView(tex, nullptr, &g_previewTexSRV);
        tex->Release();
    }
}

// ---- Explorer UI state ----
namespace
{
    evw::app::ExplorerModel g_model;
    char g_folder[1024] = "";
    char g_search[256] = "";
    char g_path[1024] = "";          // current folder path (breadcrumb)
    std::string g_currentDir;         // "" = top level (list of archives)
    std::vector<std::string> g_results;
    bool g_searching = false;
    evw::app::FilePreview g_preview;
    std::string g_status = "Enter a GTA V folder path and click Scan.";
    std::string g_selected;
    bool g_showAbout = false;

    evw::gui::Viewport g_viewport;
    bool g_viewportReady = false;
    bool g_hasModel = false;
    float g_yaw = 0.7f, g_pitch = 0.4f, g_distScale = 1.0f;

    std::vector<evw::app::RenderTexture> g_texList;
    int g_texIndex = 0;
    size_t g_dictCount = 0;
    int g_dictIndex = 0;
    std::string g_modelPath;

    bool endsWithCi(const std::string& s, const std::string& suf)
    {
        if (s.size() < suf.size()) return false;
        for (size_t i = 0; i < suf.size(); ++i)
            if (std::tolower((unsigned char)s[s.size() - suf.size() + i]) != suf[i]) return false;
        return true;
    }

    std::string parentDir(const std::string& p)
    {
        size_t pos = p.find_last_of('\\');
        return pos == std::string::npos ? std::string() : p.substr(0, pos);
    }

    std::string humanSize(long long bytes)
    {
        char buf[48];
        const char* u[] = {"B", "KB", "MB", "GB"};
        double s = static_cast<double>(bytes);
        int i = 0;
        while (s >= 1024.0 && i < 3) { s /= 1024.0; ++i; }
        std::snprintf(buf, sizeof(buf), i == 0 ? "%.0f %s" : "%.1f %s", s, u[i]);
        return buf;
    }

    void navigateTo(const std::string& dir)
    {
        g_currentDir = dir;
        std::snprintf(g_path, sizeof(g_path), "%s", dir.c_str());
        g_searching = false;
    }

    void onSelect(const std::string& path)
    {
        g_selected = path;
        g_preview = g_model.openFile(path);
        g_hasModel = false;
        g_modelPath = path;
        ReleasePreviewTex();
        g_texList.clear();
        g_texIndex = 0;
        g_dictCount = 0;
        g_dictIndex = 0;

        bool renderable = g_preview.kind == evw::app::PreviewKind::Drawable ||
                          g_preview.kind == evw::app::PreviewKind::DrawableDictionary;
        if (g_viewportReady && renderable)
        {
            auto rmodel = g_model.buildRenderModelForPath(path);
            g_viewport.setModel(rmodel);
            g_hasModel = g_viewport.hasGeometry();
            g_yaw = 0.7f; g_pitch = 0.4f; g_distScale = 1.0f;
            if (g_preview.kind == evw::app::PreviewKind::DrawableDictionary)
                g_dictCount = g_model.dictionaryDrawableCount(path);
        }
        if (endsWithCi(path, ".ytd"))
        {
            g_texList = g_model.buildTextureList(path);
            if (!g_texList.empty())
                CreatePreviewTex(g_texList[0].rgba.data(), g_texList[0].width, g_texList[0].height);
        }
    }

    void doScan()
    {
        try
        {
            g_model.init(g_folder);
            char buf[160];
            std::snprintf(buf, sizeof(buf), "Indexed %zu archives, %zu entries. Keys: %s",
                          g_model.rpfCount(), g_model.entryCount(),
                          g_model.keysLoaded() ? "loaded" : "NOT loaded (encrypted archives unreadable)");
            g_status = buf;
            navigateTo("");
            g_results.clear();
            g_searching = false;
        }
        catch (const std::exception& ex)
        {
            g_status = std::string("Scan failed: ") + ex.what();
        }
    }

    void refreshSearch()
    {
        if (!g_model.isInited()) return;
        if (g_search[0] == '\0') { g_searching = false; return; }
        g_searching = true;
        g_results = g_model.search(g_search, 1000);
    }

    // Recursively draws the folder/archive tree on the left.
    void drawTree(const std::string& dir)
    {
        for (const auto& child : g_model.listChildren(dir))
        {
            if (!child.isDirectory) continue;
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                       ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                       ImGuiTreeNodeFlags_SpanAvailWidth;
            if (g_currentDir == child.path) flags |= ImGuiTreeNodeFlags_Selected;
            bool open = ImGui::TreeNodeEx(child.path.c_str(), flags, "%s", child.name.c_str());
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                navigateTo(child.path);
            if (open)
            {
                drawTree(child.path);
                ImGui::TreePop();
            }
        }
    }

    // Draws the columned list of entries for the current folder (or search results).
    void drawList()
    {
        ImGuiTableFlags tflags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
                                 ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY |
                                 ImGuiTableFlags_SizingStretchProp;
        if (!ImGui::BeginTable("entries", 3, tflags)) return;
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 1.5f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableHeadersRow();

        auto drawRow = [&](const evw::app::EntryInfo& e) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const std::string label = (e.isDirectory ? "[+] " : "    ") + e.name;
            bool selected = g_selected == e.path;
            if (ImGui::Selectable(label.c_str(), selected,
                                  ImGuiSelectableFlags_SpanAllColumns |
                                  ImGuiSelectableFlags_AllowDoubleClick))
            {
                if (e.isDirectory)
                {
                    if (ImGui::IsMouseDoubleClicked(0)) navigateTo(e.path);
                }
                else
                {
                    onSelect(e.path);
                }
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(e.typeName.c_str());
            ImGui::TableSetColumnIndex(2);
            if (!e.isDirectory) ImGui::TextUnformatted(humanSize(e.size).c_str());
        };

        if (g_searching)
        {
            for (const auto& p : g_results)
            {
                evw::app::EntryInfo e;
                e.path = p;
                size_t pos = p.find_last_of('\\');
                e.name = pos == std::string::npos ? p : p.substr(pos + 1);
                e.isDirectory = g_model.isDirectory(p);
                drawRow(e);
            }
        }
        else
        {
            for (const auto& e : g_model.listChildren(g_currentDir)) drawRow(e);
        }
        ImGui::EndTable();
    }

    void drawPreview()
    {
        if (g_preview.path.empty()) { ImGui::TextDisabled("Select a file to preview."); return; }
        ImGui::TextWrapped("%s", g_preview.path.c_str());
        ImGui::Separator();
        ImGui::TextWrapped("%s", g_preview.summary.c_str());
        ImGui::Text("Size: %zu bytes", g_preview.dataSize);
        if (ImGui::Button("Extract..."))
        {
            std::string out = g_preview.path;
            size_t pos = out.find_last_of('\\');
            if (pos != std::string::npos) out = out.substr(pos + 1);
            size_t n = g_model.extractToFile(g_preview.path, out);
            g_status = n ? ("Extracted " + std::to_string(n) + " bytes to " + out)
                         : "Extraction failed (keys needed?)";
        }
        ImGui::Separator();

        if (g_hasModel)
        {
            // Drawable-dictionary selector: pick which drawable to render.
            if (g_dictCount > 1)
            {
                ImGui::SetNextItemWidth(160);
                if (ImGui::InputInt("Drawable##dict", &g_dictIndex))
                {
                    if (g_dictIndex < 0) g_dictIndex = 0;
                    if (g_dictIndex >= (int)g_dictCount) g_dictIndex = (int)g_dictCount - 1;
                    auto rm = g_model.buildDictionaryRenderModel(g_modelPath, (size_t)g_dictIndex);
                    g_viewport.setModel(rm);
                    g_hasModel = g_viewport.hasGeometry();
                }
                ImGui::SameLine();
                ImGui::Text("of %zu", g_dictCount);
            }
            ImGui::Text("3D model: %zu triangles", g_viewport.triangleCount());
            ImVec2 avail = ImGui::GetContentRegionAvail();
            int vw = (int)avail.x, vh = (int)(avail.y - 8);
            if (vw > 16 && vh > 16)
            {
                ID3D11ShaderResourceView* srv = g_viewport.render(vw, vh, g_yaw, g_pitch, g_distScale);
                if (srv)
                {
                    ImGui::Image((ImTextureID)srv, ImVec2((float)vw, (float)vh));
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
                    }
                }
            }
        }
        else
        {
            for (const auto& item : g_preview.items)
                ImGui::TextUnformatted(item.c_str());

            // Texture image preview for .ytd dictionaries.
            if (!g_texList.empty())
            {
                ImGui::SetNextItemWidth(200);
                if (ImGui::InputInt("Texture##ytd", &g_texIndex))
                {
                    if (g_texIndex < 0) g_texIndex = 0;
                    if (g_texIndex >= (int)g_texList.size()) g_texIndex = (int)g_texList.size() - 1;
                    const auto& t = g_texList[g_texIndex];
                    CreatePreviewTex(t.rgba.data(), t.width, t.height);
                }
                ImGui::SameLine();
                ImGui::Text("of %zu", g_texList.size());
                const auto& t = g_texList[g_texIndex];
                ImGui::Text("%dx%d", t.width, t.height);
                ImGui::SameLine();
                if (ImGui::Button("Save DDS"))
                {
                    evw::gamefiles::Texture tex;
                    tex.width = (uint16_t)t.width; tex.height = (uint16_t)t.height;
                    tex.levels = 1; tex.format = evw::gamefiles::TextureFormat::D3DFMT_A8B8G8R8;
                    tex.data = std::make_shared<evw::gamefiles::TextureData>();
                    tex.data->fullData = t.rgba;
                    std::string out = "texture_" + std::to_string(g_texIndex) + ".dds";
                    g_status = evw::texconv::writeDDSToFile(tex, out) ? ("Saved " + out)
                                                                      : "DDS save failed";
                }
                if (g_previewTexSRV)
                {
                    float maxw = ImGui::GetContentRegionAvail().x;
                    float scale = (t.width > maxw) ? maxw / t.width : 1.0f;
                    ImGui::Image((ImTextureID)g_previewTexSRV,
                                 ImVec2(t.width * scale, t.height * scale));
                }
            }

            if (!g_preview.text.empty())
            {
                if (ImGui::Button("Save XML..."))
                {
                    std::string out = g_preview.path;
                    for (char& c : out) if (c == '\\' || c == '/' || c == ':') c = '_';
                    out += ".xml";
                    std::ofstream f(out, std::ios::binary);
                    f.write(g_preview.text.data(), (std::streamsize)g_preview.text.size());
                    g_status = "Saved XML to " + out;
                }
                ImGui::SameLine();
                ImGui::Text("(%zu chars)", g_preview.text.size());
                ImGui::Separator();
                ImGui::BeginChild("xmltext", ImVec2(0, 0), true,
                                  ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::TextUnformatted(g_preview.text.c_str(),
                                       g_preview.text.c_str() + g_preview.text.size());
                ImGui::EndChild();
            }
        }
    }

    void drawMenuBar()
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Scan folder", "F5")) doScan();
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) ::PostQuitMessage(0);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Go to top", nullptr, false, g_model.isInited()))
                    navigateTo("");
                if (ImGui::MenuItem("Up one level", "Backspace",
                                    false, !g_currentDir.empty()))
                    navigateTo(parentDir(g_currentDir));
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("About")) g_showAbout = true;
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    void drawUI()
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("EvelentWalker", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_MenuBar);

        drawMenuBar();

        // Toolbar: folder + Scan, then navigation path + Up, then Search.
        ImGui::SetNextItemWidth(620);
        ImGui::InputText("##folder", g_folder, sizeof(g_folder));
        ImGui::SameLine();
        if (ImGui::Button("Scan")) doScan();
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        if (ImGui::Button(" Up ") && !g_currentDir.empty()) navigateTo(parentDir(g_currentDir));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(420);
        if (ImGui::InputText("##path", g_path, sizeof(g_path),
                             ImGuiInputTextFlags_EnterReturnsTrue))
            navigateTo(g_path);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(240);
        if (ImGui::InputText("Search", g_search, sizeof(g_search))) refreshSearch();

        ImGui::Separator();

        // Main 3-pane area, leaving room for the status bar.
        float bottom = ImGui::GetFrameHeightWithSpacing();
        ImVec2 region = ImGui::GetContentRegionAvail();
        ImVec2 paneSize(0, region.y - bottom);

        ImGui::BeginChild("tree", ImVec2(280, paneSize.y), true);
        ImGui::TextDisabled("Archives / Folders");
        ImGui::Separator();
        if (g_model.isInited())
        {
            if (ImGui::Selectable("[root]", g_currentDir.empty())) navigateTo("");
            drawTree("");
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("list", ImVec2(region.x * 0.45f, paneSize.y), true);
        drawList();
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("preview", ImVec2(0, paneSize.y), true);
        drawPreview();
        ImGui::EndChild();

        // Status bar.
        ImGui::Separator();
        ImGui::Text("%s", g_status.c_str());
        if (!g_currentDir.empty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("  |  %s", g_currentDir.c_str());
        }

        ImGui::End();

        if (g_showAbout)
        {
            ImGui::OpenPopup("About EvelentWalker");
            g_showAbout = false;
        }
        if (ImGui::BeginPopupModal("About EvelentWalker", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("EvelentWalker (C++ port)");
            ImGui::Text("RPF explorer - ImGui + Direct3D 11");
            ImGui::Separator();
            if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
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
    ReleasePreviewTex();
    g_viewport.shutdown();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
