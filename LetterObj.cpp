#include "LetterObj.h"

LetterObj::LetterObj()
{
}

LetterObj::~LetterObj()
{
}

void LetterObj::Initialize(ComPtr<ID3D12Device> device, const std::string& text)
{
	GameObject::Initialize(device);

	LoadFromOBJ(text, device);
	BakeScale(0.05f, 0.05f, 0.05f);
	BakeRotation(0.0f, 90.0f, 0.0f);
}