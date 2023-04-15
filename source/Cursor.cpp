/* Cursor.cpp
Copyright (c) 2023 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Cursor.h"

#include "Point.h"
#include "Screen.h"
#include "Shader.h"
#include "Sprite.h"
#include "SpriteSet.h"

namespace {
	const Sprite *cursorTex;

	Point lastPos[8];
	const int POSITION_SIZE = 8;
	const int INTERPOLATION_NUM = 200;

	Shader shader;

	GLint positionI;
	GLint texI;
	GLint aspectI;

	GLint maxPosIndexI;
	GLint currentPosIndexI;
	GLint maxInterpIndexI;
	GLint currentInterpIndexI;

	GLuint vao;
	GLuint vbo;
}



void Cursor::Init()
{
	static const char *vertexCode = R"V0G0N(
// Cursor vertex shader

precision mediump float;

uniform vec2 position;
uniform float aspect;

in vec2 vert;
out vec2 fragTexCoord;

void main() {
	gl_Position = vec4(((vec2(vert.x, vert.y * aspect) / 20.) + position), 0, 1);

	fragTexCoord = vec2(vert.x, -vert.y);
}

)V0G0N";

	static const char *fragmentCode = R"V0G0N(
// Cursor fragment shader

// From https://gist.github.com/mairod/a75e7b44f68110e1576d77419d608786
vec3 hueShift( vec3 color, float hueAdjust ){

    const vec3  kRGBToYPrime = vec3 (0.299, 0.587, 0.114);
    const vec3  kRGBToI      = vec3 (0.596, -0.275, -0.321);
    const vec3  kRGBToQ      = vec3 (0.212, -0.523, 0.311);

    const vec3  kYIQToR     = vec3 (1.0, 0.956, 0.621);
    const vec3  kYIQToG     = vec3 (1.0, -0.272, -0.647);
    const vec3  kYIQToB     = vec3 (1.0, -1.107, 1.704);

    float   YPrime  = dot (color, kRGBToYPrime);
    float   I       = dot (color, kRGBToI);
    float   Q       = dot (color, kRGBToQ);
    float   hue     = atan (Q, I);
    float   chroma  = sqrt (I * I + Q * Q);

    hue += hueAdjust;

    Q = chroma * sin (hue);
    I = chroma * cos (hue);

    vec3    yIQ   = vec3 (YPrime, I, Q);

    return vec3( dot (yIQ, kYIQToR), dot (yIQ, kYIQToG), dot (yIQ, kYIQToB) );

}

uniform sampler2DArray tex;
uniform int maxPosIndex;
uniform int currentPosIndex;
uniform int maxInterpIndex;
uniform int currentInterpIndex;

in vec2 fragTexCoord;
out vec4 color;

void main() {
	float currentPos = float(currentPosIndex) / float(maxPosIndex);
	float currentInterp = float(currentInterpIndex) / maxInterpIndex;
	float alphaMod = (currentPos + (currentInterp / maxPosIndex)) / (maxInterpIndex / maxPosIndex);
	color = texture(tex, vec3(fragTexCoord, 0)) * alphaMod;
	color.rgb = hueShift(vec3(color.r, 0.2 * color.a, 0.2 * color.a), alphaMod * 150);
}
)V0G0N";

	shader = Shader(vertexCode, fragmentCode);

	positionI = shader.Uniform("position");
	aspectI = shader.Uniform("aspect");

	maxPosIndexI = shader.Uniform("maxPosIndex");
	maxInterpIndexI = shader.Uniform("maxInterpIndex");
	currentInterpIndexI = shader.Uniform("currentInterpIndex");
	currentPosIndexI = shader.Uniform("currentPosIndex");

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLfloat vertexData[] = {
		0.f, 0.f,
		0.f, -1.f,
		1.f, 0.f,
		1.f, -1.f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	cursorTex = SpriteSet::Get("ui/Cursor");
}



void Cursor::UpdatePosition(Point mousePos)
{
	for(int i = POSITION_SIZE - 1; i > 0; --i)
	{
		lastPos[i] = lastPos[i - 1];
	}
	lastPos[0] = Point(mousePos.X(), -mousePos.Y());
}



void Cursor::Draw()
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);

	glBindTexture(GL_TEXTURE_2D_ARRAY, cursorTex->Texture());

	glUniform1f(aspectI, static_cast<float>(Screen::Width()) / Screen::Height());
	glUniform1i(maxInterpIndexI, INTERPOLATION_NUM);
	glUniform1i(maxPosIndexI, POSITION_SIZE);

	for(int i = POSITION_SIZE - 2; i != -1; --i)
	{
		for(int x = INTERPOLATION_NUM - 1; x != -1; --x)
		{
			float X = lastPos[i].X() + (lastPos[i + 1].X() - lastPos[i].X()) * (x / float(INTERPOLATION_NUM));
			float Y = lastPos[i].Y() + (lastPos[i + 1].Y() - lastPos[i].Y()) * (x / float(INTERPOLATION_NUM));
			glUniform2f(positionI, X, Y);
			glUniform1i(currentInterpIndexI, INTERPOLATION_NUM - x);
			glUniform1i(currentPosIndexI, POSITION_SIZE - i);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
	}

	glBindVertexArray(0);
	glUseProgram(0);
}
