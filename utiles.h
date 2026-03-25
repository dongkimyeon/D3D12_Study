#pragma once
#include "stdafx.h"

namespace Utiles
{
   

    namespace Random
    {
		std::random_device g_randomDevice;
        inline INT GetInt(INT min, INT max)
        {
            std::uniform_int_distribution<INT> dis{ min, max };
            return dis(g_randomDevice);
        }
        inline FLOAT GetFloat(FLOAT min, FLOAT max)
        {
            std::uniform_real_distribution<FLOAT> dis{ min, max };
            return dis(g_randomDevice);
        }
    }

    namespace Vector3
    {
        inline XMFLOAT3 Add(const XMFLOAT3& a, const XMFLOAT3& b)
        {
            return XMFLOAT3{ a.x + b.x, a.y + b.y, a.z + b.z };
        }
        inline XMFLOAT3 Sub(const XMFLOAT3& a, const XMFLOAT3& b)
        {
            return XMFLOAT3{ a.x - b.x, a.y - b.y, a.z - b.z };
        }
        inline XMFLOAT3 Mul(const XMFLOAT3& a, const FLOAT& scalar)
        {
            return XMFLOAT3{ a.x * scalar, a.y * scalar, a.z * scalar };
        }
        inline XMFLOAT3 Negate(const XMFLOAT3& v)
        {
            return XMFLOAT3{ -v.x, -v.y, -v.z };
        }
        inline XMFLOAT3 Normalize(const XMFLOAT3& v)
        {
            XMFLOAT3 result;
            XMStoreFloat3(&result, XMVector3Normalize(XMLoadFloat3(&v)));
            return result;
        }
        inline XMFLOAT3 Cross(const XMFLOAT3& a, const XMFLOAT3& b)
        {
            XMFLOAT3 result;
            XMStoreFloat3(&result, XMVector3Cross(XMLoadFloat3(&a), XMLoadFloat3(&b)));
            return result;
        }
        inline XMFLOAT3 Angle(const XMFLOAT3& a, const XMFLOAT3& b, BOOL isNormalized = true)
        {
            XMFLOAT3 result;
            if (isNormalized) XMStoreFloat3(&result, XMVector3AngleBetweenNormals(XMLoadFloat3(&a), XMLoadFloat3(&b)));
            else XMStoreFloat3(&result, XMVector3AngleBetweenVectors(XMLoadFloat3(&a), XMLoadFloat3(&b)));
            return result;
        }

		//내적
		inline FLOAT Dot(const XMFLOAT3& a, const XMFLOAT3& b)
		{
			XMVECTOR dotVec = XMVector3Dot(XMLoadFloat3(&a), XMLoadFloat3(&b));
			return XMVectorGetX(dotVec); // 결과값 하나만 추출
		}

		

		// XMVECTOR를 XMFLOAT3로 바로 변환해주는 편의 함수
		inline XMFLOAT3 FromVector(FXMVECTOR v)
		{
			XMFLOAT3 result;
			XMStoreFloat3(&result, v);
			return result;
		}

    }
}