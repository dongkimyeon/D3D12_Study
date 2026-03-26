#include "stdafx.h"
#include "Framework.h"
#include "SceneManager.h"
#include "TestScene.h" // 씬을 직접 등록하기 위해 포함
#include "LoadScene.h" // 씬 등록 함수 포함
ComPtr<ID3D12Device> Framework::mDevice = nullptr;

Framework::Framework(int width, int height)
    : mWindowWidth(width), mWindowHeight(height)
{
}

Framework::~Framework() {
    Release();
}

void Framework::Initialize(HWND hwnd)
{
    mHwnd = hwnd;

#if defined(DEBUG) || defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
    }
#endif

    InitDirect3D();
    CreateCommandQueueAndList();
    CreateSwapChain();
    CreateRtvDsvDescriptorHeap();
    CreateFrameBuffers();
    CreateDepthStencilView();
    CreateSyncObjects();
    CompileShaders();
    CreateImGuiSrvHeap();

    // ImGui 통합 객체 초기화
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(mHwnd);
    ImGui_ImplDX12_Init(mDevice.Get(), FRAME_BUFFER_COUNT,
        DXGI_FORMAT_R8G8B8A8_UNORM, mImGuiSrvHeap.Get(),
        mImGuiSrvHeap->GetCPUDescriptorHandleForHeapStart(),
        mImGuiSrvHeap->GetGPUDescriptorHandleForHeapStart());
    ImGui::GetIO().Fonts->Build();
    ImGui_ImplDX12_CreateDeviceObjects();

    // 시스템 초기화
    Time::Initialize();
    Input::Initialize();
    SceneManager::Initialize();


	LoadScenes();
    isRunning = true;
}

void Framework::Run()
{
    MSG msg = {};
    while (isRunning)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Update();
            Render();
        }
    }
}

void Framework::Update()
{
    Time::Update();
    Input::Update();
    Camera::Update(Time::GetDeltaTime());
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    SceneManager::Update();
}

void Framework::Render()
{
    UINT backBufferIdx = mSwapChain->GetCurrentBackBufferIndex();
    mCommandAllocators[backBufferIdx]->Reset();
    mCommandList->Reset(mCommandAllocators[backBufferIdx].Get(), mPipelineState.Get());

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = mFrameBuffers[backBufferIdx].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    mCommandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += backBufferIdx * mRtvDescriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();

    float clearColor[] = { 0.05f, 0.05f, 0.1f, 1.0f };
    mCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    D3D12_VIEWPORT vp = { 0.0f, 0.0f, (float)mWindowWidth, (float)mWindowHeight, 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, mWindowWidth, mWindowHeight };
    mCommandList->RSSetViewports(1, &vp);
    mCommandList->RSSetScissorRects(1, &scissor);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ============================================
    SceneManager::Render(mCommandList);
    // ============================================

    // ImGui 렌더 커맨드 기록
    ImGui::Render();
    ID3D12DescriptorHeap* descriptorHeaps[] = { mImGuiSrvHeap.Get() };
    mCommandList->SetDescriptorHeaps(1, descriptorHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    mCommandList->ResourceBarrier(1, &barrier);

    mCommandList->Close();
    ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, ppCommandLists);

    mSwapChain->Present(1, 0);
    WaitForGPU();
}

void Framework::WaitForGPU()
{
    UINT64 waitValue = ++mFenceValue;
    mCommandQueue->Signal(mFence.Get(), waitValue);
    if (mFence->GetCompletedValue() < waitValue)
    {
        mFence->SetEventOnCompletion(waitValue, mFenceEvent);
        WaitForSingleObject(mFenceEvent, INFINITE);
    }
}

void Framework::Release()
{
    WaitForGPU();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    SceneManager::Release();
}

void Framework::InitDirect3D()
{
    CreateDXGIFactory1(IID_PPV_ARGS(&mDxgiFactory));
    IDXGIAdapter1* adapter = nullptr;
    for (UINT i = 0; mDxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc; adapter->GetDesc1(&desc);
        if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) break;
        adapter->Release();
    }
    D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice));
    if (adapter) adapter->Release();
}

void Framework::CreateCommandQueueAndList()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 1 };
    mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));

    for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
        mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocators[i]));

    mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&mCommandList));
    mCommandList->Close();
}

void Framework::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = mWindowWidth;
    swapChainDesc.Height = mWindowHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    ComPtr<IDXGISwapChain1> swapChain1;
    mDxgiFactory->CreateSwapChainForHwnd(mCommandQueue.Get(), mHwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
    swapChain1.As(&mSwapChain);
}

void Framework::CreateRtvDsvDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAME_BUFFER_COUNT, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
    mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap));
    mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
    mDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap));
}

void Framework::CreateFrameBuffers()
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mFrameBuffers[i]));
        mDevice->CreateRenderTargetView(mFrameBuffers[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += mRtvDescriptorSize;
    }
}

void Framework::CreateDepthStencilView()
{
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Width = mWindowWidth; resDesc.Height = mWindowHeight;
    resDesc.DepthOrArraySize = 1; resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_D32_FLOAT;
    resDesc.SampleDesc.Count = 1; resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT };
    D3D12_CLEAR_VALUE clearValue = { DXGI_FORMAT_D32_FLOAT, {1.0f, 0} };
    mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&mDepthStencilBuffer));
    mDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Framework::CreateSyncObjects()
{
    mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void Framework::CompileShaders()
{
    ComPtr<ID3DBlob> vsBlob, psBlob, errBlob;
    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "vs_main", "vs_5_1", 0, 0, &vsBlob, &errBlob);
    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "ps_main", "ps_5_1", 0, 0, &psBlob, &errBlob);

    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParam.Constants.Num32BitValues = 20; // 4 + 16
    rootParam.Constants.ShaderRegister = 0;

    D3D12_ROOT_SIGNATURE_DESC sigDesc = { 1, &rootParam, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };
    ComPtr<ID3DBlob> sigBlob;
    D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    mDevice->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));

    D3D12_INPUT_ELEMENT_DESC layout[] = {
        { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { layout, 3 };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;


    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    // ===========================================

    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState));
}

void Framework::CreateImGuiSrvHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
    mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mImGuiSrvHeap));
}