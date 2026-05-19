#include "TitleScene.h"
#include "SceneManager.h"
#include "Title.h"
#include "Camera.h"
#include "framework.h"
TitleScene::TitleScene()
{
}

TitleScene::~TitleScene()
{
}

void TitleScene::Initialize()
{
	Camera::camPos = { 0, 0, -25};
	Camera::camPitch = 0;


	Title* title = new Title();
	title->Initialize(Framework::GetDevice());
	title->SetPosition(0, 3.0f, 0);
	title->SetScale(0.05f, 0.05f, 0.05f);
	title->SetRotation(0, 90.0f, 0);
	mGameObjects.push_back(title);

	mStart = new Start();
	mStart->Initialize(Framework::GetDevice());
	mStart->SetPosition(0, -5.0f, 0);
	mStart->SetScale(0.05f, 0.05f, 0.05f);
	mStart->SetRotation(0, 90.0f, 0);
	mGameObjects.push_back(mStart);

}

void TitleScene::Update(float dt)
{
	for (const auto& obj : mGameObjects) {
		obj->Update(dt);
	}



	// 마우스 좌클릭으로 Start 객체 피킹
	if (Input::GetKeyDown(eKeyCode::LButton) && mStart)
	{
		// 마우스 위치를 클라이언트 좌표로 변환
		POINT mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(Framework::GetHwnd(), &mousePos);

		float w = static_cast<float>(Framework::GetWidth());
		float h = static_cast<float>(Framework::GetHeight());

		// NDC 좌표
		float ndcX = (2.0f * mousePos.x / w) - 1.0f;
		float ndcY = 1.0f - (2.0f * mousePos.y / h);

		// View / Proj 행렬 (Render와 동일한 파라미터)
		XMMATRIX view = XMMatrixLookToLH(
			XMLoadFloat3(&Camera::camPos),
			XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0),
			XMVectorSet(0, 1, 0, 0));
		XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, w / h, 0.1f, 100.0f);

		// 역행렬로 NDC → 월드 공간 언프로젝션
		XMMATRIX invVP = XMMatrixInverse(nullptr, view * proj);
		XMVECTOR nearPt = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 0.0f, 1.0f), invVP);
		XMVECTOR farPt  = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 1.0f, 1.0f), invVP);

		XMVECTOR rayOrigin = XMLoadFloat3(&Camera::camPos);
		XMVECTOR rayDir    = XMVector3Normalize(farPt - nearPt);

		// Start 월드 AABB와 레이 교차 검사
		float dist = 0.0f;
		DirectX::BoundingBox aabb = mStart->GetWorldAABB();
		if (aabb.Intersects(rayOrigin, rayDir, dist))
		{
			SceneManager::LoadScene(L"MapSelectScene");
		}
	}
}

void TitleScene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&Camera::camPos), XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0), XMVectorSet(0, 1, 0, 0));
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 100.0f);



	for (const auto& obj : mGameObjects) {
		obj->Render(commandList, view, proj);
	}
}

void TitleScene::Release()
{
}
