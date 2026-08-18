#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <implot.h>
#include <d3d11.h>
#include <tchar.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <chrono>
#include <future>
#include <windows.h>
#include <commdlg.h>

#include "config.h"
#include "StockModel.h"
#include "AppView.h"
#include "HistoryStorage.h"

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
  {
    return std::string(ofn.lpstrFile);
  }
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
  {
    return std::string(ofn.lpstrFile);
  }
  return "";
}

// --- Network Functions ---
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

std::string FetchQuoteRaw(const std::string &symbol)
{
  CURL *curl = curl_easy_init();
  std::string response;
  if (curl)
  {
    std::string url = "https://finnhub.io/api/v1/quote?symbol=" + symbol + "&token=" + FINNHUB_API_KEY;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
      std::cerr << "curl error: " << curl_easy_strerror(res) << std::endl;
    }
    curl_easy_cleanup(curl);
  }
  return response;
}

float FetchQuotePrice(const std::string &symbol)
{
  std::string raw = FetchQuoteRaw(symbol);
  try
  {
    auto j = nlohmann::json::parse(raw);
    return j["c"].get<float>();
  }
  catch (...)
  {
    return -1.0f;
  }
}

// --- DirectX Globals ---
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

bool CreateDeviceD3D(HWND hWnd)
{
  DXGI_SWAP_CHAIN_DESC sd = {};
  sd.BufferCount = 2;
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
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

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;
  if (msg == WM_DESTROY)
  {
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

// --- Main Application Loop ---
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{
  WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0, 0, hInstance,
                   nullptr, nullptr, nullptr, nullptr, _T("StockView"), nullptr};
  RegisterClassEx(&wc);
  HWND hwnd = CreateWindow(wc.lpszClassName, _T("Stock View"), WS_OVERLAPPEDWINDOW,
                           100, 100, 1000, 700, nullptr, nullptr, wc.hInstance, nullptr);

  if (!CreateDeviceD3D(hwnd))
    return 1;

  ShowWindow(hwnd, SW_SHOWDEFAULT);
  UpdateWindow(hwnd);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

  // Instantiate Model and View
  StockModel model;
  AppView view;

  auto lastPoll = std::chrono::steady_clock::now();
  std::future<float> futurePrice = std::async(std::launch::async, FetchQuotePrice, "AAPL");
  bool isFetching = true;
  float timeCounter = 0.0f;
  const float pollIntervalSeconds = 5.0f;

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

    // 1. Controller logic: Dispatch new request if interval has passed
    if (!isFetching && std::chrono::duration<float>(now - lastPoll).count() >= pollIntervalSeconds)
    {
      isFetching = true;
      futurePrice = std::async(std::launch::async, FetchQuotePrice, "AAPL");
    }

    // 2. Controller logic: Process request if finished
    if (isFetching && futurePrice.valid() && futurePrice.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
      float p = futurePrice.get();
      if (p > 0)
      {
        model.AddPrice("AAPL", p, timeCounter);
        timeCounter += pollIntervalSeconds;
      }
      isFetching = false;
      lastPoll = std::chrono::steady_clock::now();
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // 3. Render View using the Model's state and capture events
    AppEvents events = view.Render(model.GetStocks());

    if (events.quit)
    {
      done = true;
    }
    if (events.saveRequested)
    {
      std::string path = OpenSaveFileDialog(hwnd);
      if (!path.empty())
      {
        HistoryStorage::Save(model.GetStocks(), path);
      }
    }
    if (events.loadRequested)
    {
      std::string path = OpenLoadFileDialog(hwnd);
      if (!path.empty())
      {
        auto loadedData = HistoryStorage::Load(path);
        if (!loadedData.empty())
        {
          model.LoadFromHistory(loadedData);
        }
      }
    }

    ImGui::Render();
    const float clear_color[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present(1, 0);
  }

  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
  return 0;
}