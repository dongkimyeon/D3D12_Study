#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Start.h"

class TitleScene : public Scene
{
public:
	TitleScene();
	virtual ~TitleScene();

	virtual void Initialize() override;
	virtual void Update(float dt) override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	virtual void Release() override;

private:
	std::vector<GameObject*> mGameObjects;
	Start* mStart = nullptr;
};