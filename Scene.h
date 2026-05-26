#pragma once
#include "stdafx.h"

class Scene
{
public:
	Scene() {}
	virtual ~Scene() {}

	virtual void Initialize() = 0; // ���� ���� �Լ�: �ڽ� Ŭ�������� ������ �����ؾ� ��
	virtual void Update(float dt) = 0;

	// D3D12 Ŀ�ǵ� ����Ʈ�� �޾� ���� ������ ������Ʈ���� �׸� �� �ְ� ����
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;

	virtual void Release() = 0;
};