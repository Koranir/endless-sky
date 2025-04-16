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

#include "Shader.h"

#include "../Logger.h"
#include "../GameWindow.h"
#include "../Files.h"

#include <cctype>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace {
	const int CACHE_VERSION = 1;

	extern "C" {
		struct ShaderCacheInfo {
			int version;
			GLenum format;
		};
	}

	pair<filesystem::path, optional<string>> GetCachedShader(const char *vertex, const char *fragment) {
		auto hasher = std::hash<string_view>();
		auto hash = hasher(vertex);
		hash ^= hasher(fragment) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= GameWindow::OpenGLCacheHash() + 0x9e3779b9 + (hash << 6) + (hash >> 2);

		filesystem::path configDir = Files::Config();
		auto path = configDir.append("shadercache").append(to_string(hash));

		string cached = Files::Read(path);
		return make_pair(path, cached.empty() ? nullopt : optional(cached));
	}
}



Shader::Shader(const char *vertex, const char *fragment)
{
	program = glCreateProgram();
	if(!program)
		throw runtime_error("Creating OpenGL shader program failed.");

	auto [cachePath, shaderCache] = GetCachedShader(vertex, fragment);

	bool manualCompile = true;

	if(shaderCache.has_value())
	{
		Logger::LogError("Loading cached shader at '" + cachePath.string() + "'");
		auto info = reinterpret_cast<ShaderCacheInfo *>(shaderCache.value().data());
		if(info->version != CACHE_VERSION)
		{
				Logger::LogError("Cached shader had invalid version.");
			Files::Delete(cachePath);
			shaderCache = nullopt;
		}
		else
		{
			glProgramBinary(program, info->format, shaderCache->data() + sizeof(ShaderCacheInfo), shaderCache->length() - sizeof(ShaderCacheInfo));

			GLint status;
			glGetProgramiv(program, GL_LINK_STATUS, &status);
			if(status == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
				vector<GLchar> infoLog(maxLength);
				glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
				string error(infoLog.data());
				Logger::LogError("Failed to load cached shader.");
				Logger::LogError(error);
			}
			else
				manualCompile = false;
		}
	}

	if(manualCompile)
	{
		GLuint vertexShader = Compile(vertex, GL_VERTEX_SHADER);
		GLuint fragmentShader = Compile(fragment, GL_FRAGMENT_SHADER);

		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);

		glLinkProgram(program);

		glDetachShader(program, vertexShader);
		glDetachShader(program, fragmentShader);

		GLint status;
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if(status == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
			vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
			string error(infoLog.data());
			Logger::LogError(error);

			throw runtime_error("Linking OpenGL shader program failed.");
		}
	}

	if(GameWindow::OpenGLCacheHash() && !shaderCache.has_value())
	{
		int len;
		glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &len);

		string binary;
		binary.resize(len + sizeof(ShaderCacheInfo));
		ShaderCacheInfo *info = reinterpret_cast<ShaderCacheInfo *>(binary.data());
		info->version = CACHE_VERSION;
		glGetProgramBinary(program, len, nullptr, &info->format, binary.data() + sizeof(ShaderCacheInfo));

		Files::Write(cachePath, binary);
		Logger::LogError("Cached shader.");
	}
}



GLuint Shader::Object() const noexcept
{
	return program;
}



GLint Shader::Attrib(const char *name) const
{
	GLint attrib = glGetAttribLocation(program, name);
	if(attrib == -1)
		throw runtime_error("Attribute \"" + string(name) + "\" not found.");

	return attrib;
}



GLint Shader::Uniform(const char *name) const
{
	GLint uniform = glGetUniformLocation(program, name);
	if(uniform == -1)
		throw runtime_error("Uniform \"" + string(name) + "\" not found.");

	return uniform;
}



GLuint Shader::Compile(const char *str, GLenum type)
{
	GLuint object = glCreateShader(type);
	if(!object)
		throw runtime_error("Shader creation failed.");

	static string version;
	if(version.empty())
	{
		version = "#version ";
		string glsl = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
		bool found = false;
		for(char c : glsl)
		{
			if(!found && !isdigit(c))
			{
				continue;
			}
			if(isspace(c))
				break;
			if(isdigit(c))
			{
				found = true;
				version += c;
			}
		}
		if(glsl.find("GLSL ES") != std::string::npos)
		{
			version += " es";
		}
		version += '\n';
	}
	size_t length = strlen(str);
	vector<GLchar> text(version.length() + length + 1);
	memcpy(&text.front(), version.data(), version.length());
	memcpy(&text.front() + version.length(), str, length);
	text[version.length() + length] = '\0';

	const GLchar *cText = &text.front();
	glShaderSource(object, 1, &cText, nullptr);
	glCompileShader(object);

	GLint status;
	glGetShaderiv(object, GL_COMPILE_STATUS, &status);
	if(status == GL_FALSE)
	{
		string error = version;
		error += string(str, length);

		static const int SIZE = 4096;
		GLchar message[SIZE];
		GLsizei length;

		glGetShaderInfoLog(object, SIZE, &length, message);
		error += string(message, length);
		Logger::LogError(error);
		throw runtime_error("Shader compilation failed.");
	}

	return object;
}
