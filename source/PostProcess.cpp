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
	unordered_map<string, PostProcessShader> postProcessList;
}



PostProcessShader::PostProcessShader(string name)
	: name(name)
{
	LoadShader(name);
}



string PostProcessShader::GetName()
{
	return name;
}



void PostProcessShader::LoadShader(string name)
{
	Logger::LogError("Loading Shader " + name);
	shader = Shader(name, false, false);
	bufferImage = shader.Uniform("bufferTexture");

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLfloat vertexData[] = {
		-1.f, -1.f,
		 3.f, -1.f,
		-1.f,  3.f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	loaded = true;
}



void PostProcessShader::Draw(GLuint texture, vector<pair<string, vector<double>>> uniforms)
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

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);

		glBindVertexArray(0);
		glUseProgram(0);
	}
}



void PostProcessShader::Delete()
{
	glDeleteShader(shader.Object());
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}



GLuint PostProcessShader::GetShader()
{
	return shader.Object();
}



bool PostProcessList::HasDuplicate(string name)
{
	return postProcessList.find(name) != postProcessList.end();
}



void PostProcessList::AddShader(string name)
{
	if(!HasDuplicate(name))
		postProcessList.insert(pair<string, PostProcessShader>(name, PostProcessShader(name)));
	else
		Logger::LogError("Shader already loaded");
}



void PostProcessList::RemoveShader(string name)
{
	Logger::LogError("Removing " + name);
	postProcessList.at(name).Delete();
	postProcessList.erase(postProcessList.find(name));
}



void PostProcessList::DrawList(const vector<pair<string, vector<pair<string, vector<double>>>>> &shaders, GLuint texture)
{
	bool flip = false;
	bool first = true;
	int drawn = 0;
	int size = shaders.size();
	FrameBufferObject fbo;
	FrameBufferObject fbo2;
	fbo.CreateFrameBuffer(FrameBuffer::bufferType::frame, Screen::RawWidth(), Screen::RawHeight());
	fbo2.CreateFrameBuffer(FrameBuffer::bufferType::frame, Screen::RawWidth(), Screen::RawHeight());
	for(const auto &data : shaders)
	{
		drawn++;
		if(flip)
			fbo.BindAndClear();
		else
			fbo2.BindAndClear();

		if(drawn == size)
			FrameBuffer::ResetFrameBuffer();

		postProcessList.at(data.first).Draw(first ? texture : flip ? fbo2.BufferTexture() : fbo.BufferTexture(), data.second);
		first = false;
		flip = !flip;
	}
	fbo.RemoveFrameBuffer();
	fbo2.RemoveFrameBuffer();
}



bool PostProcessList::IsEmpty()
{
	return postProcessList.empty();
}



void PostProcessList::Clear()
{
	Logger::LogError("Cleared List");
	for(pair<const string, PostProcessShader> post : postProcessList)
	{
		post.second.Delete();
		Logger::LogError("Deleted " + post.first);
	}
	postProcessList.clear();
}
