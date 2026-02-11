#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 헬퍼 함수들을 제외하여 컴파일 속도를 높임
#define NOMINMAX            // Windows.h의 min/max 매크로가 std::min/max와 충돌하는 것을 방지
#define UNICODE             // 유니코드 문자셋 사용
#define DEBUG_BUILD         // 디버그 모드 설정을 위한 정의

#include <windows.h>
#include <d3d12.h>          // D3D12 핵심 인터페이스
#include <dxgi1_6.h>        // DirectX Graphics Infrastructure (어댑터 및 스왑체인 관리)
#include <d3dcompiler.h>    // HLSL 셰이더 컴파일러
#include <assert.h>         // 오류 검출용


// C++ 표준 라이브러리
#include <cstdint>
#include <vector>
#include <random>

// 사용자 정의 헤더 파일
#include "Input.h"
#include "Time.h"

// 라이브러리 링크 (솔루션 설정에서 추가해도 되지만 코드에 명시하는 것이 관리하기 편함)
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

static bool global_windowDidResize = false;

// 스왑 체인에 사용할 버퍼 개수. 보통 더블 버퍼링(2)이나 트리플 버퍼링(3)을 사용함.
const UINT FRAME_BUFFER_COUNT = 2;
const UINT MAX_RECT_COUNT = 100; // 최대 사각형 개수 제한 (버퍼 오버플로우 방지)

// 사각형의 상태를 관리하는 구조체
class Rect {
public:
    struct Vertex {
        float x, y;
        float r, g, b, a;
    };

private:
    float x, y;
    float width, height;
    float vx, vy;
    float r, g, b, a;

public:
    // 1. Initialize: 랜덤 값으로 초기화
    void Initialize(std::mt19937& rng) {
        std::uniform_real_distribution<float> distPos(-0.7f, 0.7f);
        std::uniform_real_distribution<float> distVel(-0.5f, 0.5f);
        std::uniform_real_distribution<float> distCol(0.4f, 1.0f);

        x = distPos(rng);
        y = distPos(rng);
        width = 0.2f;
        height = 0.2f;
        vx = distVel(rng);
        vy = distVel(rng);
        r = distCol(rng);
        g = distCol(rng);
        b = distCol(rng);
        a = 1.0f;
    }

    // 2. Update: 물리 및 충돌 로직
    void Update(float dt) {
        x += vx * dt;
        y += vy * dt;

        if (x - width / 2 < -1.0f || x + width / 2 > 1.0f) {
            vx *= -1.0f;
            x = (x < 0) ? -1.0f + width / 2 : 1.0f - width / 2;
        }
        if (y - height / 2 < -1.0f || y + height / 2 > 1.0f) {
            vy *= -1.0f;
            y = (y < 0) ? -1.0f + height / 2 : 1.0f - height / 2;
        }
    }

    // 3. Render: 정점 데이터를 버퍼 리스트에 채움 (Batching 준비)
    void Render(std::vector<Vertex>& vertexData) const {
        float hw = width / 2.0f;
        float hh = height / 2.0f;

        // 좌상, 우상, 우하, 좌하 순서 (Index Buffer와 매칭)
        vertexData.push_back({ x - hw, y + hh, r, g, b, a });
        vertexData.push_back({ x + hw, y + hh, r, g, b, a });
        vertexData.push_back({ x + hw, y - hh, r, g, b, a });
        vertexData.push_back({ x - hw, y - hh, r, g, b, a });
    }
};

float myColor[3] = { 0.0f, 1.0f, 0.0f };
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

// ========================================
// 초기화 함수들
// ========================================

// 1. 윈도우 생성
HWND CreateAppWindow(HINSTANCE hInstance)
{
    WNDCLASSEXW winClass = { sizeof(WNDCLASSEXW) };
    winClass.style = CS_HREDRAW | CS_VREDRAW;
    winClass.lpfnWndProc = &WndProc;
    winClass.hInstance = hInstance;
    winClass.hCursor = LoadCursorW(0, IDC_ARROW);
    winClass.lpszClassName = L"D3D12WindowClass";

    if (!RegisterClassExW(&winClass)) return nullptr;

    // 실제 그리고 싶은 영역(Client Area)을 1280x720으로 맞추기 위해 창 전체 크기를 계산
    RECT initialRect = { 0, 0, 1280, 720 };
    AdjustWindowRectEx(&initialRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);

    return CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, winClass.lpszClassName, L"D3D12 Triangle",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        initialRect.right - initialRect.left, initialRect.bottom - initialRect.top,
        0, 0, hInstance, 0);
}

// 2. 디버그 레이어 활성화
void EnableDebugLayer()
{
#if defined(DEBUG_BUILD)
    ID3D12Debug* debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&debugController)))
    {
        debugController->EnableDebugLayer();
        debugController->Release();
    }
#endif
}

// 3. D3D12 디바이스 생성
ID3D12Device* CreateD3D12Device()
{
    IDXGIFactory4* dxgiFactory;
    CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&dxgiFactory);

    IDXGIAdapter1* selectedAdapter = nullptr;

    // 시스템에 장착된 여러 GPU 중 가장 성능이 좋은 하드웨어 GPU를 찾음
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

    ID3D12Device* device;
    D3D12CreateDevice(selectedAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&device);

    selectedAdapter->Release();
    dxgiFactory->Release();

    return device;
}

// 4. 커맨드 큐 생성
ID3D12CommandQueue* CreateCommandQueue(ID3D12Device* device)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ID3D12CommandQueue* commandQueue;
    device->CreateCommandQueue(&desc, __uuidof(ID3D12CommandQueue), (void**)&commandQueue);

    return commandQueue;
}

// 5. 스왑체인 생성
IDXGISwapChain3* CreateSwapChain(ID3D12CommandQueue* commandQueue, HWND hwnd)
{
    IDXGIFactory4* dxgiFactory;
    CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&dxgiFactory);

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = FRAME_BUFFER_COUNT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    IDXGISwapChain1* tempChain;
    dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &desc, nullptr, nullptr, &tempChain);

    IDXGISwapChain3* swapChain;
    tempChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapChain);
    tempChain->Release();
    dxgiFactory->Release();

    return swapChain;
}

// 6. RTV 디스크립터 힙 생성
ID3D12DescriptorHeap* CreateRTVDescriptorHeap(ID3D12Device* device, UINT& outDescriptorSize)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = FRAME_BUFFER_COUNT;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    ID3D12DescriptorHeap* rtvHeap;
    device->CreateDescriptorHeap(&desc, __uuidof(ID3D12DescriptorHeap), (void**)&rtvHeap);

    outDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    return rtvHeap;
}

// 7. 프레임 버퍼 생성
void CreateFrameBuffers(ID3D12Device* device, IDXGISwapChain3* swapChain, 
                        ID3D12DescriptorHeap* rtvHeap, UINT rtvDescriptorSize,
                        ID3D12Resource* frameBuffers[FRAME_BUFFER_COUNT])
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++) {
        swapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&frameBuffers[i]);
        device->CreateRenderTargetView(frameBuffers[i], nullptr, rtvHandle);
        rtvHandle.ptr += rtvDescriptorSize;
    }
}

// 8. 커맨드 알로케이터 및 리스트 생성
void CreateCommandAllocatorsAndList(ID3D12Device* device,
                                     ID3D12CommandAllocator* commandAllocators[FRAME_BUFFER_COUNT],
                                     ID3D12GraphicsCommandList** outCommandList)
{
    for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++) {
        device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
                                       __uuidof(ID3D12CommandAllocator), 
                                       (void**)&commandAllocators[i]);
    }

    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0], 
                              nullptr, __uuidof(ID3D12GraphicsCommandList), 
                              (void**)outCommandList);
    (*outCommandList)->Close();
}

// 9. Fence 및 이벤트 생성
void CreateSyncObjects(ID3D12Device* device, ID3D12Fence** outFence, HANDLE* outEvent)
{
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)outFence);
    *outEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

// 10. 셰이더 컴파일
void CompileShaders(ID3DBlob** outVSBlob, ID3DBlob** outPSBlob)
{
    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "vs_main", "vs_5_1", 0, 0, outVSBlob, nullptr);
    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "ps_main", "ps_5_1", 0, 0, outPSBlob, nullptr);
}

// 11. 루트 시그니처 생성
ID3D12RootSignature* CreateRootSignature(ID3D12Device* device)
{

    // D3D12 기본 구조체 직접 사용 (헬퍼 헤더 불필요)
    D3D12_ROOT_PARAMETER rootParameter = {};
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // 픽셀 셰이더용
    rootParameter.Constants.Num32BitValues = 4; // float4 (RGBA)
    rootParameter.Constants.ShaderRegister = 0; // b0
    rootParameter.Constants.RegisterSpace = 0;

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = 1;
    desc.pParameters = &rootParameter;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* sigBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    // 이 함수는 d3d12.h에 기본 포함되어 있습니다.
    D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob);

    if (errorBlob) {
        OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        errorBlob->Release();
    }

    ID3D12RootSignature* rootSignature = nullptr;
    device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

    if (sigBlob) sigBlob->Release();

    return rootSignature;

}

// 12. 파이프라인 상태 객체 생성
ID3D12PipelineState* CreatePipelineState(ID3D12Device* device, ID3D12RootSignature* rootSignature,
                                          ID3DBlob* vsBlob, ID3DBlob* psBlob)
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
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME; // 와이어프레임 모드 / 기본값: D3D12_FILL_MODE_SOLID
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 양면 렌더링 NONE/BACK/FRONT / 기본값: D3D12_CULL_MODE_BACK
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    ID3D12PipelineState* pipelineState;
    device->CreateGraphicsPipelineState(&psoDesc, __uuidof(ID3D12PipelineState), (void**)&pipelineState);

    return pipelineState;
}

// 13. 정점 버퍼 생성
void CreateVertexBuffer(ID3D12Device* device, UINT maxVertexBufferSize,
                        ID3D12Resource** outVertexBuffer, D3D12_VERTEX_BUFFER_VIEW* outVBView)
{
    D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_RESOURCE_DESC res = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, maxVertexBufferSize, 1, 1, 1, 
                                DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, 
                                D3D12_RESOURCE_FLAG_NONE };
    device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &res, 
                                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, 
                                    __uuidof(ID3D12Resource), (void**)outVertexBuffer);

    outVBView->BufferLocation = (*outVertexBuffer)->GetGPUVirtualAddress();
    outVBView->StrideInBytes = sizeof(Rect::Vertex);
    outVBView->SizeInBytes = maxVertexBufferSize;
}

// 14. 인덱스 버퍼 생성
void CreateIndexBuffer(ID3D12Device* device, UINT maxIndexBufferSize,
                       ID3D12Resource** outIndexBuffer, D3D12_INDEX_BUFFER_VIEW* outIBView)
{
    D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_RESOURCE_DESC res = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, maxIndexBufferSize, 1, 1, 1, 
                                DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, 
                                D3D12_RESOURCE_FLAG_NONE };
    device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &res, 
                                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, 
                                    __uuidof(ID3D12Resource), (void**)outIndexBuffer);

    // 인덱스는 최대치까지 미리 채워둠
    std::vector<uint16_t> indices;
    for (UINT i = 0; i < MAX_RECT_COUNT; ++i) {
        uint16_t base = (uint16_t)(i * 4);
        indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
    }
    void* iData;
    (*outIndexBuffer)->Map(0, nullptr, &iData);
    memcpy(iData, indices.data(), indices.size() * sizeof(uint16_t));
    (*outIndexBuffer)->Unmap(0, nullptr);

    outIBView->BufferLocation = (*outIndexBuffer)->GetGPUVirtualAddress();
    outIBView->Format = DXGI_FORMAT_R16_UINT;
    outIBView->SizeInBytes = maxIndexBufferSize;
}

// 15. 게임 오브젝트 초기화
void InitializeGameObjects(std::vector<Rect>& rects, std::mt19937& rng, int initialCount)
{
    for (int i = 0; i < initialCount; ++i) {
        Rect r;
        r.Initialize(rng);
        rects.push_back(r);
    }
}

// ========================================
// 렌더링 함수들
// ========================================

// 정점 데이터 업데이트
void UpdateVertexBuffer(const std::vector<Rect>& rects, ID3D12Resource* vertexBuffer)
{
    std::vector<Rect::Vertex> vDataList;
    vDataList.reserve(rects.size() * 4);
    for (const auto& r : rects) {
        r.Render(vDataList);
    }

    void* mappedPtr;
    vertexBuffer->Map(0, nullptr, &mappedPtr);
    memcpy(mappedPtr, vDataList.data(), vDataList.size() * sizeof(Rect::Vertex));
    vertexBuffer->Unmap(0, nullptr);
}

// 프레임 렌더링
void RenderFrame(IDXGISwapChain3* swapChain, ID3D12CommandAllocator* commandAllocators[FRAME_BUFFER_COUNT],
                 ID3D12GraphicsCommandList* commandList, ID3D12PipelineState* pipelineState,
                 ID3D12Resource* frameBuffers[FRAME_BUFFER_COUNT], ID3D12DescriptorHeap* rtvHeap,
                 UINT rtvDescriptorSize, ID3D12RootSignature* rootSignature,
                 const D3D12_VERTEX_BUFFER_VIEW* vbView, const D3D12_INDEX_BUFFER_VIEW* ibView,
                 HWND hwnd, UINT rectCount)
{
    UINT backBufferIdx = swapChain->GetCurrentBackBufferIndex();
    commandAllocators[backBufferIdx]->Reset();
    commandList->Reset(commandAllocators[backBufferIdx], pipelineState);

    // 리소스 상태 전환: Present -> RenderTarget
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = frameBuffers[backBufferIdx];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    // 화면 클리어
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += backBufferIdx * rtvDescriptorSize;
    float clearColor[] = { 0.05f, 0.05f, 0.1f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // 뷰포트 및 시저렉트 설정
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    D3D12_VIEWPORT vp = { 0, 0, (float)clientRect.right, (float)clientRect.bottom, 0, 1 };
    D3D12_RECT scissor = { 0, 0, clientRect.right, clientRect.bottom };

    commandList->RSSetViewports(1, &vp);
    commandList->RSSetScissorRects(1, &scissor);
    commandList->SetGraphicsRootSignature(rootSignature);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->SetGraphicsRoot32BitConstants(0, 4, myColor, 0);

    commandList->DrawIndexedInstanced(rectCount * 6, 1, 0, 0, 0);
    // 버퍼 바인딩 및 그리기
    commandList->IASetIndexBuffer(ibView);
    commandList->IASetVertexBuffers(0, 1, vbView);
    commandList->DrawIndexedInstanced(rectCount * 6, 1, 0, 0, 0);

    // 리소스 상태 전환: RenderTarget -> Present
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList->ResourceBarrier(1, &barrier);

    commandList->Close();
}

// GPU 동기화 - CPU가 GPU 작업 완료를 기다림
void WaitForGPU(ID3D12CommandQueue* commandQueue, ID3D12Fence* fence, UINT64& fenceValue, HANDLE fenceEvent)
{
	UINT64 waitValue = ++fenceValue; // 다음 대기할 펜스 값
    commandQueue->Signal(fence, waitValue);
    if (fence->GetCompletedValue() < waitValue) {
        fence->SetEventOnCompletion(waitValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

// ========================================
// WinMain - 진입점
// ========================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nShowCmd*/)
{
    // 윈도우 생성
    HWND hwnd = CreateAppWindow(hInstance);
    if (!hwnd) return 0;

    // 시스템 초기화
    Input::Initialize();
    Time::Initialize();

    // D3D12 초기화
    EnableDebugLayer();
    ID3D12Device* d3d12Device = CreateD3D12Device();
    ID3D12CommandQueue* commandQueue = CreateCommandQueue(d3d12Device);
    IDXGISwapChain3* swapChain = CreateSwapChain(commandQueue, hwnd);

    // 렌더 타겟 설정
    UINT rtvDescriptorSize;
    ID3D12DescriptorHeap* rtvHeap = CreateRTVDescriptorHeap(d3d12Device, rtvDescriptorSize);
    ID3D12Resource* frameBuffers[FRAME_BUFFER_COUNT];
    CreateFrameBuffers(d3d12Device, swapChain, rtvHeap, rtvDescriptorSize, frameBuffers);

    // 커맨드 리스트 설정
    ID3D12CommandAllocator* commandAllocators[FRAME_BUFFER_COUNT];
    ID3D12GraphicsCommandList* commandList;
    CreateCommandAllocatorsAndList(d3d12Device, commandAllocators, &commandList);

    // 동기화 객체
    ID3D12Fence* fence;
    HANDLE fenceEvent;
    UINT64 fenceValue = 0;
    CreateSyncObjects(d3d12Device, &fence, &fenceEvent);

    // 셰이더 및 파이프라인
    ID3DBlob* vsBlob, * psBlob;
    CompileShaders(&vsBlob, &psBlob);
    ID3D12RootSignature* rootSignature = CreateRootSignature(d3d12Device);
    ID3D12PipelineState* pipelineState = CreatePipelineState(d3d12Device, rootSignature, vsBlob, psBlob);

    // 버퍼 생성
    UINT maxVertexBufferSize = MAX_RECT_COUNT * 4 * sizeof(Rect::Vertex);
    UINT maxIndexBufferSize = MAX_RECT_COUNT * 6 * sizeof(uint16_t);
    ID3D12Resource* vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vbView;
    CreateVertexBuffer(d3d12Device, maxVertexBufferSize, &vertexBuffer, &vbView);
    ID3D12Resource* indexBuffer;
    D3D12_INDEX_BUFFER_VIEW ibView;
    CreateIndexBuffer(d3d12Device, maxIndexBufferSize, &indexBuffer, &ibView);

    // 게임 오브젝트 초기화
    std::vector<Rect> rects;
    std::mt19937 rng(1337);
    InitializeGameObjects(rects, rng, 5);

    // 메인 루프
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // 메시지 처리
        if (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            continue;
        }

        // 시간 및 입력 업데이트
        Time::Update();
        Input::Update();
        float dt = Time::GetDeltaTime();

        // 입력 처리 - Q키로 사각형 추가
        if (Input::GetKeyDown(eKeyCode::Q) && rects.size() < MAX_RECT_COUNT) {
            Rect r;
            r.Initialize(rng);
            rects.push_back(r);
        }

        if (Input::GetKeyDown(eKeyCode::K))
        {
            myColor[0] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            myColor[1] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			myColor[2] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        }
        // 게임 오브젝트 업데이트
        for (auto& r : rects) {
            r.Update(dt);
        }

        // 정점 버퍼 업데이트
        UpdateVertexBuffer(rects, vertexBuffer);

        // 렌더링
        RenderFrame(swapChain, commandAllocators, commandList, pipelineState, frameBuffers,
                    rtvHeap, rtvDescriptorSize, rootSignature, &vbView, &ibView, hwnd, (UINT)rects.size());

        // GPU에 명령 전송
        ID3D12CommandList* lists[] = { commandList };
        commandQueue->ExecuteCommandLists(1, lists);

        // 화면 표시
        swapChain->Present(1, 0);

        // GPU 동기화
        WaitForGPU(commandQueue, fence, fenceValue, fenceEvent);
    }

    return 0;
}