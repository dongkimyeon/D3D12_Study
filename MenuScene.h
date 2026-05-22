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
	LetterObj* Tutorial;
	LetterObj* Level_1;
	LetterObj* Level_2;
	LetterObj* Level_3;
	LetterObj* Start;
	LetterObj* End;
};

