#pragma once
#include "Scene.h"
#include "GameObject.h"

class TestScene : public Scene
{
public:
    TestScene();
    virtual ~TestScene();

    virtual void Initialize() override;
    virtual void Update(float dt) override;
    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) override;
    virtual void Release() override;

private:
    std::vector<GameObject*> mGameObjects;
	int mSelectedIndex = -1; // -1은 아무것도 선택되지 않음, 0 이상은 인덱스
 
};