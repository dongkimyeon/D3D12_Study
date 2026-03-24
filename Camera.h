#pragma once

#include "stdafx.h"

class Camera
{
public:
  
	static void Update(float dt);  
    static void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj); // 카메라는 렌더링하지 않음
    // 카메라 관련
    static XMFLOAT3 camPos;
    static float camYaw;
    static float camPitch;
    static float moveSpeed;
    static float lookSpeed;
    static XMMATRIX camRot;
    static bool isRotating;
    static XMVECTOR camForward;
    static POINT prevMousePos;
};

