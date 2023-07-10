/* FrameBuffer.cpp
Copyright (c) 2023 by RisingLeaf

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "FrameBuffer.h"

#include "GameWindow.h"
#include "SpriteShader.h"



FrameBuffer::FrameBuffer(int width, int height)
	: width(width), height(height)
{
	glGenFramebuffers(1, &buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer);
	const GLenum buffers[] { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, buffers);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, // target, mipmap level, internal format,
		width, height, 1,          // width, height, depth,
		0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);   // border, input format, data type, data.
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0, 0);
}



FrameBuffer::~FrameBuffer()
{
	glDeleteFramebuffers(1, &buffer);
	glDeleteTextures(1, &texture);
}



void FrameBuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, buffer);
	glViewport(0, 0, width, height);
}



void FrameBuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GameWindow::AdjustViewport();
}



GLuint FrameBuffer::Texture()
{
	return texture;
}



void FrameBuffer::Blit()
{
	SpriteShader::Item item;
	item.texture = texture;
	item.frame = 0;
	item.frameCount = 1;
	item.position[0] = 0.f;
	item.position[1] = 0.f;
	item.transform[0] = width;
	item.transform[3] = -height;
	item.swizzle = 0;

	SpriteShader::Bind();
	SpriteShader::Add(item);
	SpriteShader::Unbind();
}
