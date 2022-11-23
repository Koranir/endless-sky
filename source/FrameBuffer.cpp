/* Shader.cpp
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
#include "File.h"
#include "Files.h"
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
		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT);
		break;
	default:
		Logger::LogError("No Type supplied");
		return false;
	}
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
	{
		return true;
	}
	throw runtime_error("Could not create framebuffer");
	return false;
}



bool FrameBufferObject::RemoveFrameBuffer()
{
	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(1, &bufferTexture);
	Logger::LogError("Tried to remove" + to_string(fbo));
	return true;
}



unsigned int FrameBufferObject::Buffer()
{
	return fbo;
}



GLuint FrameBufferObject::BufferTexture()
{
	return bufferTexture;
}



void FrameBuffer::ResetFrameBuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
	{
		Logger::LogError("Frambuffer exists");
	}
}
