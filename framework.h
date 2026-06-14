#pragma once
#include "stdafx.h"

class Framework
{
public:
    Framework(int width = 1280, int height = 720);
    virtual ~Framework();

    void Initialize(HWND hwnd);
    void Run();
    void Release();
    void OnResize(int width, int height);

	void SetResizing(bool val) { mIsResizing = val; }  
	bool IsResizing() const { return mIsResizing; }    

    static ComPtr<ID3D12Device> GetDevice() { return mDevice; }
    static HWND GetHwnd()   { return mHwnd; }
    static int  GetWidth()  { return mWindowWidth; }
    static int  GetHeight() { return mWindowHeight; }

private:
    void WaitForGPU();

    void InitDirect3D();
    void CreateCommandQueueAndList();
    void CreateSwapChain();
    void CreateRtvDsvDescriptorHeap();
    void CreateFrameBuffers();
    void CreateDepthStencilView();
    void CreateSyncObjects();
    void CompileShaders();

    void Update();
    void Render();
	
private:
    static int mWindowWidth;
    static int mWindowHeight;
    bool isRunning = false;
    static HWND mHwnd;
	BOOL isFullScreen = false;
	bool mIsResizing = false;  

    static ComPtr<ID3D12Device> mDevice;
    static const UINT FRAME_BUFFER_COUNT = 2;

    ComPtr<IDXGIFactory4> mDxgiFactory;
    ComPtr<IDXGISwapChain3> mSwapChain;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    ComPtr<ID3D12DescriptorHeap> mDsvHeap;
    ComPtr<ID3D12Resource> mFrameBuffers[FRAME_BUFFER_COUNT];
    ComPtr<ID3D12Resource> mDepthStencilBuffer;
    ComPtr<ID3D12CommandAllocator> mCommandAllocators[FRAME_BUFFER_COUNT];
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<ID3D12Fence> mFence;
    ComPtr<ID3D12RootSignature> mRootSignature;
    ComPtr<ID3D12PipelineState> mPipelineState;
	ComPtr<ID3D12Debug> mDebugController;
	

	ComPtr<ID3D12Resource> mMsaaRenderTarget;
	ComPtr<ID3D12Resource> mMsaaDepthStencil;
	UINT m_nMsaa4xQualityLevels = 0;

    UINT mRtvDescriptorSize = 0;
    HANDLE mFenceEvent = nullptr;
    UINT64 mFenceValue = 0;

	D3D12_RESOURCE_STATES mMsaaRTState = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_STATES mDepthStencilState = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_STATES mFrameBufferStates[FRAME_BUFFER_COUNT] = {};
};