#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "LetterObj.h"
class MenuScene : public Scene
{
public:
	MenuScene();
	virtual ~MenuScene();

	virtual void Initialize() override;
	virtual void Update(float dt) override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	virtual void Release() override;

private:
	std::vector<GameObject*> mGameObjects;
	LetterObj* Tutorial = nullptr;
	LetterObj* Level_1  = nullptr;
	LetterObj* Level_2  = nullptr;
	LetterObj* Level_3  = nullptr;
	LetterObj* Start    = nullptr;
	LetterObj* End      = nullptr;
};

