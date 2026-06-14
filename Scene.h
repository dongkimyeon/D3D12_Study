#pragma once
#include "stdafx.h"
#include "Entity.h"

class Scene : public Entity
{
public:
	Scene() {}
	virtual ~Scene() {}

	virtual void Initialize() = 0;
	virtual void Update(float dt) = 0;

	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;

	virtual void Release() = 0;
};