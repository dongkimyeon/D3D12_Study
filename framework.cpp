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

	// --- 1. 렌더 타겟 핸들 준비 ---
	// MSAA용 핸들 (힙의 끝부분에 위치함)
	D3D12_CPU_DESCRIPTOR_HANDLE msaaRtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	msaaRtvHandle.ptr += FRAME_BUFFER_COUNT * mRtvDescriptorSize;

	D3D12_CPU_DESCRIPTOR_HANDLE msaaDsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	msaaDsvHandle.ptr += mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// 스왑 체인 핸들 (최종 출력용)
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	backBufferHandle.ptr += backBufferIdx * mRtvDescriptorSize;

	// --- 2. MSAA 버퍼에 그리기 ---
	float clearColor[] = { 0.05f, 0.05f, 0.1f, 1.0f };
	mCommandList->ClearRenderTargetView(msaaRtvHandle, clearColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(msaaDsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 렌더 타겟을 MSAA 버퍼로 설정
	mCommandList->OMSetRenderTargets(1, &msaaRtvHandle, FALSE, &msaaDsvHandle);

	// (기존 뷰포트 설정 및 렌더링 호출...)
	SceneManager::Render(mCommandList);

	// --- 3. Resolve (MSAA -> 스왑체인) ---
	D3D12_RESOURCE_BARRIER barriers[2] = {};

	// 1. MSAA 버퍼를 Resolve Source로, 스왑체인을 Resolve Dest로 변경
	barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[0].Transition.pResource = mMsaaRenderTarget.Get();
	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[1].Transition.pResource = mFrameBuffers[backBufferIdx].Get();
	barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_DEST;
	barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	mCommandList->ResourceBarrier(2, barriers);

	// [수정된 부분] 실제 그림 압축해서 복사하는 명령!
	mCommandList->ResolveSubresource(
		mFrameBuffers[backBufferIdx].Get(), 0,
		mMsaaRenderTarget.Get(), 0,
		DXGI_FORMAT_R8G8B8A8_UNORM
	);

	// [수정된 부분] 다시 원래 상태로 되돌리기
	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_DEST;
	barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	mCommandList->ResourceBarrier(2, barriers);

	// --- 4. ImGui 등 후처리 (이미 1개 샘플인 스왑체인에 그리기) ---
	// 백버퍼를 PRESENT → RENDER_TARGET 으로 전환 (ImGui 렌더링 전)
	D3D12_RESOURCE_BARRIER imguiBarrier = {};
	imguiBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	imguiBarrier.Transition.pResource = mFrameBuffers[backBufferIdx].Get();
	imguiBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	imguiBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	imguiBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	mCommandList->ResourceBarrier(1, &imguiBarrier);

	mCommandList->OMSetRenderTargets(1, &backBufferHandle, FALSE, nullptr);
	ImGui::Render();
	ID3D12DescriptorHeap* descriptorHeaps[] = { mImGuiSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(1, descriptorHeaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

	// 후처리가 끝났으니 스왑체인을 다시 렌더타겟에서 Present 상태로 변경
	D3D12_RESOURCE_BARRIER presentBarrier = {};
	presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	presentBarrier.Transition.pResource = mFrameBuffers[backBufferIdx].Get();
	presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET; // ImGui 그리느라 상태가 렌더타겟이 됨
	presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	presentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	mCommandList->ResourceBarrier(1, &presentBarrier);

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
    if(FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&mDxgiFactory))))
	{
		std::cout << "DXGI Factory Failed" << std::endl;
		exit(1);
	}
	else
	{
		std::cout << "DXGI Factory Succeeded" << std::endl;
	}
	
    IDXGIAdapter1* adapter = nullptr;
    for (UINT i = 0; mDxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
        if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) break;
        adapter->Release();
    }

    HRESULT hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice));
    if (FAILED(hr))
    {
        std::cout << "Failed to create D3D12 device" << std::endl;
        if (adapter) adapter->Release();
        exit(1);
    }

    if (adapter) adapter->Release();
}

void Framework::CreateCommandQueueAndList()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 1 };
    HRESULT hr = mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));
    if (FAILED(hr))
    {
        std::cout << "Failed to create command queue" << std::endl;
        exit(1);
    }

    for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
    {
        hr = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocators[i]));
        if (FAILED(hr))
        {
            std::cout << "Failed to create command allocator " << i << std::endl;
            exit(1);
        }
    }

    hr = mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&mCommandList));
    if (FAILED(hr))
    {
        std::cout << "Failed to create command list" << std::endl;
        exit(1);
    }

    mCommandList->Close();
}
void Framework::CreateSwapChain()
{
	// MSAA 지원 체크
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels = { DXGI_FORMAT_R8G8B8A8_UNORM, 4, D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE, 0 };
	HRESULT hr = mDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaQualityLevels, sizeof(msaaQualityLevels));
	m_nMsaa4xQualityLevels = msaaQualityLevels.NumQualityLevels;

	if (FAILED(hr) || m_nMsaa4xQualityLevels == 0)
	{
		std::cout << "GPU does not support 4x MSAA" << std::endl;
		exit(1);
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = mWindowWidth;
	swapChainDesc.Height = mWindowHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;      // 스왑 체인은 항상 1
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	ComPtr<IDXGISwapChain1> swapChain1;
	hr = mDxgiFactory->CreateSwapChainForHwnd(mCommandQueue.Get(), mHwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
	if (FAILED(hr))
	{
		std::cout << "Failed to create swap chain" << std::endl;
		exit(1);
	}

	hr = swapChain1.As(&mSwapChain);
	if (FAILED(hr))
	{
		std::cout << "Failed to cast swap chain" << std::endl;
		exit(1);
	}
}

void Framework::CreateRtvDsvDescriptorHeap()
{
	// RTV: 스왑체인용(2개) + MSAA용(1개) = 총 3개
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAME_BUFFER_COUNT + 1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
	HRESULT hr = mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap));
	if (FAILED(hr))
	{
		std::cout << "Failed to create RTV descriptor heap" << std::endl;
		exit(1);
	}

	mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// DSV: 기본(1개) + MSAA용(1개) = 총 2개
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
	hr = mDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap));
	if (FAILED(hr))
	{
		std::cout << "Failed to create DSV descriptor heap" << std::endl;
		exit(1);
	}
}

void Framework::CreateFrameBuffers()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		HRESULT hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mFrameBuffers[i]));
		if (FAILED(hr))
		{
			std::cout << "Failed to get swap chain buffer " << i << std::endl;
			exit(1);
		}

		mDevice->CreateRenderTargetView(mFrameBuffers[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += mRtvDescriptorSize;
	}

	D3D12_RESOURCE_DESC msaaDesc = mFrameBuffers[0]->GetDesc();
	msaaDesc.SampleDesc.Count = 4;
	msaaDesc.SampleDesc.Quality = m_nMsaa4xQualityLevels - 1;

	D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT };

	// [수정된 부분] View를 만들기 전에 실제 리소스(메모리)를 먼저 생성해야 합니다!
	HRESULT hr = mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &msaaDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&mMsaaRenderTarget));
	if (FAILED(hr))
	{
		std::cout << "Failed to create MSAA render target" << std::endl;
		exit(1);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE msaaRtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	msaaRtvHandle.ptr += FRAME_BUFFER_COUNT * mRtvDescriptorSize;

	mDevice->CreateRenderTargetView(mMsaaRenderTarget.Get(), nullptr, msaaRtvHandle);
}

void Framework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC dsDesc = {};
	dsDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	dsDesc.Width = mWindowWidth; dsDesc.Height = mWindowHeight;
	dsDesc.DepthOrArraySize = 1; dsDesc.MipLevels = 1;
	dsDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsDesc.SampleDesc.Count = 4;
	dsDesc.SampleDesc.Quality = m_nMsaa4xQualityLevels - 1;
	dsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = { DXGI_FORMAT_D32_FLOAT, {1.0f, 0} };
	D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT };

	// [수정된 부분] 깊이 버퍼 리소스 생성 추가!
	HRESULT hr = mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &dsDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&mMsaaDepthStencil));
	if (FAILED(hr))
	{
		std::cout << "Failed to create MSAA depth stencil" << std::endl;
		exit(1);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE msaaDsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	msaaDsvHandle.ptr += mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	mDevice->CreateDepthStencilView(mMsaaDepthStencil.Get(), nullptr, msaaDsvHandle);
}

void Framework::CreateSyncObjects()
{
    mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void Framework::CompileShaders()
{
    ComPtr<ID3DBlob> vsBlob, psBlob, errBlob;
    HRESULT hr = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "vs_main", "vs_5_1", 0, 0, &vsBlob, &errBlob);
    if (FAILED(hr))
    {
        if (errBlob) std::cout << "Vertex Shader Error: " << (char*)errBlob->GetBufferPointer() << std::endl;
        std::cout << "Failed to compile vertex shader" << std::endl;
        exit(1);
    }

    hr = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "ps_main", "ps_5_1", 0, 0, &psBlob, &errBlob);
    if (FAILED(hr))
    {
        if (errBlob) std::cout << "Pixel Shader Error: " << (char*)errBlob->GetBufferPointer() << std::endl;
        std::cout << "Failed to compile pixel shader" << std::endl;
        exit(1);
    }

    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParam.Constants.Num32BitValues = 20; // 4 + 16
    rootParam.Constants.ShaderRegister = 0;

    D3D12_ROOT_SIGNATURE_DESC sigDesc = { 1, &rootParam, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };
    ComPtr<ID3DBlob> sigBlob;
    hr = D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    if (FAILED(hr))
    {
        if (errBlob) std::cout << "Root Signature Error: " << (char*)errBlob->GetBufferPointer() << std::endl;
        std::cout << "Failed to serialize root signature" << std::endl;
        exit(1);
    }

    hr = mDevice->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
    if (FAILED(hr))
    {
        std::cout << "Failed to create root signature" << std::endl;
        exit(1);
    }

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
	psoDesc.SampleDesc.Count = 4; // MSAA 적용
	psoDesc.SampleDesc.Quality = m_nMsaa4xQualityLevels - 1;
	hr = mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState));
	if (FAILED(hr))
	{
		std::cout << "Failed to create graphics pipeline state" << std::endl;
		exit(1);
	}
}

void Framework::CreateImGuiSrvHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
    HRESULT hr = mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mImGuiSrvHeap));
    if (FAILED(hr))
    {
        std::cout << "Failed to create ImGui SRV heap" << std::endl;
        exit(1);
    }
}