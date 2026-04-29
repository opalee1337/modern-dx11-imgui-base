#pragma once // Copy pasted out of old source lol and kinda organized

// Windows
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dwmapi.h>
#include <tchar.h>
#include <TlHelp32.h>
#include <mmsystem.h>
#include <wrl/client.h>
#include <intrin.h>

// DirectX 11
#include <d3d11_1.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// STL
#include <array>
#include <string>
#include <vector>
#include <unordered_map>

#include <algorithm>
#include <random>

#include <atomic>
#include <thread>
#include <shared_mutex>

#include <cctype>
#include <cstdint>
#include <iostream>
#include <memory>
#include <functional>

// imgui
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <backend/imgui_impl_win32.h>
#include <backend/imgui_impl_dx11.h>