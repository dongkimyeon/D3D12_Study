#pragma once
#include "stdafx.h"

/**
 * Framework 클래스
 * DirectX 12 어플리케이션의 생명주기(초기화, 실행, 업데이트, 렌더링, 해제)를 관리합니다.
 */
class Framework
{
public:
    Framework(int width = 1280, int height = 720);
    virtual ~Framework();

    void Initialize(HWND hwnd); // 엔진 및 DX12 초기화
    void Run();                // 메인 루프 실행
    void Release();            // 자원 해제
    void OnResize(int width, int height); // 윈도우 크기 변경 대응

	void SetResizing(bool val) { mIsResizing = val; }  
	bool IsResizing() const { return mIsResizing; }    

    // 씬 등 외부에서 사용할 Device 정적 접근자 (DX12의 핵심 객체)
    static ComPtr<ID3D12Device> GetDevice() { return mDevice; }
    static HWND    GetHwnd()   { return sHwnd; }
    static int     GetWidth()  { return sWidth; }
    static int     GetHeight() { return sHeight; }

private:
    // 초기화 단계별 세부 함수
    void InitDirect3D();               // 장치(Device) 및 어댑터 생성
    void CreateCommandQueueAndList();  // 명령 대기열 및 리스트 생성
    void CreateSwapChain();            // 스왑 체인(화면 전환) 생성
    void CreateRtvDsvDescriptorHeap(); // 렌더 타겟/깊이 스텐실 뷰 힙 생성
    void CreateFrameBuffers();         // 실제 렌더 타겟 리소스 생성
    void CreateDepthStencilView();     // 깊이 버퍼 리소스 생성
    void CreateSyncObjects();          // CPU-GPU 동기화 객체(Fence) 생성
    void CompileShaders();             // 셰이더 컴파일 및 파이프라인 상태(PSO) 생성
    void CreateImGuiSrvHeap();         // ImGui용 셰이더 리소스 뷰 힙 생성

    void WaitForGPU();                 // GPU가 명령 처리를 마칠 때까지 대기 (동기화)
    void Update();                     // 매 프레임 로직 업데이트
    void Render();                     // 매 프레임 화면 그리기
	
private:
    int mWindowWidth;
    int mWindowHeight;
    bool isRunning = false;
    HWND mHwnd;
	BOOL isFullScreen = false;
	bool mIsResizing = false;  
    // 창 정보 정적 접근 (씬에서 레이 피킹 등에 사용)
    static HWND sHwnd;
    static int  sWidth;
    static int  sHeight;

    // DX12 핵심 객체들
    static ComPtr<ID3D12Device> mDevice;        // 하드웨어 장치 인터페이스
    static const UINT FRAME_BUFFER_COUNT = 2;  // 더블 버퍼링용 버퍼 개수

    ComPtr<IDXGIFactory4> mDxgiFactory;         // DXGI 팩토리 (어댑터, 스왑체인 관리)
    ComPtr<IDXGISwapChain3> mSwapChain;         // 스왑 체인
    ComPtr<ID3D12CommandQueue> mCommandQueue;   // 명령 대기열 (GPU로 명령 전달)
    ComPtr<ID3D12DescriptorHeap> mRtvHeap;      // RTV(Render Target View) 서술자 힙
    ComPtr<ID3D12DescriptorHeap> mDsvHeap;      // DSV(Depth Stencil View) 서술자 힙
    ComPtr<ID3D12Resource> mFrameBuffers[FRAME_BUFFER_COUNT]; // 백 버퍼 리소스
    ComPtr<ID3D12Resource> mDepthStencilBuffer; // 깊이 버퍼 리소스
    ComPtr<ID3D12CommandAllocator> mCommandAllocators[FRAME_BUFFER_COUNT]; // 명령 메모리 할당자
    ComPtr<ID3D12GraphicsCommandList> mCommandList; // 명령 기록 리스트
    ComPtr<ID3D12Fence> mFence;                 // 동기화용 펜스
    ComPtr<ID3D12RootSignature> mRootSignature; // 셰이더 리소스 바인딩 레이아웃
    ComPtr<ID3D12PipelineState> mPipelineState; // 파이프라인 상태 객체 (PSO)
    ComPtr<ID3D12DescriptorHeap> mImGuiSrvHeap; // ImGui용 힙
	ComPtr<ID3D12Debug> mDebugController;       // 디버그 제어기
	
	// MSAA (멀티 샘플 안티앨리어싱) 관련
	ComPtr<ID3D12Resource> mMsaaRenderTarget; // MSAA용 별도 렌더 타겟
	ComPtr<ID3D12Resource> mMsaaDepthStencil;  // MSAA용 깊이 버퍼
	UINT m_nMsaa4xQualityLevels = 0;           // 지원되는 MSAA 품질 수준

    UINT mRtvDescriptorSize = 0;               // RTV 서술자 한 개의 크기
    HANDLE mFenceEvent = nullptr;              // 펜스 이벤트 핸들
    UINT64 mFenceValue = 0;                    // 펜스 값 (동기화 확인용)

	// 리소스 배리어 상태 추적 변수 (암시적 가정 대신 명시적으로 현재 상태를 기록)
	D3D12_RESOURCE_STATES mMsaaRTState = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_STATES mDepthStencilState = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_STATES mFrameBufferStates[FRAME_BUFFER_COUNT] = {};
};