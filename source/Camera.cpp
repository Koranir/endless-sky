/* Camera.cpp
Copyright (c) 2022 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Camera.h"
#include "Random.h"
#include "Screen.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace std;

namespace {
	Point cameraPos, interpolatedPos, staticPos, cameraVelocity = Point();
	static const double SMOOTHNESS = 0.016;
	double INTENSITY = 0.;
	Point CENTER = Point();
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
	CENTER = center;
	Point facing = centerVelocity-cameraVelocity;
	if (facing.Unit().Dot(cameraVelocity.Unit()) > 0.)
		cameraVelocity = cameraVelocity.Lerp(centerVelocity*1.5, SMOOTHNESS);
	else
		cameraVelocity = cameraVelocity.Lerp(centerVelocity, SMOOTHNESS);
	cameraPos += (cameraVelocity)*100/Screen::Zoom();
	cameraPos = cameraPos.Lerp(center, SMOOTHNESS*2);
	if (!(targetPos == center))
		interpolatedPos = cameraPos.Lerp(targetPos, 0.4);
	else
		interpolatedPos = cameraPos;
	interpolatedPos = interpolatedPos.Lerp(staticPos, lockBlend);
	interpolatedPos -= center;
	interpolatedPos += Point(Random::Real()-0.5, Random::Real()-0.5)*INTENSITY*Screen::Width();
	INTENSITY *= 0.925;
}

Point Camera::CameraOffset()
{
	return interpolatedPos;
}

void Camera::SetCameraPosition(Point position)
{
	cameraPos = position;
	interpolatedPos = position;
}

void Camera::SetCameraVelocity(Point velocity)
{
	cameraVelocity = velocity;
}

void Camera::ShakeCamera(double intensity)
{
	INTENSITY += 0.00001*intensity;
}

void Camera::ShakeCamera(double intensity, Point origin)
{
	double len = (CENTER-origin).Length();
	INTENSITY += (0.00001*intensity)/min(len*len, 1.);
}
