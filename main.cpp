
#define DEBUG_BUILD         // 디버그 모드 설정을 위한 정의

#include "stdafx.h"


static bool global_windowDidResize = false;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 스왑 체인에 사용할 버퍼 개수. 보통 더블 버퍼링(2)이나 트리플 버퍼링(3)을 사용함.
const UINT FRAME_BUFFER_COUNT = 2;
const UINT MAX_RECT_COUNT = 100; // 최대 사각형 개수 제한 (버퍼 오버플로우 방지)

// 사각형의 상태를 관리하는 구조체

XMFLOAT3 camPos{ 0, 0, -10 };
float camYaw = 0.0f;   // 좌우 회전 (Y축 기준)
float camPitch = 0.0f; // 위아래 회전 (X축 기준)
float moveSpeed = 10.0f;
float lookSpeed = 0.006f; // 마우스 감도

std::default_random_engine rng(std::random_device{}());



class Mesh {
public:
    Mesh() {
        position.x = 0;
        position.y = 0;
        position.z = 0;
		color = { 1.0f, 1.0f, 1.0f, 1.0f };
    }

    std::vector<OBJVertex> vertices;
    std::vector<uint16_t> indices;
    XMMATRIX worldMatrix;
    std::array<float, 4> color;


    void LoadFromOBJ(const std::string& filename) {
        OBJLoader::Load(filename, vertices, indices);
        worldMatrix = XMMatrixIdentity();
    }
    void SetPosition(float newX, float newY, float newZ) {
        position.x = newX;
        position.y = newY;
        position.z  = newZ;

        worldMatrix = XMMatrixTranslation(position.x, position.y, position.z);
    }


    void Update(float dt) {
        // 회전 애니메이션
        //worldMatrix = XMMatrixRotationY(dt * 0.5f) * worldMatrix;
    }
private:
    XMFLOAT3 position;
    
};


// 윈도우 메시지 처리기 (콜백 함수)
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    // ImGui가 입력을 가로채도록 추가
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;

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
    D3D12_ROOT_PARAMETER rootParameter = {};
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameter.Constants.Num32BitValues = 20; // 4(color) + 16(matrix)
    rootParameter.Constants.ShaderRegister = 0;
    rootParameter.Constants.RegisterSpace = 0;

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = 1;
    desc.pParameters = &rootParameter;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* sigBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
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
        { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { layout, 3 };  // 3개로 변경
    psoDesc.pRootSignature = rootSignature;
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;  // SOLID로 변경
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;   // BACK으로 변경
    psoDesc.RasterizerState.DepthClipEnable = TRUE;  // 추가
    psoDesc.DepthStencilState.DepthEnable = FALSE;    // 깊이 버퍼 활성화
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;  // 깊이 버퍼 포맷
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
    outVBView->StrideInBytes = sizeof(OBJVertex);
    outVBView->SizeInBytes = maxVertexBufferSize;
}

// 14. 인덱스 버퍼 생성
void CreateIndexBuffer(ID3D12Device* device, const std::vector<uint16_t>& indices,
                       ID3D12Resource** outIndexBuffer, D3D12_INDEX_BUFFER_VIEW* outIBView)
{
    UINT maxIndexBufferSize = static_cast<UINT>(indices.size() * sizeof(uint16_t));
    
    D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_RESOURCE_DESC res = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, maxIndexBufferSize, 1, 1, 1, 
                                DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, 
                                D3D12_RESOURCE_FLAG_NONE };
    device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &res, 
                                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, 
                                    __uuidof(ID3D12Resource), (void**)outIndexBuffer);

    // 인덱스 데이터 복사
    void* iData;
    (*outIndexBuffer)->Map(0, nullptr, &iData);
    memcpy(iData, indices.data(), indices.size() * sizeof(uint16_t));
    (*outIndexBuffer)->Unmap(0, nullptr);

    outIBView->BufferLocation = (*outIndexBuffer)->GetGPUVirtualAddress();
    outIBView->Format = DXGI_FORMAT_R16_UINT;
    outIBView->SizeInBytes = maxIndexBufferSize;
}

// 15. 깊이 스텐실 버퍼 생성
void CreateDepthStencilBuffer(ID3D12Device* device, UINT width, UINT height, 
                              ID3D12Resource** outDepthStencilBuffer, ID3D12DescriptorHeap** outDSVHeap)
{
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Width = width;
    resDesc.Height = height;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_D32_FLOAT;
    resDesc.SampleDesc.Count = 1;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;
    
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, 
                                    D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, 
                                    IID_PPV_ARGS(outDepthStencilBuffer));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(outDSVHeap));

    device->CreateDepthStencilView(*outDepthStencilBuffer, nullptr, 
                                   (*outDSVHeap)->GetCPUDescriptorHandleForHeapStart());
}

// 16. ImGui용 SRV 힙 생성 함수 추가
ID3D12DescriptorHeap* CreateSrvHeapForImGui(ID3D12Device* device)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ID3D12DescriptorHeap* srvHeap;
    device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap));
    return srvHeap;
}

// ========================================
// 렌더링 함수들
// ========================================

// 정점 데이터 업데이트
void UpdateVertexBuffer(const Mesh& mesh, ID3D12Resource* vertexBuffer)
{
    void* mappedPtr;
    vertexBuffer->Map(0, nullptr, &mappedPtr);
    memcpy(mappedPtr, mesh.vertices.data(), mesh.vertices.size() * sizeof(OBJVertex));
    vertexBuffer->Unmap(0, nullptr);
}

// 프레임 렌더링
void RenderFrame(IDXGISwapChain3* swapChain, ID3D12CommandAllocator* commandAllocators[FRAME_BUFFER_COUNT],
                 ID3D12GraphicsCommandList* commandList, ID3D12PipelineState* pipelineState,
                 ID3D12Resource* frameBuffers[FRAME_BUFFER_COUNT], ID3D12DescriptorHeap* rtvHeap,
                 UINT rtvDescriptorSize, ID3D12RootSignature* rootSignature,
                 const D3D12_VERTEX_BUFFER_VIEW* vbView, const D3D12_INDEX_BUFFER_VIEW* ibView,
                 HWND hwnd, const std::vector<Mesh>& all_obj, ID3D12Resource* depthStencilBuffer, ID3D12DescriptorHeap* dsvHeap,ID3D12DescriptorHeap* imguiSrvHeap)
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
    
    // 깊이 버퍼 클리어
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // 뷰포트 및 시저렉트 설정
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    D3D12_VIEWPORT vp = { 0, 0, (float)clientRect.right, (float)clientRect.bottom, 0, 1 };
    D3D12_RECT scissor = { 0, 0, clientRect.right, clientRect.bottom };

    commandList->RSSetViewports(1, &vp);
    commandList->RSSetScissorRects(1, &scissor);
    commandList->SetGraphicsRootSignature(rootSignature);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // RenderFrame 함수 내부의 뷰 행렬 계산 부분
    XMVECTOR currentPos = XMLoadFloat3(&camPos);

    // 현재 회전값 반영
    XMMATRIX camRot = XMMatrixRotationRollPitchYaw(camPitch, camYaw, 0);
    XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), camRot);

    // LookAt 대신 LookTo를 사용하세요!
    XMMATRIX view = XMMatrixLookToLH(
        currentPos,              // 카메라 위치
        forward,                 // 바라보는 방향 벡터
        XMVectorSet(0, 1, 0, 0)  // 위쪽 방향 (월드 Up)
    );
    XMMATRIX proj = XMMatrixPerspectiveFovLH(
        XM_PIDIV4,
        (float)clientRect.right / clientRect.bottom,
        0.1f,
        100.0f
    );
   
    XMMATRIX mvp;
    for (const auto mesh : all_obj) {
        mvp = mesh.worldMatrix* view* proj;
    }

    XMMATRIX mvpTranspose = XMMatrixTranspose(mvp);

   

    commandList->SetGraphicsRoot32BitConstants(0, 16, &mvpTranspose, 4);

    // 버퍼 바인딩 및 그리기
    commandList->IASetIndexBuffer(ibView);
    commandList->IASetVertexBuffers(0, 1, vbView);
    for (const auto& mesh : all_obj) {
        XMMATRIX mvp = mesh.worldMatrix * view * proj;
        XMFLOAT4X4 mvpFloat;
        XMStoreFloat4x4(&mvpFloat, XMMatrixTranspose(mvp)); // 행렬은 HLSL에서 Column-major(또는 셰이더 기대 형식)에 맞게 전치해서 전달

        // 컬러(4값) -> 루트 상수 0~3
        commandList->SetGraphicsRoot32BitConstants(0, 4, mesh.color.data(), 0);
        // 행렬(4x4 = 16값) -> 루트 상수 4~19
        commandList->SetGraphicsRoot32BitConstants(0, 16, &mvpFloat.m[0][0], 4);

        commandList->DrawIndexedInstanced(static_cast<UINT>(mesh.indices.size()), 1, 0, 0, 0);
    }
  
    //  ImGui 렌더링 명령 기록
    ID3D12DescriptorHeap* descriptorHeaps[] = { imguiSrvHeap };
    commandList->SetDescriptorHeaps(1, descriptorHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);


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

    // 깊이 버퍼 설정
    ID3D12Resource* depthStencilBuffer;
    ID3D12DescriptorHeap* dsvHeap;
    CreateDepthStencilBuffer(d3d12Device, 1280, 720, &depthStencilBuffer, &dsvHeap);

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


    // [추가] ImGui용 SRV 힙 생성
    ID3D12DescriptorHeap* imguiSrvHeap = CreateSrvHeapForImGui(d3d12Device);

    // [추가] ImGui 초기화
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(d3d12Device, FRAME_BUFFER_COUNT,
        DXGI_FORMAT_R8G8B8A8_UNORM, imguiSrvHeap,
        imguiSrvHeap->GetCPUDescriptorHandleForHeapStart(),
        imguiSrvHeap->GetGPUDescriptorHandleForHeapStart());

    // [수정] 폰트 아틀라스 빌드 추가
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Build();

  
    ImGui_ImplDX12_CreateDeviceObjects();
  
    // Mesh 초기화
    Mesh mesh;
    mesh.LoadFromOBJ("model.obj");  // OBJ 파일 경로
    Mesh temp;
    temp.LoadFromOBJ("model.obj");  // OBJ 파일 경로

    std::vector<Mesh> meshObjects;
    for (int i = 0; i < 10; ++i) {
		temp.color = { rng() % 256 / 255.0f, rng() % 256 / 255.0f, rng() % 256 / 255.0f, 1.0f }; // 예시 색상
        meshObjects.push_back(temp);
        meshObjects.back().SetPosition(i * 3.0f, 0.0f, 0.0f);
    }



    // 버퍼 크기 계산
    UINT maxVertexBufferSize = static_cast<UINT>(mesh.vertices.size() * sizeof(OBJVertex));
    
    // 버퍼 생성
    ID3D12Resource* vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vbView;
    CreateVertexBuffer(d3d12Device, maxVertexBufferSize, &vertexBuffer, &vbView);
    ID3D12Resource* indexBuffer;
    D3D12_INDEX_BUFFER_VIEW ibView;
    CreateIndexBuffer(d3d12Device, mesh.indices, &indexBuffer, &ibView);

    // 초기 정점 데이터 업로드
    UpdateVertexBuffer(mesh, vertexBuffer);

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
        ImGuiIO& io = ImGui::GetIO(); // ImGui IO 상태 가져오기

        static POINT prevMousePos;
        static bool isRotating = false;

      
        // 시간 및 입력 업데이트
        Time::Update();
        Input::Update();    
        // 메인 루프 내부 업데이트 섹션
        float dt = Time::GetDeltaTime();

        // 1. 마우스 우클릭 로직 (회전)
        if (!io.WantCaptureMouse && Input::GetKey(eKeyCode::LButton))
        {
            POINT currMousePos;
            GetCursorPos(&currMousePos);

            if (!isRotating) {
                prevMousePos = currMousePos;
                isRotating = true;
            }

            // 마우스 이동량(Delta) 계산
            float dx = static_cast<float>(currMousePos.x - prevMousePos.x);
            float dy = static_cast<float>(currMousePos.y - prevMousePos.y);

            camYaw += dx * lookSpeed;
            camPitch += dy * lookSpeed;

            const float limit = XM_PIDIV2 - 0.1f;
            if (camPitch > limit) camPitch = limit;
            if (camPitch < -limit) camPitch = -limit;

            prevMousePos = currMousePos;
        }
        else
        {
            isRotating = false;
        }

        // 2. 방향 벡터 계산
        XMMATRIX camRotation = XMMatrixRotationRollPitchYaw(camPitch, camYaw, 0);

        // 로컬 방향(0,0,1)을 회전 행렬로 변환하여 전방/우측 벡터 추출
        XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), camRotation);
        XMVECTOR right = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), camRotation);
        XMVECTOR up = XMVectorSet(0, 1, 0, 0); // 월드 기준 Up

        // 3. 키보드 이동 (바라보는 방향 기준)
        XMVECTOR pos = XMLoadFloat3(&camPos);

        if (Input::GetKey(eKeyCode::W)) pos += forward * moveSpeed * dt;
        if (Input::GetKey(eKeyCode::S)) pos -= forward * moveSpeed * dt;
        if (Input::GetKey(eKeyCode::D)) pos += right * moveSpeed * dt;
        if (Input::GetKey(eKeyCode::A)) pos -= right * moveSpeed * dt;
        if (Input::GetKey(eKeyCode::E)) pos += up * moveSpeed * dt;
        if (Input::GetKey(eKeyCode::Q)) pos -= up * moveSpeed * dt;

        XMStoreFloat3(&camPos, pos);
  
 

        // 메시 업데이트 (회전)
        mesh.Update(dt);
        for (auto& rect : meshObjects) {
            rect.Update(dt);
        }
        // [추가] ImGui 새 프레임 시작 및 UI 구성
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

   
        //  배경색 변경 컨트롤
        ImGui::Begin("Settings");
        //ImGui::ColorEdit3("Triangle Color", myColor); // myColor 변수를 ImGui에서 직접 조작
        //fps 표시
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate); 
        //카메라 위치 표시
		ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", camPos.x, camPos.y, camPos.z);  
        ImGui::End();

        ImGui::Render(); // 렌더링 데이터 생성

        // 렌더링 (매개변수로 imguiSrvHeap을 넘기도록 수정 필요)
        RenderFrame(swapChain, commandAllocators, commandList, pipelineState, frameBuffers,
            rtvHeap, rtvDescriptorSize, rootSignature, &vbView, &ibView, hwnd, meshObjects, depthStencilBuffer, dsvHeap, imguiSrvHeap);


      
        // GPU에 명령 전송
        ID3D12CommandList* lists[] = { commandList };
        commandQueue->ExecuteCommandLists(1, lists);

        // 화면 표시
        swapChain->Present(1, 0);

        // GPU 동기화
        WaitForGPU(commandQueue, fence, fenceValue, fenceEvent);
     

    }
    // [추가] ImGui 정리
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    return 0;
}