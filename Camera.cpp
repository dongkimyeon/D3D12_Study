#include "Camera.h"

// 정적 멤버 변수 정의 및 초기화 (XMFLOAT3 사용)
DirectX::XMFLOAT3 Camera::camPos = { 0.0f, 25.0f, -25.0f };
DirectX::XMFLOAT3 Camera::camForward = { 0.0f, 0.0f, 1.0f };
DirectX::XMFLOAT3 Camera::camRight = { 1.0f, 0.0f, 0.0f };
DirectX::XMFLOAT3 Camera::camUp = { 0.0f, 1.0f, 0.0f };

float Camera::moveSpeed = 10.0f;
float Camera::lookSpeed = 0.006f;
float Camera::camYaw = 0.0f;
float Camera::camPitch = 0.5f;
POINT Camera::prevMousePos = { 0, 0 };
bool Camera::isRotating = false;
eCameraMode Camera::sMode = eCameraMode::FirstPerson;

extern bool debugMode;

void Camera::SetFollowTarget(XMFLOAT3 playerPos, float playerPitch ,float playerYaw)
{
	XMMATRIX rotM = XMMatrixRotationY(playerYaw);
	CXMMATRIX pitchM = XMMatrixRotationX(playerPitch);

	XMVECTOR fwd = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), pitchM * rotM);
	XMVECTOR rgt = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), pitchM * rotM);
	XMStoreFloat3(&camForward, fwd);
	XMStoreFloat3(&camRight, rgt);
	camUp = { 0, 1, 0 };

	if (sMode == eCameraMode::FirstPerson) {
		camPos = { playerPos.x, playerPos.y, playerPos.z };
	} else {
		// 3인칭: 플레이어 뒤 6, 위 3
		XMVECTOR target   = XMLoadFloat3(&playerPos);
		XMVECTOR camPosV  = target - fwd * 3.0f + XMVectorSet(0, 1.5f, 0, 0);
		XMStoreFloat3(&camPos, camPosV);
		// 플레이어를 바라보는 방향으로 덮어씀
		XMVECTOR lookDir = XMVector3Normalize(target - camPosV);
		XMStoreFloat3(&camForward, lookDir);
		XMVECTOR newRight = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), lookDir));
		XMStoreFloat3(&camRight, newRight);
	}
}

void Camera::Update(float dt)
{
	ImGuiIO& io = ImGui::GetIO();
	
	if (debugMode) {
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

		// 2. 카메라 방향 벡터 연산
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

		XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		if (Input::GetKey(eKeyCode::Q)) pos -= worldUp * moveSpeed * dt;
		if (Input::GetKey(eKeyCode::E)) pos += worldUp * moveSpeed * dt;

		if (Input::GetKeyDown(eKeyCode::SHIFT)) moveSpeed *= 2.0f;
		if (Input::GetKeyUp(eKeyCode::SHIFT)) moveSpeed /= 2.0f;

		DirectX::XMStoreFloat3(&camPos, pos);
	}
	
}