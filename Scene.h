#pragma once
#include "stdafx.h"
#include "Entity.h"

class Scene : public Entity
{
public:
	Scene() {}
	virtual ~Scene() {}

	virtual void Initialize() = 0; // 순수 가상 함수: 자식 클래스에서 무조건 구현해야 함
	virtual void Update(float dt) = 0;

	// D3D12 커맨드 리스트를 받아 개별 씬에서 오브젝트들을 그릴 수 있게 변경
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;

	virtual void Release() = 0;
};