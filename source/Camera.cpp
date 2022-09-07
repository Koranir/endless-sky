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
#include <cmath>

using namespace std;

namespace {
	Point center2, cameraPos, interpolatedPos, staticPos, balanceVel = Point();
	Point cameraVelocity = Point();
//	double distToMin = 0.;
	double SMOOTHNESS = 0.02; //Very Smooth.
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
	/*distToMin = (center-cameraPos).Length()/min(Screen::Height(), Screen::Width());

	double x = max(min(distToMin,1.0), 0.0);
	//ouble smoothstep = x*x*(3-(2*x));
	SMOOTHNESS = 0.016;
	center2 = center;
	//cameraVelocity += (center-cameraPos)*SMOOTHNESS;
	cameraVelocity += centerVelocity.Unit()*(log(x+0.1)+1)*(center-cameraPos)*SMOOTHNESS;
	//cameraVelocity.Lerp(centerVelocity, SMOOTHNESS);
	//cameraVelocity *= 1+smoothstep;
	cameraPos += (cameraVelocity)*100/Screen::Zoom();
	if (locked)
	{
		cameraPos = staticPos.Lerp(center2, lockBlend);
	}*/

	center2 = center;
	Point facing = centerVelocity-cameraVelocity;
	if (facing.Unit().Dot(cameraVelocity.Unit()) < 0.)
		cameraVelocity = cameraVelocity.Lerp(centerVelocity, SMOOTHNESS);
	else
		cameraVelocity += (center-cameraPos)*SMOOTHNESS;
	cameraPos += (cameraVelocity)*100/Screen::Zoom();
	cameraPos = cameraPos.Lerp(center, SMOOTHNESS);
}

Point Camera::CameraOffset()
{
	return cameraPos-center2;
}

void Camera::SetCameraPosition(Point position)
{
	cameraPos = position;
}
