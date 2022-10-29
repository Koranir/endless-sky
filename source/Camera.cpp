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
#include "Preferences.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Screen.h"

#include <cmath>

using namespace std;

namespace {
	static Point position, sPosition, lPosition, velocity, offset, vOffset, oldCenter = Point();
	static double screenShake = 0.;
	static double radius = 0.;
	static const double SMOOTHNESS = 0.012;
}



void Camera::Update(Point center, Point centerVelocity, Point focus, bool locked, double lockBlend)
{
	// Camera accelerates quickly when moving in the same direction as the centerVelocity
	Point facing = (centerVelocity-velocity).Unit();
	velocity = velocity.Lerp(centerVelocity*(1 + max(0., facing.Dot(velocity.Unit()))), SMOOTHNESS);

	// The camera is moved in this step
	sPosition += (velocity)*100/Screen::Zoom();
	sPosition = sPosition.Lerp(center, SMOOTHNESS*2);

	// Lock-on
	// The distance from the camera's position is capped already
	position = sPosition.Lerp(focus, 0.4);

	// Interpolates the camera's position to the locked position by lockBlend
	position = position.Lerp(lPosition, lockBlend);

	// Simple camera shake, enabled by a setting (on by default).
	if (Preferences::Has("Enable screen shake"))
		position += Point(sin((Random::Real()-0.5)*3.14), sin((Random::Real()-0.5)*3.14))*min(screenShake/2., Screen::Height()/2.);
	screenShake *= 0.925;

	// Just calculates some dependant variables while the center variable is still available.
	offset = position - center;
	vOffset = velocity - centerVelocity;
	oldCenter = center;
}



Point Camera::Position()
{
	return position;
}



Point Camera::Velocity()
{
	return velocity;
}



Point Camera::Offset()
{
	return offset;
}



Point Camera::VelocityOffset()
{
	return vOffset;
}



void Camera::SetPosition(Point Position)
{
	position = Position;
	sPosition = Position;
}



void Camera::SetVelocity(Point Velocity)
{
	velocity = Velocity;
}



void Camera::SetLockedPosition(Point Position)
{
	lPosition = Position;
}



void Camera::Reset(Point center, Point centerVelocity)
{
	position = center;
	sPosition = center;
	lPosition = center;
	oldCenter = center;
	velocity = centerVelocity;
	offset = Point();
	vOffset = Point();
}



void Camera::SetRadius(double Radius)
{
	radius = Radius;
}



void Camera::ScreenShake(double intensity)
{
	screenShake += intensity*0.005;
}



void Camera::ScreenShake(double intensity, Point source)
{
	screenShake += sqrt(intensity*5)*max(0.1, Screen::Height()/2. - screenShake)/(max((oldCenter-source).LengthSquared(), 4*radius*radius));
}
