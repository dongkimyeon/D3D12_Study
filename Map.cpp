#include "stdafx.h"
#include "Map.h"
#include "OBJLoader.h"

Map::Map()
{
}

Map::~Map()
{
}

void Map::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);

	OBJLoader::LoadBinary("map.bin", vertices, indices);

	for (size_t i = 0; i + 2 < vertices.size(); i += 3)
	{
		OBJVertex& v0 = vertices[i];
		OBJVertex& v1 = vertices[i + 1];
		OBJVertex& v2 = vertices[i + 2];

		float e1x = v1.x - v0.x, e1y = v1.y - v0.y, e1z = v1.z - v0.z;
		float e2x = v2.x - v0.x, e2y = v2.y - v0.y, e2z = v2.z - v0.z;

		float nx = e1y * e2z - e1z * e2y;
		float ny = e1z * e2x - e1x * e2z;
		float nz = e1x * e2y - e1y * e2x;
		float len = sqrtf(nx * nx + ny * ny + nz * nz);
		if (len > 0.0f) { nx /= len; ny /= len; nz /= len; }

		v0.nx = v1.nx = v2.nx = nx;
		v0.ny = v1.ny = v2.ny = ny;
		v0.nz = v1.nz = v2.nz = nz;

		float r = 0.6f + 0.4f * fabsf(nx);
		float g = 0.6f + 0.4f * fabsf(ny);
		float b = 0.6f + 0.4f * fabsf(nz);
		v0.r = v1.r = v2.r = r;
		v0.g = v1.g = v2.g = g;
		v0.b = v1.b = v2.b = b;
		v0.a = v1.a = v2.a = 1.0f;
	}

	CreateBuffersFromData(device);

	BakeScale(0.020f, 0.02f, 0.020f);
}

void Map::AddInstance(XMFLOAT3 position, float yaw)
{
	XMMATRIX world = XMMatrixRotationY(XMConvertToRadians(yaw))
		* XMMatrixTranslation(position.x, position.y, position.z);
	XMFLOAT4X4 w;
	XMStoreFloat4x4(&w, world);
	mInstances.push_back(w);
}

void Map::ClearInstances()
{
	mInstances.clear();
}

void Map::BuildInstanceBuffer(ComPtr<ID3D12Device> device)
{
	if (mInstances.empty()) return;

	CreateInstanceBuffer(device, (UINT)mInstances.size());

	void* ptr;
	mInstanceBufferUpload->Map(0, nullptr, &ptr);
	memcpy(ptr, mInstances.data(), mInstances.size() * sizeof(XMFLOAT4X4));
	mInstanceBufferUpload->Unmap(0, nullptr);
	mInstDirty = true;
}

void Map::UpdateInstancesForCulling(const std::vector<XMFLOAT4X4>& visible)
{
	if (!mInstanceBufferUpload) return;

	mInstanceCount = (UINT)visible.size();
	if (visible.empty()) return;

	void* ptr;
	mInstanceBufferUpload->Map(0, nullptr, &ptr);
	memcpy(ptr, visible.data(), visible.size() * sizeof(XMFLOAT4X4));
	mInstanceBufferUpload->Unmap(0, nullptr);
	mInstDirty = true;
}

void Map::Update(float dt)
{
	GameObject::Update(dt);
}
