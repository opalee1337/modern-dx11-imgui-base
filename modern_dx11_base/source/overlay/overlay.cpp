#include "overlay.h"
#include "../common/common.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

namespace pane {

// Window

bool Window::Create(HINSTANCE hInstance, void *lpParam) {
  m_name = GenerateRandomName();

  m_wc = {};
  m_wc.cbSize = sizeof(WNDCLASSEXW);
  m_wc.style = CS_HREDRAW | CS_VREDRAW;
  m_wc.lpfnWndProc = WndProc;
  m_wc.hInstance = hInstance;
  m_wc.lpszClassName = m_name.c_str();

  if (!RegisterClassExW(&m_wc))
    return false;

  m_width = GetSystemMetrics(SM_CXSCREEN);
  m_height = GetSystemMetrics(SM_CYSCREEN);

  m_hwnd =
      CreateWindowExW(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE |
                          WS_EX_LAYERED | WS_EX_TOOLWINDOW,
                      m_name.c_str(), m_name.c_str(), WS_POPUP, 0, 0, m_width,
                      m_height, nullptr, nullptr, hInstance, lpParam);

  if (!m_hwnd) {
    UnregisterClassW(m_name.c_str(), hInstance);
    return false;
  }

  MARGINS margins{-1, -1, -1, -1};
  DwmExtendFrameIntoClientArea(m_hwnd, &margins);
  SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);

  ShowWindow(m_hwnd, SW_SHOW);
  UpdateWindow(m_hwnd);

  return true;
}

void Window::Destroy() {
  if (m_hwnd) {
    DestroyWindow(m_hwnd);
    m_hwnd = nullptr;
  }
  UnregisterClassW(m_name.c_str(), m_wc.hInstance);
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                 LPARAM lParam) {
  if (msg == WM_CREATE) {
    auto *cs = reinterpret_cast<CREATESTRUCTW *>(lParam);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                      reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    return 0;
  }

  if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
    return true;

  switch (msg) {
  case WM_SIZE: {
    auto *overlay =
        reinterpret_cast<Overlay *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (overlay && overlay->GetRenderer().GetDevice() &&
        wParam != SIZE_MINIMIZED)
      overlay->GetRenderer().ResizeRenderTarget();
    return 0;
  }
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

std::wstring Window::GenerateRandomName(std::size_t length) {
  static constexpr wchar_t kChars[] =
      L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  static constexpr std::size_t kCharCount =
      (sizeof(kChars) / sizeof(wchar_t)) - 1;

  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<std::size_t> dist(0, kCharCount - 1);

  std::wstring name(length, L'\0');
  for (auto &c : name)
    c = kChars[dist(rng)];

  return name;
}

// Renderer

bool Renderer::Initialize(HWND hwnd) {
  if (!CreateDeviceAndSwapChain(hwnd))
    return false;

  if (!CreateRenderTarget())
    return false;

  return true;
}

void Renderer::Shutdown() {
  CleanupRenderTarget();
  m_swapChain.Reset();
  m_deviceContext.Reset();
  m_device.Reset();
}

void Renderer::Present() { m_swapChain->Present(0, 0); }

void Renderer::ResizeRenderTarget() {
  CleanupRenderTarget();
  m_swapChain->ResizeBuffers(2, 0, 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
  CreateRenderTarget();
}

bool Renderer::CreateDeviceAndSwapChain(HWND hwnd) {
  constexpr D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0};

  D3D_FEATURE_LEVEL featureLevel{};

  HRESULT hr = D3D11CreateDevice(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
      D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels,
      static_cast<UINT>(std::size(featureLevels)), D3D11_SDK_VERSION,
      m_device.GetAddressOf(), &featureLevel, m_deviceContext.GetAddressOf());

  if (FAILED(hr))
    return false;

  Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice;
  if (FAILED(m_device->QueryInterface(IID_PPV_ARGS(&dxgiDevice))))
    return false;

  Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
  if (FAILED(dxgiDevice->GetAdapter(&adapter)))
    return false;

  Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
  if (FAILED(adapter->GetParent(IID_PPV_ARGS(&factory))))
    return false;

  DXGI_SWAP_CHAIN_DESC1 sd{};
  sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.Stereo = FALSE;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.BufferCount = 2;
  sd.Scaling = DXGI_SCALING_STRETCH;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  hr = factory->CreateSwapChainForHwnd(m_device.Get(), hwnd, &sd, nullptr,
                                       nullptr, m_swapChain.GetAddressOf());
  factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

  return SUCCEEDED(hr);
}

bool Renderer::CreateRenderTarget() {
  Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;

  if (FAILED(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))))
    return false;

  return SUCCEEDED(m_device->CreateRenderTargetView(
      backBuffer.Get(), nullptr, m_renderTargetView.GetAddressOf()));
}

void Renderer::CleanupRenderTarget() { m_renderTargetView.Reset(); }

// ImGuiLayer

bool ImGuiLayer::Initialize(HWND hwnd, Renderer *renderer) {
  m_hwnd = hwnd;
  m_renderer = renderer;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;

  ImGui::StyleColorsDark();

  if (!ImGui_ImplWin32_Init(hwnd))
    return false;

  if (!ImGui_ImplDX11_Init(m_renderer->GetDevice(),
                           m_renderer->GetDeviceContext())) {
    ImGui_ImplWin32_Shutdown();
    return false;
  }

  return true;
}

void ImGuiLayer::Shutdown() {
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiLayer::BeginFrame() {
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::EndFrame() {
  ImGui::Render();

  constexpr float kClearColor[4] = {0.f, 0.f, 0.f, 0.f};
  auto *ctx = m_renderer->GetDeviceContext();
  auto *rtv = m_renderer->GetRenderTargetView();
  ctx->OMSetRenderTargets(1, &rtv, nullptr);
  ctx->ClearRenderTargetView(rtv, kClearColor);
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::SetInputEnabled(bool enabled) {
  LONG_PTR exStyle = GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE);

  if (enabled) {
    SetWindowLongPtrW(m_hwnd, GWL_EXSTYLE,
                      exStyle & ~WS_EX_TRANSPARENT & ~WS_EX_NOACTIVATE);

    SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

    SetForegroundWindow(m_hwnd);
    SetFocus(m_hwnd);
  } else {
    SetWindowLongPtrW(m_hwnd, GWL_EXSTYLE,
                      exStyle | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);

    SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
  }
}

// Overlay

bool Overlay::Initialize(HINSTANCE hInstance) {
  if (m_initialized)
    return true;

  if (!m_window.Create(hInstance, this))
    return false;

  if (!m_renderer.Initialize(m_window.GetHwnd())) {
    m_window.Destroy();
    return false;
  }

  if (!m_imgui.Initialize(m_window.GetHwnd(), &m_renderer)) {
    m_renderer.Shutdown();
    m_window.Destroy();
    return false;
  }

  m_initialized = true;
  m_imgui.SetInputEnabled(IsOpen);
  return true;
}

void Overlay::Shutdown() {
  if (!m_initialized)
    return;

  m_running = false;
  IsOpen = false;

  m_imgui.Shutdown();
  m_renderer.Shutdown();
  m_window.Destroy();

  m_initialized = false;
}

void Overlay::Run() {
  m_running = true;

  MSG msg{};
  timeBeginPeriod(1);

  while (m_running) {
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);

      if (msg.message == WM_QUIT) {
        m_running = false;
        IsOpen = false;
      }
    }

    // toggle menu on RShift
    static bool pressed = false;
    if (GetAsyncKeyState(VK_RSHIFT) & 0x8000) {
      if (!pressed) {
        IsOpen = !IsOpen;
        m_imgui.SetInputEnabled(IsOpen);
        pressed = true;
      }
    } else {
      pressed = false;
    }

    m_imgui.BeginFrame();
    if (OnRender)
      OnRender();
    m_imgui.EndFrame();
    m_renderer.Present();

    pane::timer::PrecisionSleep(1.0); // our cool ass hpt timer
  }

  timeEndPeriod(1);
}

void Overlay::Wait() {}

} // namespace pane