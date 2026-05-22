#pragma once
#include "stdafx.h"
#include "Framework.h"
#include "Camera.h"
#include <DirectXCollision.h>


inline bool PickAABB(const DirectX::BoundingBox& worldAABB, float screenW = 1280.f, float screenH = 720.f)
{
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(Framework::GetHwnd(), &pt);

    // 클라이언트 좌표 → NDC
    float ndcX = (2.f * pt.x / screenW) - 1.f;
    float ndcY = 1.f - (2.f * pt.y / screenH);

    XMMATRIX view = XMMatrixLookToLH(
        XMLoadFloat3(&Camera::camPos),
        XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0),
        XMVectorSet(0, 1, 0, 0));
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, screenW / screenH, 0.1f, 100.f);

    XMMATRIX invProj = XMMatrixInverse(nullptr, proj);
    XMMATRIX invView = XMMatrixInverse(nullptr, view);

    // 클립 공간 → 뷰 공간 방향
    XMVECTOR rayViewDir = XMVector3TransformCoord(
        XMVectorSet(ndcX, ndcY, 1.f, 0.f), invProj);
    rayViewDir = XMVectorSetW(rayViewDir, 0.f);

    // 뷰 공간 → 월드 공간 방향
    XMVECTOR rayWorldDir = XMVector3Normalize(
        XMVector3TransformNormal(rayViewDir, invView));
    XMVECTOR rayOrigin = XMLoadFloat3(&Camera::camPos);

    float dist;
    return worldAABB.Intersects(rayOrigin, rayWorldDir, dist);
}
