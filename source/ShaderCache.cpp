#define _CRT_SECURE_NO_WARNINGS true

#include "ShaderCache.h"

#include "Files.h"
#include "Logger.h"

#include <cstdio>
#include <vector>

using namespace std;

namespace {
}

bool ShaderCache::ReadCache(const string &name, GLuint program)
{
	if(!CanCache())
		return false;

	string cachePath = Files::Cache() + name + ".bin";

	FILE *cacheFile;

	cacheFile = fopen(cachePath.c_str(), "rb");

	//cacheFile.open(cachePath, ifstream::binary);

	// Return false if the file failed to open
	if(!cacheFile) {
		Logger::LogError("File " + cachePath + " failed to open.");
		return false;
	}

	// Find the size of the file
	fseek(cacheFile, 0, SEEK_END);
	size_t cacheFileSize = ftell(cacheFile);
	fseek(cacheFile, 0, SEEK_SET);

	vector<char> data;
	data.reserve(cacheFileSize * sizeof(char));

	Logger::LogError("Reading cache from file " + cachePath + " with size of " + to_string(cacheFileSize * sizeof(char)));
	size_t result = fread(data.data(), sizeof(char), cacheFileSize, cacheFile);

	fclose(cacheFile);

	if(!(cacheFileSize == result))
	{
		Logger::LogError("Error reading file " + cachePath + " that should have had size of " + to_string(cacheFileSize) + " but had size read of " + to_string(result));
		return false;
	}

	// Glenum is 4 bytes
	GLenum format = *(reinterpret_cast<GLenum *>(data.data()));
	Logger::LogError(to_string(static_cast<int>(format)));

	glProgramBinary(program, format, data.data() + sizeof(GLenum), cacheFileSize - sizeof(GLenum));

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);

	return true;
}

bool ShaderCache::WriteCache(const string &name, GLuint program)
{
	if(!CanCache())
		return false;

	GLint programSize;
	glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &programSize);

	GLsizei bytesWritten;

	GLenum programBinaryFormat;

	vector<char> data;
	data.reserve(programSize + sizeof(GLenum));

	
	glGetProgramBinary(program, programSize, &bytesWritten, &programBinaryFormat, data.data() + sizeof(GLenum));

	Logger::LogError(to_string(static_cast<int>(programBinaryFormat)));

	if(!(bytesWritten == programSize))
		return false;

	*(reinterpret_cast<GLenum *>(data.data())) = programBinaryFormat;

	string cachePath = Files::Cache() + name + ".bin";

	FILE *cacheFile;
	cacheFile = fopen(cachePath.c_str(), "wb");
	fseek(cacheFile, 0, SEEK_SET);

	Logger::LogError("Writing cache to file " + cachePath + " with size of " + to_string(programSize + sizeof(char)));
	fwrite(data.data(), sizeof(char), programSize + sizeof(GLenum), cacheFile);


	fclose(cacheFile);

	return true;
}

GLint ShaderCache::CacheFormats()
{
	GLint formats;
	glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
	return formats;
}

bool ShaderCache::CanCache()
{
	if((CacheFormats() < 1))
	{
		Logger::LogError("Driver does not support binary formats");
		return false;
	}
	return true;
}
