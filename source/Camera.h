/* Camera.h
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

#ifndef CAMERA_H_
#define CAMERA_H_

#include "Point.h"



// Class that holds a few methods & values for manipulating the camera
// Position, Velocity, Offsets, Update Function
class Camera {
public:
	static void Update(Point center, Point centerVelocity, Point focus, bool locked, double lockBlend);

	static Point Position();
	static Point Velocity();
	static Point Offset();
	static Point VelocityOffset();

	static void SetPosition(Point Position);
	static void SetVelocity(Point Velocity);
	static void SetOffset(Point Offset, Point center);
	//Dangerous, uses old values
	static void SetOffset(Point Offset);
	static void SetLockedPosition(Point Position);

	static void Reset(Point center, Point centerVelocity);

	static void WhiteShake(double intensity);
	static void WhiteShake(double intensity, Point source);
};



#endif
