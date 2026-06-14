#pragma once
#include "GameObject.h"

class LetterObj : public GameObject
{
public:
	LetterObj();
	virtual ~LetterObj();

	void Initialize(ComPtr<ID3D12Device> device, const std::string& text);
};
