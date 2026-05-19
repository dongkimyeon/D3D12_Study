#include "stdafx.h"
#include "Map2Scene.h"
#include "Cube.h"
#include "Plane.h"
#include "Gizumo.h"
#include "framework.h"
#include "Camera.h"
#include "Utiles.h"
#include "HeliBlade.h"
#include "HeliBody.h"	
#include "HeliTale.h"

extern bool debugMode;

Map2Scene::Map2Scene	()
{

}

Map2Scene::~Map2Scene()
{
}

void Map2Scene::Initialize()
{
	// ============================================
	GameObject* gizumo = new Gizumo();
	gizumo->Initialize(Framework::GetDevice());
	mGameObjects.push_back(gizumo);


	for (int i = 0; i < 2; ++i)
	{
		GameObject* floorPlane = new Plane();
		floorPlane->Initialize(Framework::GetDevice());
		floorPlane->SetPosition(0, 15.0f * i, 0); // 각 평면을 y축으로 5 단위씩 떨어뜨려 배치
		if (i == 1) {
			floorPlane->SetRotation(-XM_PI, 0, 0); // 두 번째 평면은 x축으로 90도 회전
		}
		mGameObjects.push_back(floorPlane);

	}
	





	// 1. Body 생성
	auto body = std::make_unique<HeliBody>(); // 스마트 포인터로 안전하게 생성
	body->Initialize(Framework::GetDevice());
	body->BakeRotationX(-90.f);


	mGameObjects.push_back(body.get());        // 벡터에는 주소값(get)만 복사
	mHeliBody = std::move(body);              // 소유권을 멤버 변수로 이동

	// 2. Tale 생성
	auto tale = std::make_unique<HeliTale>();
	tale->Initialize(Framework::GetDevice());
	tale->BakeRotationX(-90.f);

	mGameObjects.push_back(tale.get());
	mHeliTale = std::move(tale);

	// 3. Blade 생성
	auto blade = std::make_unique<HeliBlade>();
	blade->Initialize(Framework::GetDevice());
	blade->BakeRotationX(-90.f);

	mGameObjects.push_back(blade.get());
	mHeliBlade = std::move(blade);






}

void Map2Scene::Update(float dt)
{
	
	// 1. 자체 회전각 누적 (static 또는 멤버 변수)
	static float bladeAngle = 0.0f;
	static float taleAngle = 0.0f;

	bladeAngle += 1000.0f * dt; // 블레이드 회전 속도
	taleAngle += 800.0f * dt;   // 테일 회전 속도

	// 2. 부모의 위치, 회전 정보 가져오기
	XMFLOAT3 bodyPos = mHeliBody->GetPosition();
	XMFLOAT3 bodyRot = mHeliBody->GetRotation();

	XMMATRIX mBodyRot = XMMatrixRotationRollPitchYaw(bodyRot.x, bodyRot.y, bodyRot.z);

	// ---------------------------------------------
	// 블레이드 처리
	// ---------------------------------------------
	// 블레이드의 회전: 부모의 회전 + 자기 자신의 Y축 회전
	mHeliBlade->SetRotation(bodyRot.x, bodyRot.y + XMConvertToRadians(bladeAngle), bodyRot.z);

	// 블레이드의 위치: 부모 위치 + 부모의 회전이 적용된 로컬 오프셋
	XMVECTOR bladeLocalOffset = XMVectorSet(2.6f, 0.8f, 0.0f, 0.0f);
	XMVECTOR bladeWorldOffset = XMVector3TransformNormal(bladeLocalOffset, mBodyRot);
	XMVECTOR bladePosVec = XMLoadFloat3(&bodyPos) + bladeWorldOffset;

	XMFLOAT3 bladePos;
	XMStoreFloat3(&bladePos, bladePosVec);
	mHeliBlade->SetPosition(bladePos);

	// ---------------------------------------------
	// 테일 처리
	// ---------------------------------------------
	// 테일의 회전: 부모의 회전 + 자기 자신의 X(또는 Z)축 회전
	// (모델 방향에 따라 Z 또는 X 둘 중 하나 선택)
	mHeliTale->SetRotation(bodyRot.x, bodyRot.y, bodyRot.z + XMConvertToRadians(taleAngle));

	XMVECTOR taleLocalOffset = XMVectorSet(-6.4f, 0.7f, -0.3f, 0.0f);
	XMVECTOR taleWorldOffset = XMVector3TransformNormal(taleLocalOffset, mBodyRot);
	XMVECTOR talePosVec = XMLoadFloat3(&bodyPos) + taleWorldOffset;

	XMFLOAT3 talePos;
	XMStoreFloat3(&talePos, talePosVec);
	mHeliTale->SetPosition(talePos);

	// 3. 모든 객체의 기본적인 업데이트 (이때 내부적으로 변경된 Pos, Rot로 worldMatrix가 올바르게 생성됨)
	for (const auto& obj : mGameObjects) {
		obj->Update(dt);
	}
	/*if (!debugMode)
	{
		for (const auto& obj : mGameObjects) {
			Cube* cube = dynamic_cast<Cube*>(obj);
			if (cube) {
	
				if (Input::GetKey(eKeyCode::LButton)){
					POINT currMousePos;
					GetCursorPos(&currMousePos);

					if (!Camera::isRotating) { Camera::prevMousePos = currMousePos; Camera::isRotating = true; }

					Camera::camYaw += (currMousePos.x - Camera::prevMousePos.x) * Camera::lookSpeed;
					Camera::camPitch += (currMousePos.y - Camera::prevMousePos.y) * Camera::lookSpeed;
					const float limit = DirectX::XM_PIDIV2 - 0.1f;
					if (Camera::camPitch > limit) Camera::camPitch = limit;
					if (Camera::camPitch < -limit) Camera::camPitch = -limit;

					Camera::prevMousePos = currMousePos;
				}
				else { Camera::isRotating = false; }

				// 2. 카메라 방향 벡터 연산
				DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(Camera::camPitch, Camera::camYaw, 0.0f);

				DirectX::XMVECTOR forward = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotationMatrix);
				DirectX::XMVECTOR right = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotationMatrix);
				DirectX::XMVECTOR up = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotationMatrix);

				// 연산된 방향 벡터를 XMFLOAT3에 저장 (메모리)
				DirectX::XMStoreFloat3(&Camera::camForward, forward);
				DirectX::XMStoreFloat4(&cube->forward_vector, forward); 
				DirectX::XMStoreFloat3(&Camera::camRight, right);
				DirectX::XMStoreFloat3(&Camera::camUp, up);

				//obj가 바라보는 방향과 카메라가 보는 방향을 일치 시킨다.
				cube->rotation.y = Camera::camYaw;
				cube->rotation.x = Camera::camPitch; 


				// 3. 카메라 이동 (Load -> Math -> Store)
				DirectX::XMVECTOR objPos = DirectX::XMLoadFloat3(&cube->position);
				DirectX::XMVECTOR camPos = DirectX::XMLoadFloat3(&Camera::camPos);
				camPos = objPos - forward * 10.0f + up * 2.0f; // 카메라를 큐브 뒤쪽과 위쪽에 배치
				
				float moveSpeed = 100.0f;

				if (Input::GetKey(eKeyCode::W)) objPos += forward * moveSpeed * dt;
				if (Input::GetKey(eKeyCode::S)) objPos -= forward * moveSpeed * dt;
				if (Input::GetKey(eKeyCode::D)) objPos += right * moveSpeed * dt;
				if (Input::GetKey(eKeyCode::A)) objPos -= right * moveSpeed * dt;

				XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

				if (Input::GetKey(eKeyCode::Q)) objPos -= worldUp * moveSpeed * dt;
				if (Input::GetKey(eKeyCode::E)) objPos += worldUp * moveSpeed * dt;

				if (Input::GetKeyDown(eKeyCode::SHIFT)) moveSpeed *= 2.0f;
				if (Input::GetKeyUp(eKeyCode::SHIFT)) moveSpeed /= 2.0f;

				DirectX::XMStoreFloat3(&Camera::camPos, camPos);
				DirectX::XMStoreFloat3(&cube->position, objPos);
			}
		}
	}*/



	
	for (auto& objA : mGameObjects) {
		Cube* cube = dynamic_cast<Cube*>(objA);
		if (!cube) continue; // 큐브가 아니면 패스

		for (auto& objB : mGameObjects) {
			Plane* plane = dynamic_cast<Plane*>(objB);
			if (!plane) continue; // 평면이 아니면 패스

			// --- 평면의 정보 추출 ---
			// 평면의 Normal은 기본적으로 (0, 1, 0)이며, 평면의 Rotation에 의해 변함
			XMFLOAT3 planeRotVec = plane->GetRotation();
			XMMATRIX planeRot = XMMatrixRotationRollPitchYaw(planeRotVec.x, planeRotVec.y, planeRotVec.z);
			XMVECTOR planeNormal = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), planeRot);
			XMFLOAT3 planePosVec = plane->GetPosition();
			XMVECTOR planePos = XMLoadFloat3(&planePosVec);
			XMFLOAT3 cubePosVec = cube->GetPosition();
			XMVECTOR cubePos = XMLoadFloat3(&cubePosVec);

			// --- 점과 평면 사이의 거리 계산 (D = n · (Q - P0)) ---
			XMVECTOR vecToCube = XMVectorSubtract(cubePos, planePos);
			XMVECTOR distVec = XMVector3Dot(planeNormal, vecToCube);

			float distance = XMVectorGetX(distVec);

			// 큐브의 절반 크기 (바닥면까지의 거리)
			float halfHeight = cube->GetScale().y * 0.5f;

			// 충돌 조건: 거리가 큐브의 절반보다 작을 때
			if (distance < halfHeight)
			{
				// 충돌 반응: 평면의 법선 방향으로 큐브를 밀어냄 (Penetration Resolution)
				float penetrationDepth = halfHeight - distance;
				XMVECTOR correction = XMVectorScale(planeNormal, penetrationDepth);

				XMVECTOR correctedPos = XMVectorAdd(cubePos, correction);
				XMFLOAT3 newCubePos;
				XMStoreFloat3(&newCubePos, correctedPos);
				cube->SetPosition(newCubePos);

			}
		}
	}



}

void Map2Scene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{

    XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&Camera::camPos), XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0), XMVectorSet(0, 1, 0, 0));
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 100.0f);


  
    for (const auto& obj : mGameObjects) {
        obj->Render(commandList, view, proj);
    }

    // 설정 UI 
    ImGui::Begin("Settings");
	ImGui::Separator();
    ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", Camera::camPos.x, Camera::camPos.y, Camera::camPos.z);

	ImGui::Separator();
	for (const auto& obj : mGameObjects) {
		Cube* cube = dynamic_cast<Cube*>(obj);
		if (cube) {
			
			// 1. 방향 구하기
			XMFLOAT3 toObj;
			XMVECTOR toObjVec;
			XMVECTOR camPosVec = XMLoadFloat3(&Camera::camPos);
			XMFLOAT3 cubePosVec2 = cube->GetPosition();
			XMVECTOR objPosVec = XMLoadFloat3(&cubePosVec2);

			toObjVec = XMVectorSubtract(objPosVec, camPosVec);

			// 2. 정규화 (Scalar 방식은 루트를 직접 계산해야 함)
			float length = XMVectorGetX(XMVector3Length(toObjVec));
			toObjVec = XMVectorScale(toObjVec, 1.0f / length);

			// 3. 내적 (Dot Product) 직접 계산
			float fDot = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&Camera::camForward), toObjVec));

			// 4. 외적 (Cross Product) 계산
			XMVECTOR cross = XMVector3Cross(XMLoadFloat3(&Camera::camForward), toObjVec);

			// 5. 스칼라 삼중적 마무리
			float tripleProduct = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&Camera::camForward), cross));
		}
	}

	// 오브젝트 목록에서 Plane 타입 찾기
	for (const auto& obj : mGameObjects)
	{
		Plane* plane = dynamic_cast<Plane*>(obj);
		if (plane != nullptr)
		{
			XMFLOAT3 pos = plane->GetPosition();
			if (ImGui::DragFloat3("Plane Position", &pos.x, 0.1f))
			{
				// 위치와 월드 매트릭스를 함께 갱신해줌
				plane->SetPosition(pos.x, pos.y, pos.z);
			}
			break; 
		}
	}

	
	
	ImGui::Text("Helicopter Parts Control");
	ImGui::Separator();

	// 람다 정의: 특정 오브젝트의 Transform UI를 그려주는 로컬 헬퍼
	auto DrawTransformUI = [](const char* label, auto* obj) {
		if (!obj) return;

		XMFLOAT3 pos = obj->GetPosition();
		XMFLOAT3 rot = obj->GetRotation();

		// ##ID 를 사용하여 내부 레이블 중복 방지 및 UI 깔끔하게 유지
		char bufPos[64], bufRot[64];
		sprintf_s(bufPos, "%s Position", label);
		sprintf_s(bufRot, "%s Rotation", label);

		if (ImGui::DragFloat3(bufPos, &pos.x, 0.1f))
			obj->SetPosition(pos);

		if (ImGui::DragFloat3(bufRot, &rot.x, 0.1f))
			obj->SetRotation(rot);

		ImGui::Spacing();
		};

	
	DrawTransformUI("Body", mHeliBody.get());
	DrawTransformUI("Blade", mHeliBlade.get());
	DrawTransformUI("Tail", mHeliTale.get());
			
		
	ImGui::Separator();

	ImGui::End();

	ImGui::Begin("Object Control");

	// 1. 오브젝트 선택 리스트 생성
	int cubeDisplayCount = 0; 
	if (ImGui::BeginListBox("Cube List"))
	{
		for (int i = 0; i < mGameObjects.size(); ++i)
		{
			// 큐브 타입만 골라서 리스트에 표시
			Cube* cube = dynamic_cast<Cube*>(mGameObjects[i]);
			if (cube)
			{
				char label[32];
				sprintf_s(label, "Cube %d (Index: %d)", cubeDisplayCount++, i);

				bool isSelected = (mSelectedIndex == i);
				if (ImGui::Selectable(label, isSelected))
				{
					mSelectedIndex = i;
				}

			
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndListBox();
	}

	ImGui::Separator();

	// 2. 선택된 큐브의 데이터 수정
	if (mSelectedIndex != -1)
	{
		GameObject* selectedObj = mGameObjects[mSelectedIndex];
		XMFLOAT3 pos = selectedObj->GetPosition(); 
		XMFLOAT3 rot = selectedObj->GetRotation(); 
		XMFLOAT4 forwardV = selectedObj->GetForwardVector();
		XMVECTOR forward = XMLoadFloat4(&forwardV);

		ImGui::Text("Editing: Cube %d", mSelectedIndex);
		if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
		{
			selectedObj->SetPosition(pos.x, pos.y, pos.z);
		}
		ImGui::Separator();
		if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f))
		{
			selectedObj->SetRotation(rot.x, rot.y, rot.z);
		}
	}
	else
	{
		ImGui::Text("Select a cube from the list.");
	}

    ImGui::End();
}

void Map2Scene::Release()
{
	mGameObjects.clear();
}