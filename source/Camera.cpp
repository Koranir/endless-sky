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
	Point center2 = Point();
	Point baseOffset = Point();
	Point smoothOffset = Point();
	Point trueOffset = Point();
	Point frozenOffset = Point();
	Point finalOffset = Point();
}

void Camera::SetSmoothOffset(Point offset)
{
	smoothOffset = offset;
}

void Camera::SetStaticCamera(Point offset)
{
	frozenOffset = offset;
}

Point Camera::StaticPoint()
{
	return frozenOffset;
}

void Camera::SetCameraOffset(Point center, Point centerVelocity, bool locked, double lockBlend, Point targetPos)
{
	center2 = center;
	//Point OldOffset = CAMERA_OFFSET;
	baseOffset = centerVelocity*-0.08*Screen::Height();
	if (smoothOffset.Distance(baseOffset) > max(Screen::Height(), Screen::Width())*1.2)
		smoothOffset = smoothOffset.Lerp(baseOffset, 0.16);
	else
		smoothOffset = smoothOffset.Lerp(baseOffset, 0.02);
	trueOffset = baseOffset - smoothOffset;
	double x = trueOffset.Length()/Screen::RawHeight();
		trueOffset = trueOffset.Unit() * x*x*(3-(2*x)) * Screen::RawHeight();
	if (locked)
	{
		finalOffset = (frozenOffset.Lerp(center, lockBlend) - center);
		smoothOffset = finalOffset;
	}
	else
	{
		finalOffset = trueOffset.Lerp(frozenOffset - center, lockBlend);
		finalOffset = finalOffset.Lerp(targetPos, 0.5);
	}
}

Point Camera::CameraOffset()
{
	return finalOffset;
}

Point Camera::TrueOffset()
{
	return trueOffset;
}

Point Camera::CameraPos()
{
	return center2 + finalOffset;
}
