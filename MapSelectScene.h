#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Map1.h"
#include "Map2.h"

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
	Map1* mMap1 = nullptr;
	Map2* mMap2 = nullptr;
};