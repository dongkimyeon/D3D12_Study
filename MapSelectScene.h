#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Start.h"

class MapSelectScene : public Scene
{
public:
	MapSelectScene();
	virtual ~MapSelectScene();

	virtual void Initialize() override;
	virtual void Update(float dt) override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	virtual void Release() override;

private:
	std::vector<GameObject*> mGameObjects;
	Start* mStart = nullptr;
};