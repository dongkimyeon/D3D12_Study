#include "stdafx.h"
#include "Cube.h"
#include <random>
#include <map>
#include <tuple>

Cube::Cube()
{
}

Cube::~Cube()
{
}

void Cube::Initialize(ComPtr<ID3D12Device> device)
{
    GameObject::Initialize(device);

    LoadFromOBJ("cube.obj", device);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    std::map<std::tuple<float, float, float>, std::tuple<float, float, float>> colorMap;

    for (auto& v : vertices)
    {

        float kx = round(v.x * 100.0f);
        float ky = round(v.y * 100.0f);
        float kz = round(v.z * 100.0f);
        auto key = std::make_tuple(kx, ky, kz);

        if (colorMap.find(key) == colorMap.end())
        {
            colorMap[key] = std::make_tuple(dis(gen), dis(gen), dis(gen));
        }

        v.r = std::get<0>(colorMap[key]);
        v.g = std::get<1>(colorMap[key]);
        v.b = std::get<2>(colorMap[key]);
        v.a = 1.0f;
    }

    UpdateVertexBuffer();
}

void Cube::Update(float dt)
{

	GameObject::Update(dt);
}
	
	

void Cube::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    GameObject::Render(commandList, view, proj);
}