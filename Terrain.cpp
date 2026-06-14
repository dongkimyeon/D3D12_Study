#include "Terrain.h"
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

static std::vector<float> LoadHeightmapPNG(const wchar_t* path, int& outW, int& outH)
{
    outW = outH = 0;

    IWICImagingFactory* factory = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
        return {};

    IWICBitmapDecoder* decoder = nullptr;
    if (FAILED(factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnDemand, &decoder)))
    { factory->Release(); return {}; }

    IWICBitmapFrameDecode* frame = nullptr;
    decoder->GetFrame(0, &frame);

    IWICFormatConverter* conv = nullptr;
    factory->CreateFormatConverter(&conv);
    conv->Initialize(frame, GUID_WICPixelFormat8bppGray,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);

    UINT w, h;
    conv->GetSize(&w, &h);
    outW = (int)w; outH = (int)h;

    std::vector<BYTE> pixels(w * h);
    conv->CopyPixels(nullptr, w, (UINT)pixels.size(), pixels.data());

    std::vector<float> result(w * h);
    for (size_t i = 0; i < pixels.size(); i++)
        result[i] = pixels[i] / 255.0f;

    conv->Release(); frame->Release(); decoder->Release(); factory->Release();
    return result;
}

Terrain::Terrain()
{

}
Terrain::~Terrain()
{

}

void Terrain::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);

	const int   GRID_X  = 100;
	const int   GRID_Z  = 100;
	const float START_X = -200.0f;
	const float START_Z = -100.0f;
	const float WIDTH   =  400.0f;
	const float DEPTH   =  400.0f;
	const float MIN_Y   =    0.0f;
	const float MAX_Y   =   20.0f;

	mStartX = START_X; mStartZ = START_Z;
	mWidth  = WIDTH;   mDepth  = DEPTH;
	mMaxY   = MAX_Y;

	const int numVX = GRID_X + 1;
	const int numVZ = GRID_Z + 1;

	mHmData = LoadHeightmapPNG(L"heightMap.png", mHmW, mHmH);
	bool useHeightmap = !mHmData.empty();

	std::mt19937 rng(1234);
	std::uniform_real_distribution<float> dist(MIN_Y, MAX_Y);

	vertices.resize(numVX * numVZ);
	for (int z = 0; z < numVZ; z++)
	{
		for (int x = 0; x < numVX; x++)
		{
			float px = START_X + (float)x / GRID_X * WIDTH;
			float pz = START_Z + (float)z / GRID_Z * DEPTH;

			float py;
			if (useHeightmap)
			{

				float u  = (float)x / GRID_X * (mHmW - 1);
				float v  = (float)z / GRID_Z * (mHmH - 1);
				int   x0 = std::clamp((int)u, 0, mHmW - 1);
				int   x1 = std::clamp(x0 + 1,  0, mHmW - 1);
				int   z0 = std::clamp((int)v, 0, mHmH - 1);
				int   z1 = std::clamp(z0 + 1,  0, mHmH - 1);
				float fx = u - x0, fz = v - z0;
				float h00 = mHmData[z0 * mHmW + x0];
				float h10 = mHmData[z0 * mHmW + x1];
				float h01 = mHmData[z1 * mHmW + x0];
				float h11 = mHmData[z1 * mHmW + x1];
				py = (h00 * (1 - fx) + h10 * fx) * (1 - fz)
				   + (h01 * (1 - fx) + h11 * fx) * fz;
				py *= MAX_Y;
			}
			else
			{
				py = dist(rng);
			}

			vertices[z * numVX + x] = { px, py, pz,  0.0f, 1.0f, 0.0f,  0.4f, 0.7f, 0.3f, 1.0f };
		}
	}

	indices.reserve(GRID_X * GRID_Z * 6);
	for (int z = 0; z < GRID_Z; z++)
	{
		for (int x = 0; x < GRID_X; x++)
		{
			uint32_t tl = z * numVX + x;
			uint32_t tr = tl + 1;
			uint32_t bl = (z + 1) * numVX + x;
			uint32_t br = bl + 1;
			indices.push_back(tl); indices.push_back(bl); indices.push_back(tr);
			indices.push_back(tr); indices.push_back(bl); indices.push_back(br);
		}
	}

	std::vector<XMFLOAT3> normAccum(vertices.size(), { 0.0f, 0.0f, 0.0f });
	for (size_t i = 0; i < indices.size(); i += 3)
	{
		auto& v0 = vertices[indices[i]];
		auto& v1 = vertices[indices[i + 1]];
		auto& v2 = vertices[indices[i + 2]];
		XMVECTOR n = XMVector3Normalize(XMVector3Cross(
			XMVectorSet(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z, 0),
			XMVectorSet(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z, 0)));
		XMFLOAT3 nf; XMStoreFloat3(&nf, n);
		for (int j = 0; j < 3; j++)
		{
			normAccum[indices[i + j]].x += nf.x;
			normAccum[indices[i + j]].y += nf.y;
			normAccum[indices[i + j]].z += nf.z;
		}
	}
	for (size_t i = 0; i < vertices.size(); i++)
	{
		XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&normAccum[i]));
		XMFLOAT3 nf; XMStoreFloat3(&nf, n);
		vertices[i].nx = nf.x;
		vertices[i].ny = nf.y;
		vertices[i].nz = nf.z;
	}

	CreateBuffersFromData(device);
}

void Terrain::Update(float dt)
{
	GameObject::Update(dt);
}

void Terrain::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	GameObject::Render(commandList, view, proj);
}

float Terrain::GetHeightAt(float wx, float wz) const
{
	XMFLOAT3 pos = GetPosition();
	XMFLOAT3 sc  = GetScale();
	float lx = (wx - pos.x) / sc.x;
	float lz = (wz - pos.z) / sc.z;

	float u = (lx - mStartX) / mWidth;
	float v = (lz - mStartZ) / mDepth;

	if (u < 0.f || u > 1.f || v < 0.f || v > 1.f)
		return position.y;

	if (mHmData.empty())
		return position.y;

	float fx = u * (mHmW - 1);
	float fz = v * (mHmH - 1);
	int x0 = std::clamp((int)fx, 0, mHmW - 1);
	int x1 = std::clamp(x0 + 1,  0, mHmW - 1);
	int z0 = std::clamp((int)fz, 0, mHmH - 1);
	int z1 = std::clamp(z0 + 1,  0, mHmH - 1);
	float tx = fx - x0, tz = fz - z0;

	float h00 = mHmData[z0 * mHmW + x0];
	float h10 = mHmData[z0 * mHmW + x1];
	float h01 = mHmData[z1 * mHmW + x0];
	float h11 = mHmData[z1 * mHmW + x1];
	float h = (h00*(1-tx) + h10*tx)*(1-tz) + (h01*(1-tx) + h11*tx)*tz;

	return h * mMaxY * sc.y + pos.y;
}
