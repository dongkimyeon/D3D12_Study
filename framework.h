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

    // 씬 등 외부에서 사용할 Device 정적 접근자
    static ComPtr<ID3D12Device> GetDevice() { return mDevice; }

private:
    void InitDirect3D();
    void CreateCommandQueueAndList();
    void CreateSwapChain();
    void CreateRtvDsvDescriptorHeap();
    void CreateFrameBuffers();
    void CreateDepthStencilView();
    void CreateSyncObjects();
    void CompileShaders();
    void CreateImGuiSrvHeap();

    void WaitForGPU();
    void Update();
    void Render();

private:
    int mWindowWidth;
    int mWindowHeight;
    bool isRunning = false;
    HWND mHwnd;

    static ComPtr<ID3D12Device> mDevice; // Static으로 변경하여 편의성 향상
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
    ComPtr<ID3D12DescriptorHeap> mImGuiSrvHeap;

    UINT mRtvDescriptorSize = 0;
    HANDLE mFenceEvent = nullptr;
    UINT64 mFenceValue = 0;
};