#pragma once

#include "stdafx.h"

class Camera
{
public:
	static void Update(float dt);
	static void SetPosition(float x, float y, float z) { camPos = { x, y, z }; }

	static XMFLOAT3 camPos;
	static XMFLOAT3 camForward;
	static XMFLOAT3 camRight;
	static XMFLOAT3 camUp;

	static float camYaw;
	static float camPitch;
	static float moveSpeed;
	static float lookSpeed;

	static bool isRotating;
	static POINT prevMousePos;
};