/* Shader.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef FRAME_BUFFER_H_
#define FRAME_BUFFER_H_

#include "opengl.h"

#include <string>
#include <vector>



// Class representing a shader, i.e. a compiled GLSL program that the GPU uses
// in order to draw something. In modern GPL, everything is drawn with shaders.
// In general, rather than using this class directly, drawing code will use one
// of the classes representing a particular shader.
class FrameBuffer {
public:
	enum class bufferType : GLint{
		frame = 0,
		texture = 1,
		mask = 2
	};
	static bool CreateFrameBuffer(bufferType type, GLuint *fbo, int width, int height);
	static bool SetFrameBuffer(GLuint fbo = 0);
	static bool RemoveFrameBuffer(GLuint *fbo);
private:



private:
	static std::vector<GLuint*> buffers;
	static GLuint texture;
};



#endif
