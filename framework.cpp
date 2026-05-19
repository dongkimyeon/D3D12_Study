#include "stdafx.h"
#include "Framework.h"
#include "SceneManager.h"
#include "Map1Scene.h"
#include "Map2Scene.h"
#include "LoadScene.h"

#define DEBUG

ComPtr<ID3D12Device> Framework::mDevice = nullptr;
HWND Framework::sHwnd   = nullptr;
int  Framework::sWidth  = 0;
int  Framework::sHeight = 0;

Framework::Framework(int width, int height)
	: mWindowWidth(width), mWindowHeight(height)
{
}

Framework::~Framework() {
	Release();
}

void Framework::Initialize(HWND hwnd)
{
	mHwnd  = hwnd;
	sHwnd  = hwnd;
	sWidth  = mWindowWidth;
	sHeight = mWindowHeight;

	// [1] 디버그 레이어 활성화: 개발 중 발생하는 DX12 관련 오류를 콘솔에 출력해줍니다.
#if defined(DEBUG) || defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		PrintLog(LogColor::CYAN, "[D3D12_DEBUG] D3D12 Debug Layer Enabled");
	}
	else {
		PrintLog(LogColor::RED, "[D3D12_DEBUG] Failed to enable D3D12 Debug Layer");
	}
#endif

	// [2] DirectX 12 핵심 객체 초기화 순서
	InitDirect3D();                 // Device 생성
	CreateCommandQueueAndList();    // 명령 전달 체계 생성
	CreateSwapChain();              // 화면 출력을 위한 스왑 체인 생성
	CreateRtvDsvDescriptorHeap();   // 렌더 타겟/깊이 스텐실 포인터 보관함(Heap) 생성
	CreateFrameBuffers();           // 실제 화면 버퍼 생성
	CreateDepthStencilView();       // 깊이 버퍼 생성
	CreateSyncObjects();            // CPU-GPU 동기화 객체(Fence) 생성
	CompileShaders();               // 셰이더 및 파이프라인(PSO) 설정
	CreateImGuiSrvHeap();           // UI(ImGui)용 리소스 힙 생성

	// [3] 외부 라이브러리 및 매니저 초기화 (ImGui, Time, Input, Scene)
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
	// 메인 메시지 루프: 윈도우 메시지를 처리하거나 게임 로직(Update/Render)을 수행합니다.
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
	// 매 프레임 변하는 상태 업데이트 (시간, 입력, 카메라, 씬 로직)
	Time::Update();
	Input::Update();
	Camera::Update(Time::GetDeltaTime());

	// [추가] FPS 표시 로직
	static float fpsTimer = 0.0f;
	static int frameCount = 0;
	fpsTimer += Time::GetDeltaTime();
	frameCount++;

	if (fpsTimer >= 1.0f) 
	{
		float fps = (float)frameCount / fpsTimer;
		std::wstring title = L"DirectX_12 | FPS : " + std::to_wstring((int)fps);
		SetWindowTextW(mHwnd, title.c_str());

		fpsTimer = 0.0f;
		frameCount = 0;
	}

	// ImGui 프레임 시작
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	SceneManager::Update();

	// F9 입력 시 전체화면 전환 토글
	if (Input::GetKeyDown(eKeyCode::F9))
	{
		BOOL isFullScreen = false;
		mSwapChain->GetFullscreenState(&isFullScreen, nullptr);
		mSwapChain->SetFullscreenState(!isFullScreen, nullptr);

		// 전체화면 전환 후 실제 윈도우 클라이언트 영역 크기를 가져와서 OnResize 호출
		RECT clientRect = {};
		GetClientRect(mHwnd, &clientRect);
		int newWidth = clientRect.right - clientRect.left;
		int newHeight = clientRect.bottom - clientRect.top;

		// 크기가 0이면 무시 (전환 직후 잠시 0이 될 수 있음)
		if (newWidth > 0 && newHeight > 0)
		{
			OnResize(newWidth, newHeight);
		}
	}
}
	

void Framework::Render()
{
	// [1] 명령 기록 준비
	// DX12는 명령 리스트에 명령을 기록한 뒤 한꺼번에 GPU로 보냅니다.
	UINT backBufferIdx = mSwapChain->GetCurrentBackBufferIndex();
	mCommandAllocators[backBufferIdx]->Reset(); // 할당자 초기화
	mCommandList->Reset(mCommandAllocators[backBufferIdx].Get(), mPipelineState.Get()); // 리스트 초기화

	// [2] 렌더 타겟 및 깊이 버퍼 핸들 계산
	// MSAA(멀티 샘플링)용 버퍼를 대상으로 먼저 그립니다.
	D3D12_CPU_DESCRIPTOR_HANDLE msaaRtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	msaaRtvHandle.ptr += FRAME_BUFFER_COUNT * mRtvDescriptorSize;

	D3D12_CPU_DESCRIPTOR_HANDLE msaaDsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	msaaDsvHandle.ptr += mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	backBufferHandle.ptr += backBufferIdx * mRtvDescriptorSize;

	// [3] 렌더 패스 시작: 명시적 배리어로 MSAA RT와 깊이 버퍼를 렌더링 가능 상태로 전환
	// - 첫 프레임: COMMON → RENDER_TARGET / DEPTH_WRITE
	// - 이후 프레임: RESOLVE_SOURCE → RENDER_TARGET (MSAA RT), DEPTH_WRITE 유지 (깊이 버퍼)
	{
		D3D12_RESOURCE_BARRIER renderPassBarriers[2] = {};
		int barrierCount = 0;

		if (mMsaaRTState != D3D12_RESOURCE_STATE_RENDER_TARGET)
		{
			renderPassBarriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			renderPassBarriers[barrierCount].Transition.pResource = mMsaaRenderTarget.Get();
			renderPassBarriers[barrierCount].Transition.StateBefore = mMsaaRTState;
			renderPassBarriers[barrierCount].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			renderPassBarriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			++barrierCount;
			mMsaaRTState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		}

		if (mDepthStencilState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
		{
			renderPassBarriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			renderPassBarriers[barrierCount].Transition.pResource = mMsaaDepthStencil.Get();
			renderPassBarriers[barrierCount].Transition.StateBefore = mDepthStencilState;
			renderPassBarriers[barrierCount].Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			renderPassBarriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			++barrierCount;
			mDepthStencilState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		}

		if (barrierCount > 0)
			mCommandList->ResourceBarrier(barrierCount, renderPassBarriers);
	}

	// [4] 화면 클리어 및 렌더 타겟 설정
	float clearColor[] = { 0.05f, 0.05f, 0.1f, 1.0f };
	mCommandList->ClearRenderTargetView(msaaRtvHandle, clearColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(msaaDsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	mCommandList->OMSetRenderTargets(1, &msaaRtvHandle, FALSE, &msaaDsvHandle);

	// [5] 가위 구역(Scissor) 및 뷰포트 설정, 드로우 콜 기록
	D3D12_VIEWPORT vp = { 0.0f, 0.0f, (float)mWindowWidth, (float)mWindowHeight, 0.0f, 1.0f };
	D3D12_RECT scissor = { 0, 0, mWindowWidth, mWindowHeight };
	mCommandList->RSSetViewports(1, &vp);
	mCommandList->RSSetScissorRects(1, &scissor);
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 씬의 오브젝트들을 그립니다.
	SceneManager::Render(mCommandList);

	// [6] MSAA Resolve: MSAA 버퍼 → 백 버퍼
	// 상태 추적 변수(mMsaaRTState, mFrameBufferStates)를 사용해 StateBefore를 명시적으로 지정합니다.
	D3D12_RESOURCE_BARRIER resolveBarriers[2] = {};
	resolveBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resolveBarriers[0].Transition.pResource = mMsaaRenderTarget.Get();
	resolveBarriers[0].Transition.StateBefore = mMsaaRTState;                        // RENDER_TARGET
	resolveBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	resolveBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	resolveBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resolveBarriers[1].Transition.pResource = mFrameBuffers[backBufferIdx].Get();
	resolveBarriers[1].Transition.StateBefore = mFrameBufferStates[backBufferIdx];   // PRESENT
	resolveBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_DEST;
	resolveBarriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	mCommandList->ResourceBarrier(2, resolveBarriers);
	mMsaaRTState = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	mFrameBufferStates[backBufferIdx] = D3D12_RESOURCE_STATE_RESOLVE_DEST;

	// 멀티샘플링된 데이터를 일반 데이터로 합칩니다.
	mCommandList->ResolveSubresource(
		mFrameBuffers[backBufferIdx].Get(), 0,
		mMsaaRenderTarget.Get(), 0,
		DXGI_FORMAT_R8G8B8A8_UNORM
	);

	// Resolve 후 백 버퍼만 PRESENT로 전환합니다.
	// MSAA RT는 RESOLVE_SOURCE 상태로 유지하고, 다음 프레임 [3] 배리어에서 RENDER_TARGET으로 전환합니다.
	D3D12_RESOURCE_BARRIER fbToPresentBarrier = {};
	fbToPresentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	fbToPresentBarrier.Transition.pResource = mFrameBuffers[backBufferIdx].Get();
	fbToPresentBarrier.Transition.StateBefore = mFrameBufferStates[backBufferIdx];   // RESOLVE_DEST
	fbToPresentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	fbToPresentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	mCommandList->ResourceBarrier(1, &fbToPresentBarrier);
	mFrameBufferStates[backBufferIdx] = D3D12_RESOURCE_STATE_PRESENT;

	// [7] ImGui(UI) 렌더링: 백 버퍼를 RENDER_TARGET으로 전환
	D3D12_RESOURCE_BARRIER imguiBarrier = {};
	imguiBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	imguiBarrier.Transition.pResource = mFrameBuffers[backBufferIdx].Get();
	imguiBarrier.Transition.StateBefore = mFrameBufferStates[backBufferIdx];         // PRESENT
	imguiBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	imguiBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	mCommandList->ResourceBarrier(1, &imguiBarrier);
	mFrameBufferStates[backBufferIdx] = D3D12_RESOURCE_STATE_RENDER_TARGET;

	mCommandList->OMSetRenderTargets(1, &backBufferHandle, FALSE, nullptr);
	ImGui::Render();
	ID3D12DescriptorHeap* descriptorHeaps[] = { mImGuiSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(1, descriptorHeaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

	// [8] 최종 화면 출력 준비 (Present 상태로 전이)
	D3D12_RESOURCE_BARRIER presentBarrier = {};
	presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	presentBarrier.Transition.pResource = mFrameBuffers[backBufferIdx].Get();
	presentBarrier.Transition.StateBefore = mFrameBufferStates[backBufferIdx];       // RENDER_TARGET
	presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	presentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	mCommandList->ResourceBarrier(1, &presentBarrier);
	mFrameBufferStates[backBufferIdx] = D3D12_RESOURCE_STATE_PRESENT;

	// [9] 명령 실행 및 프레임 교체
	mCommandList->Close();
	ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(1, ppCommandLists); // GPU에 명령 전달
	mSwapChain->Present(0, 0); // 화면 출력
	WaitForGPU(); // CPU가 GPU의 작업 완료를 기다림 (프레임 동기화)
}

void Framework::WaitForGPU()
{
	// CPU가 GPU보다 너무 앞서나가지 않도록 Fence를 이용해 기다립니다.
	UINT64 waitValue = ++mFenceValue;
	mCommandQueue->Signal(mFence.Get(), waitValue); // GPU가 이 지점에 도달하면 Fence 값을 갱신하도록 명령
	if (mFence->GetCompletedValue() < waitValue)
	{
		mFence->SetEventOnCompletion(waitValue, mFenceEvent);
		WaitForSingleObject(mFenceEvent, INFINITE); // GPU 작업 완료까지 대기
	}
}

void Framework::Release()
{
	// 종료 시 GPU가 작업 중인지 확인하고 해제합니다.
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


	// 종료 시 해제되지 않은 메모리(Live Objects)가 있는지 보고합니다.
	IDXGIDebug1* pdxgiDebug = NULL;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void**)&pdxgiDebug)))
	{
		PrintLog(LogColor::GRAY, "[D3D12_DEBUG] Reporting Live DXGI Objects...");
		pdxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
		pdxgiDebug->Release();
		PrintLog(LogColor::GRAY, "[D3D12_DEBUG] Live Object Report Complete");
	}
	else
	{
		PrintLog(LogColor::RED, "[D3D12_DEBUG] Failed to get DXGI Debug Interface");
	}

}

void Framework::OnResize(int width, int height)
{
	if (mSwapChain == nullptr) return;

	mWindowWidth  = width;
	mWindowHeight = height;
	sWidth  = width;
	sHeight = height;

	// [1] GPU 작업 완료 대기
	WaitForGPU();

	// [2] CommandList를 닫힌 상태로 만들고 초기화
	//     Close() 후 Reset() 해야 안전함
	mCommandList->Close();
	mCommandList->Reset(mCommandAllocators[0].Get(), nullptr);

	// [3] 기존 프레임 버퍼 해제 (ComPtr이므로 Reset()으로 해제)
	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		mFrameBuffers[i].Reset();
	}

	// [4] MSAA 렌더타겟 및 깊이 스텐실 버퍼 해제
	mMsaaRenderTarget.Reset();
	mMsaaDepthStencil.Reset();

	// [5] 스왑 체인 버퍼 리사이즈
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	mSwapChain->GetDesc(&swapChainDesc);

	HRESULT hr = mSwapChain->ResizeBuffers(
		FRAME_BUFFER_COUNT,
		mWindowWidth,
		mWindowHeight,
		swapChainDesc.BufferDesc.Format,
		swapChainDesc.Flags
	);

	if (FAILED(hr))
	{
		PrintLog(LogColor::RED, "[Error] Failed to resize swap chain buffers");
		exit(0);
	}

	// [6] 새로운 크기로 프레임 버퍼 및 깊이 스텐실 재생성
	CreateFrameBuffers();
	CreateDepthStencilView();

	// [7] CommandList 닫기 (Render에서 Reset해서 재사용함)
	mCommandList->Close();

	// [8] ImGui에 새 DisplaySize 알려주기
	//     ImGui는 내부적으로 DisplaySize로 투영 행렬을 계산하므로
	//     명시적으로 갱신해줘야 비율이 맞게 렌더링됨
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)mWindowWidth, (float)mWindowHeight);

	PrintLog(LogColor::CYAN, "[OnResize] Resize Complete: "
		+ std::to_string(mWindowWidth) + "x" + std::to_string(mWindowHeight));
}

void Framework::InitDirect3D()
{
	// Device 생성: 그래픽 카드와 통신하기 위한 가상 어댑터입니다.
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
	// 시스템의 그래픽 카드 중 하드웨어 가속이 가능한 것을 찾습니다.
	for (UINT i = 0; mDxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) break;
		adapter->Release();
	}

	// 실제 하드웨어 장치(Device) 생성 (Feature Level 11.0 기반)
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
	// Command Queue: 명령을 GPU로 실어 나르는 도로
	D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 1 };
	HRESULT hr = mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create Command Queue"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] Command Queue Created");

	// Command Allocator: 명령 리스트가 사용할 메모리 공간
	for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
	{
		hr = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocators[i]));
		if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create Command Allocator " + std::to_string(i)); exit(1); }
	}
	PrintLog(LogColor::CYAN, "[D3D12] Command Allocators Created (" + std::to_string(FRAME_BUFFER_COUNT) + ")");

	// Command List: 실제 명령(그리기 등)을 기록하는 장부
	hr = mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&mCommandList));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create Command List"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] Command List Created");

	mCommandList->Close(); // 시작 시에는 닫아둡니다.
}

void Framework::CreateSwapChain()
{
	// Swap Chain: Back Buffer에 그린 그림을 전면(Monitor)으로 교체해주는 장치
	// 4x MSAA 지원 여부 및 품질 수준 체크
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
	swapChainDesc.SampleDesc.Count = 1; // 스왑체인은 MSAA를 직접 쓰지 않고 Resolve 기법을 씁니다.
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 최신 고성능 방식

	ComPtr<IDXGISwapChain1> swapChain1;
	hr = mDxgiFactory->CreateSwapChainForHwnd(mCommandQueue.Get(), mHwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create Swap Chain"); exit(1); }

	hr = swapChain1.As(&mSwapChain);
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to cast Swap Chain"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] Swap Chain Created (" + std::to_string(mWindowWidth) + "x" + std::to_string(mWindowHeight) + ")");
}

void Framework::CreateRtvDsvDescriptorHeap()
{
	// Descriptor Heap: 리소스(Buffer, Texture)를 가리키는 포인터들의 배열(Heap)
	// RTV(Render Target View) 힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAME_BUFFER_COUNT + 1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
	HRESULT hr = mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create RTV Descriptor Heap"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] RTV Descriptor Heap Created (" + std::to_string(FRAME_BUFFER_COUNT + 1) + " descriptors)");

	mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// DSV(Depth Stencil View) 힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
	hr = mDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create DSV Descriptor Heap"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] DSV Descriptor Heap Created (2 descriptors)");
}

void Framework::CreateFrameBuffers()
{
	// 스왑 체인으로부터 버퍼를 가져와 RTV를 생성합니다.
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		HRESULT hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mFrameBuffers[i]));
		if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to get Swap Chain Buffer " + std::to_string(i)); exit(1); }
		mDevice->CreateRenderTargetView(mFrameBuffers[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += mRtvDescriptorSize;
	}
	PrintLog(LogColor::CYAN, "[D3D12] Frame Buffers Created (" + std::to_string(FRAME_BUFFER_COUNT) + ")");

	// MSAA용 렌더 타겟 생성 (별도의 멀티샘플링 버퍼)
	D3D12_RESOURCE_DESC msaaDesc = mFrameBuffers[0]->GetDesc();
	msaaDesc.SampleDesc.Count = 4;
	msaaDesc.SampleDesc.Quality = m_nMsaa4xQualityLevels - 1;
	D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT };

	HRESULT hr = mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &msaaDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&mMsaaRenderTarget));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create MSAA Render Target"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] MSAA Render Target Created (4x)");

	D3D12_CPU_DESCRIPTOR_HANDLE msaaRtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	msaaRtvHandle.ptr += FRAME_BUFFER_COUNT * mRtvDescriptorSize;
	mDevice->CreateRenderTargetView(mMsaaRenderTarget.Get(), nullptr, msaaRtvHandle);
	PrintLog(LogColor::CYAN, "[D3D12] MSAA RTV Created");

	// 생성 직후의 초기 상태를 추적 변수에 기록
	mMsaaRTState = D3D12_RESOURCE_STATE_COMMON;
	for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++)
		mFrameBufferStates[i] = D3D12_RESOURCE_STATE_PRESENT;
}

void Framework::CreateDepthStencilView()
{
	// 깊이 버퍼: 물체의 앞뒤 관계를 판단하기 위한 버퍼
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
		D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&mMsaaDepthStencil));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create MSAA Depth Stencil"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] MSAA Depth Stencil Buffer Created (D32_FLOAT, 4x)");

	D3D12_CPU_DESCRIPTOR_HANDLE msaaDsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	msaaDsvHandle.ptr += mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mDevice->CreateDepthStencilView(mMsaaDepthStencil.Get(), nullptr, msaaDsvHandle);
	PrintLog(LogColor::CYAN, "[D3D12] MSAA DSV Created");

	// 생성 직후의 초기 상태를 추적 변수에 기록
	mDepthStencilState = D3D12_RESOURCE_STATE_COMMON;
}

void Framework::CreateSyncObjects()
{
	// Fence: GPU 작업이 어디까지 진행되었는지 CPU가 확인하기 위한 신호등
	HRESULT hr = mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[D3D12] Failed to create Fence"); exit(1); }
	mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!mFenceEvent) { PrintLog(LogColor::RED, "[D3D12] Failed to create Fence Event"); exit(1); }
	PrintLog(LogColor::CYAN, "[D3D12] Sync Objects Created (Fence + Event)");
}

void Framework::CompileShaders()
{
	// [1] HLSL 파일 컴파일 (Vertex Shader, Pixel Shader)
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

	// [2] Root Signature 생성: 셰이더가 어떤 데이터(상수, 텍스처 등)를 사용하는지 정의하는 인터페이스
	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParam.Constants.Num32BitValues = 20; // 20개의 32비트 상수 (행렬 등)
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

	// [3] 입력 배치(Input Layout) 정의: 정점 데이터 구조 설명
	D3D12_INPUT_ELEMENT_DESC layout[] = {
		{ "POS",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COL",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// [4] Pipeline State Object (PSO) 생성: 모든 렌더링 상태를 묶은 객체
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { layout, 3 };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
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
	// ImGui는 텍스처를 그리기 위해 SRV(Shader Resource View)가 필요합니다.
	D3D12_DESCRIPTOR_HEAP_DESC desc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
	HRESULT hr = mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mImGuiSrvHeap));
	if (FAILED(hr)) { PrintLog(LogColor::RED, "[ImGui] Failed to create SRV Heap"); exit(1); }
	PrintLog(LogColor::YELLOW, "[ImGui] SRV Heap Created");
}