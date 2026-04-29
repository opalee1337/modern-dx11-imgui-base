# modern_dx11_base

dev message:

I was to laidback to write a readme so i let ai do it for me no hate please and thank you,
so basically why i made this is because most released "dx11-imgui-bases" are really outdated and idk i thought it was a fun project to make,
it features a high precision timer because most people just use "std::this_thread::sleep_for(std::chrono::milliseconds(1))", which has timer resolution problems
aka 15.6ms off on windows lol so feel free to use it as a learning project, its also written in modern c++ and not some 2014 ahhh.

A clean, minimal DirectX 11 overlay base written in C++23. Transparent fullscreen window, Dear ImGui integration, and a simple render callback everything you need to start building without the boilerplate.

---

## Features

- **Transparent fullscreen overlay** — `WS_EX_LAYERED` + DWM composition, fully click-through by default
- **DirectX 11 renderer** — hardware device, `IDXGISwapChain1`, double-buffered, BGRA format
- **Dear ImGui** — DX11 + Win32 backend, no `.ini` saved to disk
- **Input toggle** — `RShift` toggles the menu open/closed, switching the window between click-through and interactive
- **High-precision timer** — `timeBeginPeriod(1)` + custom HPT sleep for tight frame pacing
- **Random window class name** — generated at runtime to avoid static fingerprinting
- **COM RAII** — all D3D objects managed via `WRL::ComPtr`, no manual releases
- **C++23** — `std::ranges`, `std::size`, latest standard throughout

---

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 (v143 toolset)
- DirectX SDK (legacy) — `DXSDK_DIR` environment variable must be set

> If you have the Windows SDK 10.0+ installed, you may already have D3D11 headers. The legacy DXSDK is only needed if your machine doesn't have them via the Windows SDK.

---

## Building

**Visual Studio 2022 (recommended)**

Open `modern_dx11_base.sln` and hit `F5` or `Ctrl+B`. That's it.

**CMake**

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

---

## Project Structure

```
modern_dx11_base/
├── source/
│   ├── main.cpp                  # Entry point, render callback setup
│   ├── pch.h / pch.cpp           # Precompiled header
│   ├── overlay/
│   │   ├── overlay.h             # Window, Renderer, ImGuiLayer, Overlay classes
│   │   └── overlay.cpp
│   └── common/
│       ├── common.h
│       └── timer/
│           ├── timer.h           # High-precision timer
│           └── timer.cpp
└── deps/
    └── imgui/                    # Dear ImGui (vendored)
        └── backend/              # imgui_impl_dx11 + imgui_impl_win32
```

---

## Usage

All render logic goes in the `OnRender` callback in `main.cpp`:

```cpp
gOverlay.OnRender = []() {
    if (gOverlay.IsOpen) {
        // your ImGui calls here
        ImGui::Begin("My Overlay");
        ImGui::Text("Hello world");
        ImGui::End();
    }
};
```

`gOverlay.IsOpen` reflects whether the menu is currently toggled on. Use it to gate any input-consuming ImGui windows.

**Keybind** — `RShift` toggles the menu. Change it in `overlay.cpp`:

```cpp
if (GetAsyncKeyState(VK_RSHIFT) & 0x8000) {
```

Replace `VK_RSHIFT` with any [virtual key code](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes).

---

## Architecture

The overlay is split into three focused classes inside the `pane` namespace:

- `Window` — creates the layered, topmost, click-through popup window with a random class name
- `Renderer` — owns the D3D11 device, device context, swap chain, and render target
- `ImGuiLayer` — wraps ImGui init/shutdown and the per-frame begin/end calls, handles toggling `WS_EX_TRANSPARENT` for input passthrough

These are composed by `Overlay`, which exposes the message loop, the `OnRender` callback, and the global `gOverlay` singleton.

---

## License

Do whatever you want with it.
