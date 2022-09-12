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

class Ship;

// Class that holds and sets offsets for the camera.
class Camera {
public:
	//Screen Offsets
	//Sets camera offsets
	//Static Point is relative to the origin
	static void SetStaticCamera(Point offset);
	//returns the static point
	static Point StaticPoint();
	//Passes a lot of engine variables to calculate the offset from the player
	//center is the player position
	//centerVelocity is the velocity of the player, used to calculate the base offset
	//locked determines whether the camera is using the offset or the static point
	//lockBlend is used to lerp from the offset to the static point
	//targetpos is the position of the target
	static void SetCameraOffset(Point center, Point centerVelocity, bool locked, double lockBlend, Point targetPos);
	//returns the offset relative to player
	static Point CameraOffset();
	//returns the offset relative to the origin
	static Point CameraPos();
	//Set Camera Position
	static void SetCameraPosition(Point position);
	//Set Camera Velocity
	static void SetCameraVelocity(Point velocity);
	//returns the zoom modifier
	static double ZoomMod();
	//Sets the xoom modifier
	static void SetZoomMod();
	//Zoom Update Function
	static void UpdateZoomMod(const Ship &flagship);
};



#endif
