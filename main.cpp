#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 헬퍼 함수들을 제외하여 컴파일 속도를 높임
#define NOMINMAX            // Windows.h의 min/max 매크로가 std::min/max와 충돌하는 것을 방지
#define UNICODE             // 유니코드 문자셋 사용
#define DEBUG_BUILD         // 디버그 모드 설정을 위한 정의

#include <windows.h>
#include <d3d12.h>          // D3D12 핵심 인터페이스
#include <dxgi1_6.h>        // DirectX Graphics Infrastructure (어댑터 및 스왑체인 관리)
#include <d3dcompiler.h>    // HLSL 셰이더 컴파일러
#include <assert.h>         // 오류 검출용
#include <cstdint>
// 라이브러리 링크 (솔루션 설정에서 추가해도 되지만 코드에 명시하는 것이 관리하기 편함)
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

static bool global_windowDidResize = false;

// 스왑 체인에 사용할 버퍼 개수. 보통 더블 버퍼링(2)이나 트리플 버퍼링(3)을 사용함.
const UINT FRAME_BUFFER_COUNT = 2;

// 윈도우 메시지 처리기 (콜백 함수)
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE) DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        global_windowDidResize = true; // 창 크기가 변했음을 알림 (실제 구현 시 리소스 재생성 필요)
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nShowCmd*/)
{
    // ============================================================================
    // 1. 윈도우 생성
    // ============================================================================
    HWND hwnd;
    {
        WNDCLASSEXW winClass = { sizeof(WNDCLASSEXW) };
        winClass.style = CS_HREDRAW | CS_VREDRAW;
        winClass.lpfnWndProc = &WndProc;
        winClass.hInstance = hInstance;
        winClass.hCursor = LoadCursorW(0, IDC_ARROW);
        winClass.lpszClassName = L"D3D12WindowClass";

        if (!RegisterClassExW(&winClass)) return 0;

        // 실제 그리고 싶은 영역(Client Area)을 1024x768로 맞추기 위해 창 전체 크기를 계산
        RECT initialRect = { 0, 0, 1280, 720 };
        AdjustWindowRectEx(&initialRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);

        hwnd = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, winClass.lpszClassName, L"D3D12 Triangle",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
            initialRect.right - initialRect.left, initialRect.bottom - initialRect.top,
            0, 0, hInstance, 0);
    }

    // ============================================================================
    // 2. 디버그 레이어 활성화
    // D3D12는 잘못된 코드를 작성해도 무시하고 지나가다가 나중에 크래시가 날 수 있습니다.
    // 디버그 레이어를 켜면 API 호출 오류를 실시간으로 출력창에 알려줍니다.
    // ============================================================================
#if defined(DEBUG_BUILD)
    {
        ID3D12Debug* debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&debugController)))
        {
            debugController->EnableDebugLayer();
            debugController->Release();
        }
    }
#endif

    // ============================================================================
    // 3. 디바이스 생성 (GPU 선택)
    // ============================================================================
    ID3D12Device* d3d12Device;
    {
        IDXGIFactory4* dxgiFactory;
        CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&dxgiFactory);

        IDXGIAdapter1* selectedAdapter = nullptr;
        SIZE_T maxDedicatedVideoMemory = 0;

        // 시스템에 장착된 여러 GPU 중 가장 성능이 좋은(VRAM이 큰) 하드웨어 GPU를 찾음
        for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &selectedAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 desc;
            selectedAdapter->GetDesc1(&desc);
            // 소프트웨어 렌더러는 제외하고 하드웨어 GPU만 선택
            if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
                break;
            }
            selectedAdapter->Release();
        }

        // Feature Level 11.0: D3D12를 쓰더라도 최소 D3D11 하드웨어 기능은 지원해야 함
        D3D12CreateDevice(selectedAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&d3d12Device);

        selectedAdapter->Release();
        dxgiFactory->Release();
    }

    // ============================================================================
    // 4. 커맨드 큐 생성
    // GPU는 CPU와 비동기적으로 작동합니다. CPU가 "이거 그려줘"라고 명령을 던지면
    // 명령들이 이 '큐(Queue)'에 쌓이고 GPU가 순서대로 가져가서 처리합니다.
    // ============================================================================
    ID3D12CommandQueue* commandQueue;
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // 일반적인 그래픽 명령용
        d3d12Device->CreateCommandQueue(&desc, __uuidof(ID3D12CommandQueue), (void**)&commandQueue);
    }

    // ============================================================================
    // 5. 스왑체인 생성
    // 화면에 보여지는 버퍼(Front Buffer)와 그리는 중인 버퍼(Back Buffer)를 교체해주는 시스템
    // ============================================================================
    IDXGISwapChain3* swapChain;
    {
        IDXGIFactory4* dxgiFactory;
        CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&dxgiFactory);

        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1; // MSAA 미사용
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = FRAME_BUFFER_COUNT;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 성능이 가장 좋은 최신 방식

        IDXGISwapChain1* tempChain;
        dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &desc, nullptr, nullptr, &tempChain);
        tempChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapChain);
        tempChain->Release();
        dxgiFactory->Release();
    }

    // ============================================================================
    // 6. RTV (Render Target View) 디스크립터 힙 생성
    // D3D12에서 리소스(텍스처 등)를 사용하려면 'View(Descriptor)'가 필요합니다.
    // 디스크립터 힙은 이런 View들을 담아두는 배열 같은 메모리 공간입니다.
    // ============================================================================
    ID3D12DescriptorHeap* rtvHeap;
    UINT rtvDescriptorSize;
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = FRAME_BUFFER_COUNT;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // 렌더 타겟용 힙
        d3d12Device->CreateDescriptorHeap(&desc, __uuidof(ID3D12DescriptorHeap), (void**)&rtvHeap);
        // 하드웨어마다 디스크립터 한 칸의 크기가 다르므로 물어봐야 함 (포인터 연산용)
        rtvDescriptorSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // ============================================================================
    // 7. 백버퍼 리소스 및 View 생성
    // 스왑체인이 관리하는 실제 메모리(버퍼)를 가져와서 RTV와 연결합니다.
    // ============================================================================
    ID3D12Resource* frameBuffers[FRAME_BUFFER_COUNT];
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++) {
            swapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&frameBuffers[i]);
            // 리소스와 뷰를 연결: 이제 rtvHandle을 통해 이 버퍼에 그림을 그릴 수 있음
            d3d12Device->CreateRenderTargetView(frameBuffers[i], nullptr, rtvHandle);
            rtvHandle.ptr += rtvDescriptorSize; // 다음 칸으로 이동
        }
    }

    // ============================================================================
    // 8 & 9. 커맨드 알로케이터 및 리스트
    // Allocator: 명령을 저장할 메모리 공간 / List: 명령을 기록하는 도구(녹화기)
    // ============================================================================
    ID3D12CommandAllocator* commandAllocators[FRAME_BUFFER_COUNT];
    ID3D12GraphicsCommandList* commandList;
    {
        for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++) {
            d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&commandAllocators[i]);
        }
        // 리스트는 하나만 있어도 됩니다. (매 프레임 Reset해서 다시 기록하므로)
        d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0], nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&commandList);
        commandList->Close(); // 생성 직후엔 닫아두는 것이 관례
    }

    // ============================================================================
    // 10. Fence 생성 (CPU-GPU 동기화 도구)
    // GPU가 "나 여기까지 다 그렸어!"라고 신호를 주면 CPU가 확인하는 용도
    // ============================================================================
    ID3D12Fence* fence;
    UINT64 fenceValue = 0;
    HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&fence);

    // ============================================================================
    // 11 & 12. 셰이더 컴파일
    // GPU에서 실행될 소형 프로그램(HLSL)을 바이너리 형태로 변환
    // ============================================================================
    ID3DBlob* vsBlob, * psBlob;
    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "vs_main", "vs_5_1", 0, 0, &vsBlob, nullptr);
    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "ps_main", "ps_5_1", 0, 0, &psBlob, nullptr);

    // ============================================================================
    // 13. 루트 시그니처 (Root Signature)
    // 셰이더와 리소스(상수 버퍼, 텍스처 등) 사이의 "입구"를 정의하는 설계도
    // ============================================================================
    ID3D12RootSignature* rootSignature;
    {
        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        ID3DBlob* sigBlob;
        D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, nullptr);
        d3d12Device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&rootSignature);
        sigBlob->Release();
    }

    // ============================================================================
    // 14. PSO (Pipeline State Object) 생성
    // "어떤 셰이더를 쓸지", "어떻게 섞을지(Blend)", "어디를 깎아낼지(Cull)" 등
    // 모든 렌더링 상태를 하나의 거대한 상태 객체로 묶어서 GPU에 한 번에 전달합니다.
    // ============================================================================

    ID3D12PipelineState* pipelineState;
    {
        D3D12_INPUT_ELEMENT_DESC layout[] = {
            { "POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { layout, 2 };
        psoDesc.pRootSignature = rootSignature;
        psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
        psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

		//psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME; // 와이어프레임 모드
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // 솔리드 모드

		//psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT; // 앞면 컬링
		//psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK; // 뒷면 컬링
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 컬링 없음
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        d3d12Device->CreateGraphicsPipelineState(&psoDesc, __uuidof(ID3D12PipelineState), (void**)&pipelineState);
    }

    // ============================================================================
    // 15. 버텍스 버퍼 (사각형 데이터) 생성
    // ============================================================================

    // 1. 정점 데이터 (4개)
    float vertices[] = {
        // x,      y,    r, g, b, a
        -0.5f,  0.5f,  1, 0, 0, 1, // 0번: 왼쪽 위 (빨강)
         0.5f,  0.5f,  0, 1, 0, 1, // 1번: 오른쪽 위 (초록)
         0.5f, -0.5f,  0, 0, 1, 1, // 2번: 오른쪽 아래 (파랑)
        -0.5f, -0.5f,  1, 1, 0, 1  // 3번: 왼쪽 아래 (노랑)
    };

    // 2. 인덱스 데이터 (삼각형 2개를 만들기 위한 번호 순서)
    // 삼각형 1: 0 -> 1 -> 2
    // 삼각형 2: 0 -> 2 -> 3
    uint16_t indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    ID3D12Resource* vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vbView;
    {
        // UPLOAD 힙: CPU에서 메모리를 써서 GPU로 바로 보낼 수 있는 임시 공간
        D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_UPLOAD };
        D3D12_RESOURCE_DESC res = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, sizeof(vertices), 1, 1, 1, DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

        d3d12Device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &res, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&vertexBuffer);

        void* data;
        vertexBuffer->Map(0, nullptr, &data);
        memcpy(data, vertices, sizeof(vertices));
        vertexBuffer->Unmap(0, nullptr);

        vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vbView.StrideInBytes = sizeof(float) * 6;
        vbView.SizeInBytes = sizeof(vertices);
    }
	//인덱스 버퍼 생성
    ID3D12Resource* indexBuffer;
    D3D12_INDEX_BUFFER_VIEW ibView;
    {
        UINT indexBufferSize = sizeof(indices);

        D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_UPLOAD };
        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resDesc.Width = indexBufferSize;
        resDesc.Height = 1;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels = 1;
        resDesc.SampleDesc.Count = 1;
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        // 리소스 생성
        d3d12Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            __uuidof(ID3D12Resource), (void**)&indexBuffer);

        // 데이터 복사 (Map/Unmap)
        void* mappedData;
        indexBuffer->Map(0, nullptr, &mappedData);
        memcpy(mappedData, indices, indexBufferSize);
        indexBuffer->Unmap(0, nullptr);

        // 인덱스 버퍼 뷰 설정
        ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        ibView.Format = DXGI_FORMAT_R16_UINT; // 16비트 정수(uint16_t) 사용
        ibView.SizeInBytes = indexBufferSize;
    }


    // ============================================================================
    // 16. 메인 렌더 루프
    // ============================================================================
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            continue;
        }

        // 창 크기가 변경되었을 때 처리
        if (global_windowDidResize) {
            // 1. GPU가 작업을 끝낼 때까지 대기
            UINT64 waitValue = ++fenceValue;
            commandQueue->Signal(fence, waitValue);
            if (fence->GetCompletedValue() < waitValue) {
                fence->SetEventOnCompletion(waitValue, fenceEvent);
                WaitForSingleObject(fenceEvent, INFINITE);
            }

            // 2. 기존 백버퍼 리소스 해제 (필수!)
            for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++) {
                frameBuffers[i]->Release();
            }

            // 3. 스왑체인 크기 변경
            RECT rect;
            GetClientRect(hwnd, &rect);
            swapChain->ResizeBuffers(FRAME_BUFFER_COUNT, rect.right, rect.bottom, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

            // 4. 백버퍼 리소스 및 뷰 재생성 (기존 7번 과정 반복)
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
            for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++) {
                swapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&frameBuffers[i]);
                d3d12Device->CreateRenderTargetView(frameBuffers[i], nullptr, rtvHandle);
                rtvHandle.ptr += rtvDescriptorSize;
            }

            global_windowDidResize = false; // 처리 완료
            continue; // 이번 프레임은 건너뛰고 다음 프레임부터 새로 그리기
        }

        // 현재 그릴 수 있는 백버퍼 번호를 가져옴
        UINT backBufferIdx = swapChain->GetCurrentBackBufferIndex();

        // 16-2. 명령 기록 준비
        commandAllocators[backBufferIdx]->Reset();
        commandList->Reset(commandAllocators[backBufferIdx], pipelineState);

        // 16-3. 리소스 배리어 (Resource Barrier)
        // 리소스의 "용도"를 변경합니다. (표시용 PRESENT -> 렌더링용 RENDER_TARGET)
        // GPU 내부적으로 메모리 레이아웃을 최적화할 수 있게 알려주는 중요한 과정입니다.
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = frameBuffers[backBufferIdx];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        commandList->ResourceBarrier(1, &barrier);

        // 16-4. 화면 클리어 및 타겟 설정
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += backBufferIdx * rtvDescriptorSize;

        float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // 16-5. 그리기 명령 (녹화)
        RECT clientRect; GetClientRect(hwnd, &clientRect);
        D3D12_VIEWPORT vp = { 0, 0, (float)clientRect.right, (float)clientRect.bottom, 0, 1 };
        D3D12_RECT scissor = { 0, 0, clientRect.right, clientRect.bottom };

        commandList->RSSetViewports(1, &vp);
        commandList->RSSetScissorRects(1, &scissor);
        commandList->SetGraphicsRootSignature(rootSignature);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commandList->IASetIndexBuffer(&ibView);
        commandList->IASetVertexBuffers(0, 1, &vbView);

        commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

        // 16-6. 용도 다시 변경 (RENDER_TARGET -> PRESENT)
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        commandList->ResourceBarrier(1, &barrier);

        commandList->Close();

        // 16-7. 제출 및 화면 표시
        ID3D12CommandList* lists[] = { commandList };
        commandQueue->ExecuteCommandLists(1, lists);
        swapChain->Present(1, 0);

        // CPU-GPU 동기화: GPU가 이 프레임을 다 그릴 때까지 CPU를 잠시 기다리게 함
        // (실제 프로덕션에서는 매 프레임 기다리지 않고 버퍼를 여러 개 돌려 씀)
        UINT64 waitValue = ++fenceValue;
        commandQueue->Signal(fence, waitValue);
        if (fence->GetCompletedValue() < waitValue) {
            fence->SetEventOnCompletion(waitValue, fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
        }
    }

    return 0;
}