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



// Class that holds a few methods & values for the camera
// Update() is the main function, it calculates a new position for the camera every frame based on the given variables.
// Position(), Velocity(), Offset(), and VelocityOffset() all return their respective variables.
// Offset() is the most used function.
// The Set...() functions just set their variables to those given.
// SetPosition() assigns to two variables.
// Reset() changes all the variables to those as if there was no dynamic camera i.e the offset becomes zero.
// WhiteShake() is a very simple implementation of screenshake, it can be called with a source point or directly.
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
	static void SetLockedPosition(Point Position);

	static void Reset(Point center, Point centerVelocity);

	static void SetRadius(double Radius);

	static void WhiteShake(double intensity);
	static void WhiteShake(double intensity, Point source);
};



#endif
