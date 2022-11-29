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
//	if(SDL_GL_MakeCurrent(Screen::GetSDLWindow(), *Screen::GetSDLContextPtr()))
//	{
//		throw runtime_error("Unable to set the current OpenGL context!");
//	}
//	if(SDL_GL_GetCurrentContext() == NULL)
//	{
//		Logger::LogError("SDL_GLGCC is NULL");
//		Logger::LogError(SDL_GetError());
//	}
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
	Logger::LogError("Loaded Shader");
	loaded = true;
}



void PostProcessShader::Draw(GLuint texture)
{
	if(loaded)
	{
		Logger::LogError("Drawing Shader");
		if(!shader.Object())
			throw runtime_error("PostProcess called before Init().");

		glUseProgram(shader.Object());
		glBindVertexArray(vao);

		glBindTexture(GL_TEXTURE_2D, texture);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);

		glBindVertexArray(0);
		glUseProgram(0);
		Logger::LogError("Drew Shader");
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
	Logger::LogError("Checking ofr duplicates");
	return postProcessList.find(name) != postProcessList.end();
}



void PostProcessList::AddShader(string name)
{
	Logger::LogError("Adding Shader");
	if(!HasDuplicate(name))
		postProcessList.insert(pair<string, PostProcessShader>(name, PostProcessShader(name)));
	else
		Logger::LogError("Shader already loaded");
	Logger::LogError("Added Shader");
}



void PostProcessList::RemoveShader(string name)
{
	Logger::LogError("Removing " + name);
	postProcessList.at(name).Delete();
	postProcessList.erase(postProcessList.find(name));
}



void PostProcessList::DrawList(vector<string> shaders, GLuint texture)
{
	Logger::LogError("Drawing list");
	for(const auto &it : postProcessList)
	{
		Logger::LogError(it.first);
	}
	for(const string &name : shaders)
	{
		Logger::LogError(name);
		postProcessList.at(name).Draw(texture);
	}
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
