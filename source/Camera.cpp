/* Camera.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Camera.h"
#include "Screen.h"

#include <algorithm>

using namespace std;

namespace {
	Point center2, cameraPos, interpolatedPos, staticPos = Point();
	Point cameraVelocity = Point();
	double SMOOTHNESS = 0.02; //Very Smooth.
	//double DRAG = 0.01;
}

void Camera::SetSmoothOffset(Point offset)
{
	cameraPos = center2+offset;
}

void Camera::SetStaticCamera(Point offset)
{
	staticPos = offset;
}

Point Camera::StaticPoint()
{
	return staticPos;
}

void Camera::SetCameraOffset(Point center, Point centerVelocity, bool locked, double lockBlend, Point targetPos)
{
	center2 = center;
	cameraVelocity.Lerp(centerVelocity, SMOOTHNESS);
	//cameraVelocity += ((center-cameraPos)*SMOOTHNESS);
	//cameraVelocity *= DRAG;
	cameraPos += cameraVelocity;
	if (locked)
	{
		cameraPos = staticPos.Lerp(center2, lockBlend);
	}
}

Point Camera::CameraOffset()
{
	return center2 - cameraPos;
}
