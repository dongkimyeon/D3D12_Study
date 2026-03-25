#include "Camera.h"

// 정적 멤버 변수 정의 및 초기화
DirectX::XMFLOAT3 Camera::camPos = { 0.0f, 0.0f, -10.0f };
DirectX::XMMATRIX Camera::camRot = DirectX::XMMatrixIdentity();
float Camera::moveSpeed = 10.0f;
float Camera::lookSpeed = 0.006;
float Camera::camYaw = 0.0f;
float Camera::camPitch = 0.0f;
POINT Camera::prevMousePos = { 0, 0 };
bool Camera::isRotating = false;
XMVECTOR Camera::camForward;



void Camera::Update(float dt)
{
    ImGuiIO& io = ImGui::GetIO();

    // 뷰, 프로젝션 행렬 계산
    XMVECTOR currentPos = XMLoadFloat3(&camPos);
    XMMATRIX camRot = XMMatrixRotationRollPitchYaw(camPitch, camYaw, 0);

    // 1. 카메라 마우스 회전 제어
    if (!io.WantCaptureMouse && Input::GetKey(eKeyCode::LButton))
    {
        POINT currMousePos;
        GetCursorPos(&currMousePos);

        if (!isRotating) { prevMousePos = currMousePos; isRotating = true; }

        camYaw += (currMousePos.x - prevMousePos.x) * lookSpeed;
        camPitch += (currMousePos.y - prevMousePos.y) * lookSpeed;
        const float limit = XM_PIDIV2 - 0.1f;
        if (camPitch > limit) camPitch = limit;
        if (camPitch < -limit) camPitch = -limit;

        prevMousePos = currMousePos;
    }
    else { isRotating = false; }

    // 2. 카메라 이동
    currentPos = XMLoadFloat3(&camPos);
    camRot = XMMatrixRotationRollPitchYaw(camPitch, camYaw, 0);
    camForward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), camRot);
    XMVECTOR right = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), camRot);
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    XMVECTOR pos = XMLoadFloat3(&camPos);

    if (Input::GetKey(eKeyCode::W)) pos += camForward * moveSpeed * dt;
    if (Input::GetKey(eKeyCode::S)) pos -= camForward * moveSpeed * dt;
    if (Input::GetKey(eKeyCode::D)) pos += right * moveSpeed * dt;
    if (Input::GetKey(eKeyCode::A)) pos -= right * moveSpeed * dt;
    if (Input::GetKey(eKeyCode::E)) pos -= up * moveSpeed * dt;
    if (Input::GetKey(eKeyCode::Q)) pos += up * moveSpeed * dt;

    if (Input::GetKeyDown(eKeyCode::SHIFT)) moveSpeed *= 2.0f;
    if (Input::GetKeyUp(eKeyCode::SHIFT)) moveSpeed /= 2.0f;

    XMStoreFloat3(&camPos, pos);
}

