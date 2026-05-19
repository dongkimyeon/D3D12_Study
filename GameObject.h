#pragma once
#include "stdafx.h" // OBJVertex, DirectXMath 등 포함

class GameObject
{
public:
    GameObject();
    virtual ~GameObject();

    virtual void Initialize(ComPtr<ID3D12Device> device);
    virtual void Update(float dt);

    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj);

    void LoadFromOBJ(const std::string& filename, ComPtr<ID3D12Device> device);

	/*-------------------------GETTER--------------------------*/
	XMFLOAT4X4 GetWorldMatrix() const { return worldMatrix; }
	XMFLOAT3 GetPosition() const { return position; }
	XMFLOAT3 GetRotation() const { return rotation; }
	XMFLOAT3 GetScale() const { return scale; }
	XMFLOAT4 GetForwardVector() const { return forward_vector; }

	/*-------------------------SETTER--------------------------*/
	void SetWorldMatrix(const  XMFLOAT4X4 matrix) { worldMatrix = matrix; }

	void SetPosition(float x, float y, float z) { position = { x, y, z }; }
	void SetPosition(XMFLOAT3 pos) { position = pos; }

	void SetRotation(float pitch, float yaw, float roll) { rotation = { XMConvertToRadians(pitch), XMConvertToRadians(yaw), XMConvertToRadians(roll) }; }
	void SetRotation(XMFLOAT3 rot) { rotation = rot; }

	void SetScale(float scaleX, float scaleY, float scaleZ) { scale = { scaleX, scaleY, scaleZ }; }
	void SetScale(XMFLOAT3 s) { scale = s; }

	void BakeScale(float sx, float sy, float sz);
	void BakeRotation(float pitch, float yaw, float roll);
	void BakeRotationX(float angleDeg);
	void BuildNormalBuffer(ComPtr<ID3D12Device> device);
	void SetAlpha(float alpha);

	// AABB 와이어프레임 디버그
	static bool sShowAABB;
	virtual DirectX::BoundingBox GetWorldAABB() const;
	void RenderAABB(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj);


protected:
    // 업로드 힙 스테이징 버퍼를 갱신하고 더티 플래그 설정
    void UpdateVertexBuffer();

    // vertices/indices로부터 업로드+디폴트 힙 버퍼를 생성 (Gizumo, Plane 등 직접 버텍스 설정 시 사용)
    void CreateBuffersFromData(ComPtr<ID3D12Device> device);

    // 더티 버퍼를 디폴트 힙으로 복사하고 배리어를 전환하는 헬퍼
    void UploadBufferIfDirty(
        ComPtr<ID3D12GraphicsCommandList>& cmdList,
        ComPtr<ID3D12Resource>& gpuBuf,
        ComPtr<ID3D12Resource>& uploadBuf,
        D3D12_RESOURCE_STATES& currentState,
        D3D12_RESOURCE_STATES targetState,
        UINT64 byteSize,
        bool& dirty);

protected:
    XMFLOAT3 position;
	XMFLOAT3 rotation;
	XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	XMFLOAT4 forward_vector = { 0, 0, 1, 0 }; // 초기 전방 벡터 (Z축 방향)
	XMFLOAT4X4 worldMatrix;
protected:
    std::vector<OBJVertex> vertices;
    std::vector<uint16_t> indices;

    

    // 렌더링용 디폴트 힙 버퍼
    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vbView;
    D3D12_INDEX_BUFFER_VIEW ibView;

    // CPU→GPU 복사용 업로드 힙 스테이징 버퍼
    ComPtr<ID3D12Resource> vertexBufferUpload;
    ComPtr<ID3D12Resource> indexBufferUpload;

	// 노멀 라인 렌더링을 위한 멤버 변수들
	ComPtr<ID3D12Resource> normalVertexBuffer;
	ComPtr<ID3D12Resource> normalIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW normalVbView = {};
	D3D12_INDEX_BUFFER_VIEW normalIbView = {};
	UINT normalIndexCount = 0;

    // 노멀 버퍼 업로드 힙 스테이징 버퍼
	ComPtr<ID3D12Resource> normalVertexBufferUpload;
	ComPtr<ID3D12Resource> normalIndexBufferUpload;

    // 디폴트 힙 버퍼 현재 상태 추적
    D3D12_RESOURCE_STATES mVBState    = D3D12_RESOURCE_STATE_COPY_DEST;
    D3D12_RESOURCE_STATES mIBState    = D3D12_RESOURCE_STATE_COPY_DEST;
    D3D12_RESOURCE_STATES mNVBState   = D3D12_RESOURCE_STATE_COPY_DEST;
    D3D12_RESOURCE_STATES mNIBState   = D3D12_RESOURCE_STATE_COPY_DEST;

    // 업로드 힙→디폴트 힙 복사 필요 여부
    bool mVBDirty      = false;
    bool mIBDirty      = false;
    bool mNormalsDirty = false;

    // AABB 단위 박스 와이어프레임 (모든 인스턴스 공유)
    static void EnsureAABBMesh();
    static ComPtr<ID3D12Resource>   sAABBVB;
    static ComPtr<ID3D12Resource>   sAABBIB;
    static D3D12_VERTEX_BUFFER_VIEW sAABBVbView;
    static D3D12_INDEX_BUFFER_VIEW  sAABBIbView;
};