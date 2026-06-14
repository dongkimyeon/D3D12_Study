#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <cstdint>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <string>
#include <array>

#include <wrl/client.h>
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <assert.h>
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

#include "OBJLoader.h"
#include <DirectXMath.h>
#include <DirectXCollision.h>

#include "Input.h"
#include "Time.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

enum class LogColor {
	WHITE = 7,
	CYAN = 11,
	GREEN = 10,
	YELLOW = 14,
	MAGENTA = 13,
	RED = 12,
	BLUE = 9,
	GRAY = 8

};

inline void PrintLog(LogColor color, const std::string& msg)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, (WORD)color);
	std::cout << msg << std::endl;
	SetConsoleTextAttribute(hConsole, (WORD)LogColor::WHITE);
}

