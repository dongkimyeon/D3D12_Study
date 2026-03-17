#pragma once
#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 헬퍼 함수들을 제외하여 컴파일 속도를 높임
#define NOMINMAX            // Windows.h의 min/max 매크로가 std::min/max와 충돌하는 것을 방지
#define UNICODE             // 유니코드 문자셋 사용

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// C++ 표준 라이브러리
#include <cstdint>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <string>
#include <array>

#include <wrl/client.h>
#include <windows.h>
#include <d3d12.h>          // D3D12 핵심 인터페이스
#include <dxgi1_6.h>        // DirectX Graphics Infrastructure (어댑터 및 스왑체인 관리)
#include <d3dcompiler.h>    // HLSL 셰이더 컴파일러
#include <assert.h>         // 오류 검출용


#include "OBJLoader.h"
#include <DirectXMath.h>  // 행렬 연산용


#include "imgui.h"
#include "backends/imgui_impl_dx12.h"   
#include "backends/imgui_impl_win32.h"


// 사용자 정의 헤더 파일
#include "Input.h"
#include "Time.h"

// 라이브러리 링크 (솔루션 설정에서 추가해도 되지만 코드에 명시하는 것이 관리하기 편함)
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")


using Microsoft::WRL::ComPtr;
using namespace DirectX;
