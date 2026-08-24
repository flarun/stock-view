#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <implot.h>
#include <d3d11.h>
#include <tchar.h>
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>

#include "StockModel.h"
#include "AppView.h"
#include "HistoryStorage.h"
#include "DataProvider.h"
#include "DataService.h"
#include "ConfigManager.h"

// --- Windows Native Dialog Helpers (Same as before) ---
std::string OpenSaveFileDialog(HWND owner)
{
  OPENFILENAMEA ofn;
  CHAR szFile[260] = {0};
  ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
  ofn.lStructSize = sizeof(OPENFILENAMEA);
  ofn.hwndOwner = owner;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "JSON Files\0*.json\0All Files\0*.*\0";
  ofn.nFilterIndex = 1;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
  ofn.lpstrDefExt = "json";
  if (GetSaveFileNameA(&ofn) == TRUE)
    return std::string(ofn.lpstrFile);
  return "";
}

std::string OpenLoadFileDialog(HWND owner)
{
  OPENFILENAMEA ofn;
  CHAR szFile[260] = {0};
  ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
  ofn.lStructSize = sizeof(OPENFILENAMEA);
  ofn.hwndOwner = owner;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "JSON Files\0*.json\0All Files\0*.*\0";
  ofn.nFilterIndex = 1;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
  if (GetOpenFileNameA(&ofn) == TRUE)
    return std::string(ofn.lpstrFile);
  return "";
}

// --- DirectX Globals & Init (Same as before) ---
static ID3D11Device *g_pd3dDevice = nullptr;
static ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
static IDXGISwapChain *g_pSwapChain = nullptr;
static ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

void CreateRenderTarget()
{
  ID3D11Texture2D *pBackBuffer;
  g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
  pBackBuffer->Release();
}

void CleanupRenderTarget()
{
  if (g_mainRenderTargetView)
  {
    g_mainRenderTargetView->Release();
    g_mainRenderTargetView = nullptr;
  }
}

bool CreateDeviceD3D(HWND hWnd)
{
  DXGI_SWAP_CHAIN_DESC sd = {};
  sd.BufferCount = 2;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = hWnd;
  sd.SampleDesc.Count = 1;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  D3D_FEATURE_LEVEL featureLevel;
  const D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
  if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                                    featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
                                    &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
    return false;

  CreateRenderTarget();
  return true;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;

  // --- THE FIX: Tell DirectX to resize the canvas when the window resizes ---
  if (msg == WM_SIZE)
  {
    if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
    {
      CleanupRenderTarget();
      g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
      CreateRenderTarget();
    }
    return 0;
  }

  if (msg == WM_DESTROY)
  {
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}

// --- Main Loop ---
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{
  ImGui_ImplWin32_EnableDpiAwareness();
  WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0, 0, hInstance, nullptr, nullptr, nullptr, nullptr, _T("StockView"), nullptr};
  RegisterClassEx(&wc);
  HWND hwnd = CreateWindow(wc.lpszClassName, _T("Stock View"), WS_OVERLAPPEDWINDOW, 100, 100, 1000, 700, nullptr, nullptr, wc.hInstance, nullptr);
  if (!CreateDeviceD3D(hwnd))
    return 1;

  ShowWindow(hwnd, SW_SHOWDEFAULT);
  UpdateWindow(hwnd);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
  // --- DPI SCALING MATH ---
  // Query Windows for the monitor's DPI (96 is the standard 100% scale)
  float dpiScale = GetDpiForWindow(hwnd) / 96.0f;

  // 1. Scale all the text
  ImGui::GetIO().FontGlobalScale = dpiScale;

  // 2. Scale all the UI elements (padding, borders, scrollbars)
  ImGui::GetStyle().ScaleAllSizes(dpiScale);

  ConfigManager::GetInstance().Load();

  // 1. Initialize Architecture
  StockModel model;
  AppView view;

  // Inject the Finnhub provider into our DataService
  auto provider = std::make_shared<FinnhubProvider>();
  DataService dataService(model, provider);
  dataService.Start(); // Boot up the background worker thread
  auto &activeTickers = ConfigManager::GetInstance().GetSettings().activeTickers;
  auto lastPoll = std::chrono::steady_clock::now() - std::chrono::seconds(5);
  auto appStartTime = std::chrono::steady_clock::now();

  bool done = false;
  while (!done)
  {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      if (msg.message == WM_QUIT)
        done = true;
    }
    if (done)
      break;

    auto now = std::chrono::steady_clock::now();
    double currentAppTime = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();

    // 2. Scheduler Logic: Just dump tasks into the queue!
    // Get the live polling interval from the ConfigManager (convert ms to seconds)
    float currentPollInterval = ConfigManager::GetInstance().GetSettings().pollingIntervalMs / 1000.0f;

    if (std::chrono::duration<float>(now - lastPoll).count() >= currentPollInterval)
    {
      for (const std::string &ticker : activeTickers)
      {
        dataService.EnqueueFetch(ticker, currentAppTime);
      }
      lastPoll = now;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // 3. Render View & Handle Events
    AppEvents events = view.Render(model.GetStocks(), model.HasApiError());

    if (events.quit)
      done = true;

    if (events.saveRequested)
    {
      std::string path = OpenSaveFileDialog(hwnd);
      if (!path.empty())
        HistoryStorage::Save(model.GetStocks(), path);
    }
    if (events.loadRequested)
    {
      std::string path = OpenLoadFileDialog(hwnd);
      if (!path.empty())
      {
        auto loadedData = HistoryStorage::Load(path);
        if (!loadedData.empty())
          model.LoadFromHistory(loadedData);
      }
    }

    if (!events.addTicker.empty())
    {
      if (std::find(activeTickers.begin(), activeTickers.end(), events.addTicker) == activeTickers.end())
      {
        activeTickers.push_back(events.addTicker);
        ConfigManager::GetInstance().Save();
        // Fetch the new ticker immediately
        dataService.EnqueueFetch(events.addTicker, currentAppTime);
      }
    }

    if (!events.removeTicker.empty())
    {
      activeTickers.erase(std::remove(activeTickers.begin(), activeTickers.end(), events.removeTicker), activeTickers.end());
      model.RemoveStock(events.removeTicker);
      ConfigManager::GetInstance().Save();
    }

    ImGui::Render();
    const float clear_color[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present(1, 0);
  }

  // 4. Clean Shutdown
  dataService.Stop(); // Safely close the worker thread before destroying resources

  CleanupRenderTarget();
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
  return 0;
}