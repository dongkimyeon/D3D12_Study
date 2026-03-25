#include "Camera.h"

// 정적 멤버 변수 정의 및 초기화 (XMFLOAT3 사용)
DirectX::XMFLOAT3 Camera::camPos = { 0.0f, 25.0f, -25.0f };
DirectX::XMFLOAT3 Camera::camForward = { 0.0f, 0.0f, 1.0f };
DirectX::XMFLOAT3 Camera::camRight = { 1.0f, 0.0f, 0.0f };
DirectX::XMFLOAT3 Camera::camUp = { 0.0f, 1.0f, 0.0f };

float Camera::moveSpeed = 10.0f;
float Camera::lookSpeed = 0.006f;
float Camera::camYaw = 0.0f;
float Camera::camPitch = 0.0f;
POINT Camera::prevMousePos = { 0, 0 };
bool Camera::isRotating = false;

void Camera::Update(float dt)
{
	ImGuiIO& io = ImGui::GetIO();

	// 1. 카메라 마우스 회전 제어
	if (!io.WantCaptureMouse && Input::GetKey(eKeyCode::LButton))
	{
		POINT currMousePos;
		GetCursorPos(&currMousePos);

		if (!isRotating) { prevMousePos = currMousePos; isRotating = true; }

		camYaw += (currMousePos.x - prevMousePos.x) * lookSpeed;
		camPitch += (currMousePos.y - prevMousePos.y) * lookSpeed;
		const float limit = DirectX::XM_PIDIV2 - 0.1f;
		if (camPitch > limit) camPitch = limit;
		if (camPitch < -limit) camPitch = -limit;

		prevMousePos = currMousePos;
	}
	else { isRotating = false; }

	// 2. 카메라 방향 벡터 연산 (SIMD 레지스터 사용)
	DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(camPitch, camYaw, 0.0f);

	DirectX::XMVECTOR forward = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotationMatrix);
	DirectX::XMVECTOR right = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotationMatrix);
	DirectX::XMVECTOR up = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotationMatrix);

	// 연산된 방향 벡터를 XMFLOAT3에 저장 (메모리)
	DirectX::XMStoreFloat3(&camForward, forward);
	DirectX::XMStoreFloat3(&camRight, right);
	DirectX::XMStoreFloat3(&camUp, up);

	// 3. 카메라 이동 (Load -> Math -> Store)
	DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&camPos);

	if (Input::GetKey(eKeyCode::W)) pos += forward * moveSpeed * dt;
	if (Input::GetKey(eKeyCode::S)) pos -= forward * moveSpeed * dt;
	if (Input::GetKey(eKeyCode::D)) pos += right * moveSpeed * dt;
	if (Input::GetKey(eKeyCode::A)) pos -= right * moveSpeed * dt;
	if (Input::GetKey(eKeyCode::E)) pos -= up * moveSpeed * dt;
	if (Input::GetKey(eKeyCode::Q)) pos += up * moveSpeed * dt;

	if (Input::GetKeyDown(eKeyCode::SHIFT)) moveSpeed *= 2.0f;
	if (Input::GetKeyUp(eKeyCode::SHIFT)) moveSpeed /= 2.0f;

	DirectX::XMStoreFloat3(&camPos, pos);
}