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



bool FrameBuffer::CreateFrameBuffer(bufferType type, GLuint *fbo, int width, int height)
{
	glGenFramebuffers(1, fbo);
	buffers.emplace_back(fbo);

	glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
	switch (type)
	{
	case bufferType::frame:
		GLuint bufferTexture;
		glGenTextures(1, &bufferTexture);
		glBindTexture(GL_TEXTURE_2D, bufferTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
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
		return true;
	}
	throw runtime_error("Could not create framebuffer");
	return false;
}



bool FrameBuffer::RemoveFrameBuffer(GLuint *fbo)
{
	glDeleteFramebuffers(1, fbo);
	for(auto buffer : buffers)
		glDeleteFramebuffers(1, buffer);
	return true;
}



bool FrameBuffer::SetFrameBuffer(GLuint fbo)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
		return true;
	throw runtime_error("Could not create framebuffer");
	return false;
}
