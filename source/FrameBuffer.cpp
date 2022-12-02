/* FrameBuffer.cpp
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

#include "FrameBuffer.h"

#include "Shader.h"

#include <sys/stat.h>

#include "Logger.h"

#include <cctype>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdio>

using namespace std;



bool FrameBufferObject::CreateFrameBuffer(FrameBuffer::bufferType type, int width, int height)
{
	width = width;
	height = height;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	switch (type)
	{
	case FrameBuffer::bufferType::frame:
		glGenTextures(1, &bufferTexture);
		glBindTexture(GL_TEXTURE_2D, bufferTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferTexture, 0);
		break;
	default:
		Logger::LogError("No Type supplied");
		return false;
	}
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return true;
	}
	throw runtime_error("Could not create framebuffer");
	return false;
}



bool FrameBufferObject::UpdateBuffer(int width, int height) const
{
	glBindTexture(GL_TEXTURE_2D, bufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
	return true;
}



bool FrameBufferObject::RemoveFrameBuffer()
{
	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(1, &bufferTexture);
	return true;
}



bool FrameBufferObject::BindAndClear() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	FrameBuffer::Clear();
	return true;
}



void FrameBuffer::Clear()
{
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);
}



void FrameBufferObject::BlitBufferTo(FrameBufferObject to)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, to.Buffer());
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glBlitFramebuffer(0, 0, width, height, 0, 0, to.Width(), to.Height(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
}



void FrameBuffer::BlitBufferToScreen(FrameBufferObject from, int width, int height)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, from.Buffer());
	glBlitFramebuffer(0, 0, from.Width(), from.Height(), 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}



unsigned int FrameBufferObject::Buffer()
{
	return fbo;
}



int FrameBufferObject::Width()
{
	return width;
}



int FrameBufferObject::Height()
{
	return height;
}



GLuint FrameBufferObject::BufferTexture() const
{
	return bufferTexture;
}



void FrameBuffer::ResetFrameBuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
