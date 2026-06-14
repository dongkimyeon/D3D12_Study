#pragma once
#include "GameObject.h"
#include <DirectXCollision.h>

class HeliBody : public GameObject
{
public:
	HeliBody();
	virtual ~HeliBody();

	virtual void Initialize(ComPtr<ID3D12Device> device) override;
	virtual bool UseOBB() const override { return true; }

private:

};

