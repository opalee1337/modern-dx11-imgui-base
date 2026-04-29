#pragma once
#include "pch.h"

namespace pane { // Dont ask about the name brodie i dont even know myself it sounded cool for a "framework" igs??

class Window {
public:
  bool Create(HINSTANCE hInstance, void *lpParam = nullptr);
  void Destroy();

  HWND GetHwnd() const { return m_hwnd; }
  int GetWidth() const { return m_width; }
  int GetHeight() const { return m_height; }

private:
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                  LPARAM lParam);
  static std::wstring GenerateRandomName(std::size_t length = 16);

  HWND m_hwnd{};
  WNDCLASSEXW m_wc{};
  std::wstring m_name{};
  int m_width{};
  int m_height{};
};

class Renderer {
public:
  bool Initialize(HWND hwnd);
  void Shutdown();
  void Present();
  void ResizeRenderTarget();

  ID3D11Device *GetDevice() const { return m_device.Get(); }
  ID3D11DeviceContext *GetDeviceContext() const {
    return m_deviceContext.Get();
  }
  ID3D11RenderTargetView *GetRenderTargetView() const {
    return m_renderTargetView.Get();
  }

private:
  bool CreateDeviceAndSwapChain(HWND hwnd);
  bool CreateRenderTarget();
  void CleanupRenderTarget();

  // Lifetime management for COM objects (RAII)
  Microsoft::WRL::ComPtr<ID3D11Device> m_device;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_deviceContext;
  Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
};

class ImGuiLayer {
public:
  bool Initialize(HWND hwnd, Renderer *renderer);
  void Shutdown();

  void BeginFrame();
  void EndFrame();

  void SetInputEnabled(bool enabled);

private:
  HWND m_hwnd{};
  Renderer *m_renderer{};
};

class Overlay {
public:
  static Overlay &Get() {
    static Overlay instance;
    return instance;
  }

  Overlay() = default;
  ~Overlay() = default;

  Overlay(const Overlay &) = delete;
  Overlay &operator=(const Overlay &) = delete;
  Overlay(Overlay &&) = delete;
  Overlay &operator=(Overlay &&) = delete;

  bool Initialize(HINSTANCE hInstance);
  void Shutdown();

  void Run();
  void Wait();
  void RequestExit() { m_running = false; }

  bool IsRunning() const { return m_running; }
  bool IsOpen{false};

  std::function<void()> OnRender;

  Window &GetWindow() { return m_window; }
  const Window &GetWindow() const { return m_window; }

  Renderer &GetRenderer() { return m_renderer; }
  const Renderer &GetRenderer() const { return m_renderer; }

private:
  Window m_window;
  Renderer m_renderer;
  ImGuiLayer m_imgui;

  bool m_running{};
  bool m_initialized{};
};

} // namespace pane

inline pane::Overlay &gOverlay = pane::Overlay::Get();