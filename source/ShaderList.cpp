#include "PostProcess.h"
#include "Screen.h"
#include "MainPanel.h"
#include "GameWindow.h"
#include "ShaderList.h"

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

bool ShaderList::HasDuplicate(string name)
{
	return shaderList.find(name) != shaderList.end();
}



void ShaderList::AddShader(string name)
{
	if(!HasDuplicate(name))
		shaderList.insert(pair<string, PostProcessShader>(name, PostProcessShader(name)));
	else
		Logger::LogError("Shader already loaded");
}



void ShaderList::RemoveShader(string name)
{
	Logger::LogError("Removing " + name);
	shaderList.at(name).Delete();
	shaderList.erase(shaderList.find(name));
}



void ShaderList::DrawList(const vector<pair<string, vector<pair<string, vector<double>>>>> &shaders, GLuint texture)
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

		shaderList.at(data.first).Draw(first ? texture : flip ? fbo2.BufferTexture() : fbo.BufferTexture(), data.second);
		first = false;
		flip = !flip;
	}
	fbo.RemoveFrameBuffer();
	fbo2.RemoveFrameBuffer();
}



bool ShaderList::IsEmpty()
{
	return shaderList.empty();
}



void ShaderList::Clear()
{
	Logger::LogError("Cleared List");
	for(pair<const string, PostProcessShader> post : shaderList)
	{
		post.second.Delete();
		Logger::LogError("Deleted " + post.first);
	}
	shaderList.clear();
}
