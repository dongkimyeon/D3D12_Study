#include "stdafx.h"
#include "Framework.h"
#include "SceneManager.h"
#include "TestScene.h"
#include "LoadScene.h"


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
		PrintLog(LogColor::CYAN, "[DEBUG] D3D12 Debug Layer Enabled");
	}
	else {
		PrintLog(LogColor::RED, "[DEBUG] Failed to enable D3D12 Debug Layer");
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

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	PrintLog(LogColor::YELLOW, "[ImGui] Context Created");
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(mHwnd);
	PrintLog(LogColor::YELLOW, "[ImGui] Win32 Backend Initialized");
	ImGui_ImplDX12_Init(mDevice.Get(), FRAME_BUFFER_COUNT,
		DXGI_FORMAT_R8G8B8A8_UNORM, mImGuiSrvHeap.Get(),
		mImGuiSrvHeap->GetCPUDescriptorHandleForHeapStart(),
		mImGuiSrvHeap->GetGPUDescriptorHandleForHeapStart());
	PrintLog(LogColor::YELLOW, "[ImGui] DX12 Backend Initialized");
	ImGui::GetIO().Fonts->Build();
	ImGui_ImplDX12_CreateDeviceObjects();
	PrintLog(LogColor::YELLOW, "[ImGui] Device Objects Created");

	Time::Initialize();
	PrintLog(LogColor::MAGENTA, "[System] Time Initialized");
	Input::Initialize();
	PrintLog(LogColor::MAGENTA, "[System] Input Initialized");
	SceneManager::Initialize();
	PrintLog(LogColor::MAGENTA, "[System] SceneManager Initialized");

	LoadScenes();
	PrintLog(LogColor::MAGENTA, "[System] Scenes Loaded");

	isRunning = true;
	PrintLog(LogColor::MAGENTA, "[Framework] Initialize Complete");
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

	D3D12_CPU_DESCRIPTOR_HANDLE msaaRtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	msaaRtvHandle.ptr += FRAME_BUFFER_COUNT * mRtvDescriptorSize;

	D3D12_CPU_DESCRIPTOR_HANDLE msaaDsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	msaaDsvHandle.ptr += mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	backBufferHandle.ptr += backBufferIdx * mRtvDescriptorSize;

	float clearColor[] = { 0.05f, 0.05f, 0.1f, 1.0f };
	mCommandList->ClearRenderTargetView(msaaRtvHandle, clearColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(msaaDsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	mCommandList->OMSetRenderTargets(1, &msaaRtvHandle, FALSE, &msaaDsvHandle);

	D3D12_VIEWPORT vp = { 0.0f, 0.0f, (float)mWindowWidth, (float)mWindowHeight, 0.0f, 1.0f };
	D3D12_RECT scissor = { 0, 0, mWindowWidth, mWindowHeight };
	mCommandList->RSSetViewports(1, &vp);
	mCommandList->RSSetScissorRects(1, &scissor);
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	SceneManager::Render(mCommandList);

	D3D12_RESOURCE_BARRIER barriers[2] = {};
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

	mCommandList->ResolveSubresource(
		mFrameBuffers[backBufferIdx].Get(), 0,
		mMsaaRenderTarget.Get(), 0,
		DXGI_FORMAT_R8G8B8A8_UNORM
	);

	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_DEST;
	barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	mCommandList->ResourceBarrier(2, barriers);

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

	D3D12_RESOURCE_BARRIER presentBarrier = {};
	presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	presentBarrier.Transition.pResource = mFrameBuffers[backBufferIdx].Get();
	presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
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
	PrintLog(LogColor::GRAY, "[Release] GPU Wait Complete");

	ImGui_ImplDX12_Shutdown();
	PrintLog(LogColor::GRAY	, "[Release] ImGui DX12 Shutdown");
	ImGui_ImplWin32_Shutdown();
	PrintLog(LogColor::GRAY, "[Release] ImGui Win32 Shutdown");
	ImGui::DestroyContext();
	PrintLog(LogColor::GRAY, "[Release] ImGui Context Destroyed");

	SceneManager::Release();
	PrintLog(LogColor::GRAY, "[Release] SceneManager Released");

#if defined(DEBUG) || defined(_DEBUG)
	IDXGIDebug1* pdxgiDebug = NULL;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void**)&pdxgiDebug)))
	{
		PrintLog(LogColor::GRAY, "[DEBUG] Reporting Live DXGI Objects...");
		pdxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
		pdxgiDebug->Release();
		PrintLog(LogColor::GRAY, "[DEBUG] Live Object Report Complete");
	}
	else
	{
		PrintLog(LogColor::RED, "[DEBUG] Failed to get DXGI Debug Interface");
	}
#endif
}

void Framework::InitDirect3D()
{
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&mDebugController))))
	{
		mDebugController->EnableDebugLayer();
		PrintLog(LogColor::CYAN, "[D3D12] Debug Layer Enabled");
	}
	else
	{
		PrintLog(LogColor::RED, "[D3D12] Failed to enable Debug Layer");
	}

	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&mDxgiFactory))))
	{
		PrintLog(LogColor::RED, "[D3D12] DXGI Factory Failed");
		exit(1);
	}
	PrintLog(LogColor::CYAN, "[D3D12] DXGI Factory Created");

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
		PrintLog(LogColor::RED, "[D3D12] Failed to create Device");
		if (adapter) adapter->Release();
		exit(1);
	}
	PrintLog(LogColor::CYAN, "[D3D12] Device Created (Feature Level 11.0)");
	if (adapter) adapter->Release();
}

void Framework::CreateCommandQueueAndList()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 1 };
	HRESULT hr = mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create Command Queue"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] Command Queue Created");

	for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
	{
		hr = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocators[i]));
		if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create Command Allocator " + std::to_string(i)); exit(1); }
	}
	PrintLog(LogColor::CYAN, "[D3D12] Command Allocators Created (" + std::to_string(FRAME_BUFFER_COUNT) + ")");

	hr = mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&mCommandList));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create Command List"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] Command List Created");

	mCommandList->Close();
}

void Framework::CreateSwapChain()
{
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels = { DXGI_FORMAT_R8G8B8A8_UNORM, 4, D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE, 0 };
	HRESULT hr = mDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaQualityLevels, sizeof(msaaQualityLevels));
	m_nMsaa4xQualityLevels = msaaQualityLevels.NumQualityLevels;

	if (FAILED(hr) || m_nMsaa4xQualityLevels == 0)
	{
		PrintLog(LogColor::RED, "[D3D12] GPU does not support 4x MSAA");
		exit(1);
	}
	PrintLog(LogColor::CYAN, "[D3D12] 4x MSAA Supported (Quality Levels: " + std::to_string(m_nMsaa4xQualityLevels) + ")");

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = mWindowWidth;
	swapChainDesc.Height = mWindowHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	ComPtr<IDXGISwapChain1> swapChain1;
	hr = mDxgiFactory->CreateSwapChainForHwnd(mCommandQueue.Get(), mHwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create Swap Chain"); exit(1); }

	hr = swapChain1.As(&mSwapChain);
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to cast Swap Chain"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] Swap Chain Created (" + std::to_string(mWindowWidth) + "x" + std::to_string(mWindowHeight) + ")");
}

void Framework::CreateRtvDsvDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAME_BUFFER_COUNT + 1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
	HRESULT hr = mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create RTV Descriptor Heap"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] RTV Descriptor Heap Created (" + std::to_string(FRAME_BUFFER_COUNT + 1) + " descriptors)");

	mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
	hr = mDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create DSV Descriptor Heap"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] DSV Descriptor Heap Created (2 descriptors)");
}

void Framework::CreateFrameBuffers()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		HRESULT hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mFrameBuffers[i]));
		if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to get Swap Chain Buffer " + std::to_string(i)); exit(1); }
		mDevice->CreateRenderTargetView(mFrameBuffers[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += mRtvDescriptorSize;
	}
	PrintLog(LogColor::CYAN, "[D3D12] Frame Buffers Created (" + std::to_string(FRAME_BUFFER_COUNT) + ")");

	D3D12_RESOURCE_DESC msaaDesc = mFrameBuffers[0]->GetDesc();
	msaaDesc.SampleDesc.Count = 4;
	msaaDesc.SampleDesc.Quality = m_nMsaa4xQualityLevels - 1;
	D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT };

	HRESULT hr = mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &msaaDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&mMsaaRenderTarget));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create MSAA Render Target"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] MSAA Render Target Created (4x)");

	D3D12_CPU_DESCRIPTOR_HANDLE msaaRtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	msaaRtvHandle.ptr += FRAME_BUFFER_COUNT * mRtvDescriptorSize;
	mDevice->CreateRenderTargetView(mMsaaRenderTarget.Get(), nullptr, msaaRtvHandle);
	PrintLog(LogColor::CYAN, "[D3D12] MSAA RTV Created");
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

	HRESULT hr = mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &dsDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&mMsaaDepthStencil));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create MSAA Depth Stencil"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] MSAA Depth Stencil Buffer Created (D32_FLOAT, 4x)");

	D3D12_CPU_DESCRIPTOR_HANDLE msaaDsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	msaaDsvHandle.ptr += mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mDevice->CreateDepthStencilView(mMsaaDepthStencil.Get(), nullptr, msaaDsvHandle);
	PrintLog(LogColor::CYAN, "[D3D12] MSAA DSV Created");
}

void Framework::CreateSyncObjects()
{
	HRESULT hr = mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create Fence"); exit(1); }
	mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!mFenceEvent) { PrintLog(LogColor::RED, "[D3D12] Failed to create Fence Event"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] Sync Objects Created (Fence + Event)");
}

void Framework::CompileShaders()
{
	ComPtr<ID3DBlob> vsBlob, psBlob, errBlob;
	HRESULT hr = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "vs_main", "vs_5_1", 0, 0, &vsBlob, &errBlob);
	if (FAILED(hr))
	{
		if (errBlob) PrintLog(LogColor::RED, "[Shader] Vertex Shader Error: " + std::string((char*)errBlob->GetBufferPointer()));
		exit(1);
	}
	PrintLog(LogColor::GREEN, "[Shader] Vertex Shader Compiled");

	hr = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "ps_main", "ps_5_1", 0, 0, &psBlob, &errBlob);
	if (FAILED(hr))
	{
		if (errBlob) PrintLog(LogColor::RED, "[Shader] Pixel Shader Error: " + std::string((char*)errBlob->GetBufferPointer()));
		exit(1);
	}
	PrintLog(LogColor::GREEN, "[Shader] Pixel Shader Compiled");

	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParam.Constants.Num32BitValues = 20;
	rootParam.Constants.ShaderRegister = 0;

	D3D12_ROOT_SIGNATURE_DESC sigDesc = { 1, &rootParam, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };
	ComPtr<ID3DBlob> sigBlob;
	hr = D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
	if (FAILED(hr))
	{
		if (errBlob) PrintLog(LogColor::RED, "[Shader] Root Signature Error: " + std::string((char*)errBlob->GetBufferPointer()));
		exit(1);
	}

	hr = mDevice->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[Shader] Failed to create Root Signature"); exit(1); }
	PrintLog(LogColor::GREEN, "[Shader] Root Signature Created");

	D3D12_INPUT_ELEMENT_DESC layout[] = {
		{ "POS",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COL",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 4;
	psoDesc.SampleDesc.Quality = m_nMsaa4xQualityLevels - 1;

	hr = mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[Shader] Failed to create Pipeline State"); exit(1); }
	PrintLog(LogColor::GREEN, "[Shader] Graphics Pipeline State Created (MSAA 4x, CullBack, DepthTest)");
}

void Framework::CreateImGuiSrvHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
	HRESULT hr = mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mImGuiSrvHeap));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[ImGui] Failed to create SRV Heap"); exit(1); }
	PrintLog(LogColor::YELLOW, "[ImGui] SRV Heap Created");
}