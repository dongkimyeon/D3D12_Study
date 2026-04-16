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

    // 위치(x, y, z)를 묶어서 동일한 정점인지 식별할 맵 생성
    std::map<std::tuple<float, float, float>, std::tuple<float, float, float>> colorMap;

    for (auto& v : vertices)
    {
        // 부동소수점 오차를 방지하기 위해 소수점 둘째 자리까지 반올림하여 비교 그룹을 만듬
        float kx = round(v.x * 100.0f);
        float ky = round(v.y * 100.0f);
        float kz = round(v.z * 100.0f);
        auto key = std::make_tuple(kx, ky, kz);

        // 해당 좌표에 아직 색상이 할당되지 않았다면 새로 무작위 색상 생성
        if (colorMap.find(key) == colorMap.end())
        {
            colorMap[key] = std::make_tuple(dis(gen), dis(gen), dis(gen));
        }

        // 해당 위치로 지정된 공통 색상을 정점에 적용
        v.r = std::get<0>(colorMap[key]);
        v.g = std::get<1>(colorMap[key]);
        v.b = std::get<2>(colorMap[key]);
        v.a = 1.0f;
    }

    // 변경된 데이터를 VRAM에 다시 업로드
    UpdateVertexBuffer();
	BuildNormalBuffer(device);
}

void Cube::Update(float dt)
{

	//{
	//	// 1. 현재의 Pitch, Yaw, Roll을 기반으로 회전 행렬 생성
	//	XMMATRIX rotationMat = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);


	//	XMVECTOR baseForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); //기준 전방 벡터 Z축
	//	XMVECTOR lookDirection = XMVector3TransformNormal(baseForward, rotationMat);


	//	//계산한 바라보는 벡터 방향을 forward_vector에 저장
	//	XMStoreFloat4(&forward_vector, lookDirection);

	//	float speed = 10.0f;
	//	XMVECTOR moveVector = lookDirection * speed * dt;
	//	//해당 바라 보는 벡터 방향으로 스칼라 곱연산

	//	//현재 위치를 XMVECTOR로 변환(레지스터로 연산을 빠르게 하기위함)
	//	XMVECTOR currentPosition = XMLoadFloat3(&position);

	//	//현재 위치 벡터에 이동 벡터를 더하여 새로운 위치 계산
	//	currentPosition = XMVectorAdd(currentPosition, moveVector);

	//	//바뀐 위치 VECTOR를 다시 XMFLOAT3로 변환하여 position에 저장
	//	XMStoreFloat3(&position, currentPosition);
	//}


	GameObject::Update(dt);
}
	
	


void Cube::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    GameObject::Render(commandList, view, proj);
}