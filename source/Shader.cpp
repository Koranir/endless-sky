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

#include "File.h"
#include "Files.h"
#include "Shader.h"
#include "ShaderCache.h"

#include <sys/stat.h>

#include "Logger.h"

#include <cctype>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdio>

using namespace std;



namespace {
	bool HasCached(const string name)
	{
		bool hasCached = true;
		string path = Files::Shaders() + name + "/" + name + ".cache";
		Logger::LogError("Cheking " + path + " for binary.");
		struct stat buf;
		hasCached &= !stat(path.c_str(), &buf);
		Logger::LogError(to_string(hasCached) + " at Exists");
		hasCached &= Files::Timestamp(path) > max(Files::Timestamp(Shader::ShaderPath(name, true)),
												Files::Timestamp(Shader::ShaderPath(name, false)));
		Logger::LogError(to_string(hasCached) + " at Time " + to_string(Files::Timestamp(path)));

		return hasCached;
	}



	bool Readcache(string path, GLuint program)
	{
		FILE *bin;
		bin = fopen(path.c_str(), "rb");
		fseek(bin, 0, SEEK_END);
		size_t size = ftell(bin);
		fseek(bin, 0, SEEK_SET);

		// Read it into a buffer.
		char *buffer = new char[size];
		size_t result = fread(buffer, 1, size, bin);
		if(result != size)
		{
			throw runtime_error("Shader Cache read failed at " + path + ", Size: " + to_string(size) + "\n Sizeof: " + to_string(result));
			return false;
		}
		fclose(bin);

		// Grab the format from the front of the buffer;
		GLenum format = *(reinterpret_cast<GLenum*>(buffer));
		Logger::LogError(to_string(format));

		// Calculate offset to start of the shader binary.
		void* binaryStart = buffer + sizeof(GLenum);
		size_t binaryLength = size - sizeof(GLenum);

		// Upload the binary.
		glProgramBinary(program, format, binaryStart, binaryLength);

		// Clean up.
		delete[] buffer;
		return true;
	}



	bool Cache(string path, char* buffer, size_t length)
	{
		FILE *bin;
		bin = fopen(path.c_str(), "w+b");
		fwrite(buffer, sizeof(char), length, bin);
		fclose(bin);
		return true;
	}



	bool CanCache()
	{
		// Shader caching is only available on GPUs which support at least one binary format.
		GLint formats;
		glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
		return formats > 0;
	}
}



Shader::Shader(const char *name)
{
	const string strName = name;
	MakeShader(strName, false);
}



Shader::Shader(const string name, bool useShaderSwizzle)
{
	MakeShader(name, useShaderSwizzle);
}



void Shader::MakeShader(const string name, bool useShaderSwizzle)
{
	const string vertexPath = Shader::ShaderPath(name, false, useShaderSwizzle);
	const string fragmentPath = Shader::ShaderPath(name, true, useShaderSwizzle);
	const string vertexCode = Files::Read(vertexPath);
	const string fragmentCode = Files::Read(fragmentPath);
	if(vertexCode.empty())
		throw runtime_error("Vertex Shader cannot be found at " + vertexPath);
	if(fragmentCode.empty())
		throw runtime_error("Fragment Shader cannot be found at " + fragmentPath);

	Logger::LogError("sprite bin exists?: " + to_string(Files::Exists(Files::Shaders() + "sprite/sprite.cache")));

	program = glCreateProgram();

	if(HasCached(name))
	{
		string path = Files::Shaders() + name + "/" + name + ".cache";
		// Get the binary file from the cache.
		Readcache(path, program);

		Logger::LogError("Shader binary found: " + name);
	}
	else
	{
		GLuint vertexShader = Compile(vertexCode.c_str(), GL_VERTEX_SHADER);
		GLuint fragmentShader = Compile(fragmentCode.c_str(), GL_FRAGMENT_SHADER);
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);

		glLinkProgram(program);

		glDetachShader(program, vertexShader);
		glDetachShader(program, fragmentShader);
		Logger::LogError("Shader uncached: " + name);
	}
	if(!program)
		throw runtime_error("Creating OpenGL shader program failed.");



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
	else if(!HasCached(name) && CanCache())
	{
		GLint binaryLength = 0;
		glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &binaryLength);

		size_t bufferSize = sizeof(GLenum) + binaryLength;
		char* binary = new char[sizeof(GLenum) + binaryLength];

		GLenum binaryFormat;
		glGetProgramBinary(program, binaryLength, nullptr, &binaryFormat, binary + sizeof(GLenum));

		*(reinterpret_cast<GLint*>(binary)) = binaryFormat;

		Cache(Files::Shaders() + name + "/" + name + ".cache", binary, bufferSize);
		Logger::LogError("Tried to cache " + name + " shader.");

		delete[] binary;
	}
	else if(HasCached(name))
		Logger::LogError("Has cached " + name + ".");
	else
		Logger::LogError("Caching not supoorted. Failed to cache " + name + ".");
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



string Shader::ShaderPath(string shader, bool isFragment, bool useShaderSwizzle)
{
	string path = Files::Shaders();
	path += shader;
	path += useShaderSwizzle ? "/swizzle/" : "/";
	path += isFragment ? "FragmentCode.txt" : "VertexCode.txt";
	return path;
}
