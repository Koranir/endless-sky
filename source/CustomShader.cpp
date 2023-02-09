#include "PostProcess.h"
#include "Screen.h"
#include "MainPanel.h"
#include "GameWindow.h"

#include <stdexcept>
#include "Logger.h"
#include "opengl.h"
#include "SDL2/SDL.h"

#include "Audio.h"
#include "Command.h"
#include "Conversation.h"
#include "ConversationPanel.h"
#include "DataFile.h"
#include "DataNode.h"
#include "Dialog.h"
#include "Files.h"
#include "text/Font.h"
#include "FrameTimer.h"
#include "GameData.h"
#include "GameLoadingPanel.h"
#include "GameWindow.h"
#include "Hardpoint.h"
#include "Logger.h"
#include "MenuPanel.h"
#include "Panel.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "PrintData.h"
#include "Screen.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "Test.h"
#include "TestContext.h"
#include "UI.h"

#include "PostProcess.h"

#include <chrono>
#include <iostream>
#include <map>
#include <thread>

#include <cassert>
#include <future>
#include <stdexcept>
#include <string>

using namespace std;

namespace{
}



CustomShader::CustomShader(string name)
	: name(name)
{
	LoadShader(name);
}



string CustomShader::GetName()
{
	return name;
}



void CustomShader::LoadShader(string name)
{
	Logger::LogError("Loading Shader " + name);
	shader = Shader(name, false, false);

	scaleI = shader.Uniform("scale");
	frameI = shader.Uniform("frame");
	frameCountI = shader.Uniform("frameCount");
	positionI = shader.Uniform("position");
	transformI = shader.Uniform("transform");
	blurI = shader.Uniform("blur");
	clipI = shader.Uniform("clip");
	alphaI = shader.Uniform("alpha");

	bufferImage = shader.Uniform("bufferTexture");

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLfloat vertexData[] = {
		-.5f, -.5f,
		-.5f,  .5f,
		 .5f, -.5f,
		 .5f,  .5f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	loaded = true;
}



void CustomShader::Draw(GLuint texture, vector<pair<string, vector<double>>> uniforms)
{
	if(loaded)
	{
		if(!shader.Object())
			throw runtime_error("PostProcess called before Init().");

		glUseProgram(shader.Object());
		glBindVertexArray(vao);

		glBindTexture(GL_TEXTURE_2D, texture);
		for(const auto &it : uniforms)
		{
			GLint target = shader.Uniform(it.first.c_str());
			switch(it.second.size())
			{
			case 1:
				glUniform1f(target, static_cast<float>(it.second[0]));
			case 2:
				glUniform2f(target, static_cast<float>(it.second[0]), static_cast<float>(it.second[1]));
			case 3:
				glUniform3f(target, static_cast<float>(it.second[0]), static_cast<float>(it.second[1]),
								static_cast<float>(it.second[2]));
			}
		}

		glUniform1f(frameI, item.frame);
		glUniform1f(frameCountI, item.frameCount);
		glUniform2fv(positionI, 1, item.position);
		glUniformMatrix2fv(transformI, 1, false, item.transform);
		// Special case: check if the blur should be applied or not.
		static const float UNBLURRED[2] = {0.f, 0.f};
		glUniform2fv(blurI, 1, withBlur ? item.blur : UNBLURRED);
		glUniform1f(clipI, item.clip);
		glUniform1f(alphaI, item.alpha);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindVertexArray(0);
		glUseProgram(0);
	}
}



void CustomShader::Delete()
{
	glDeleteShader(shader.Object());
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}



GLuint CustomShader::GetShader()
{
	return shader.Object();
}




