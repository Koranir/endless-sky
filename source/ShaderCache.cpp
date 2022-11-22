#include "ShaderCache.h"
#include "File.h"
#include "Files.h"
#include "Shader.h"

#include "Logger.h"

#include <fstream>
#include <sys/stat.h>
#include <filesystem>

using namespace std;

bool ShaderCache::HasCached(const string name, bool useShaderSwizzle)
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

	if(useShaderSwizzle)
		hasCached &= Files::Timestamp(path) > Files::Timestamp(Shader::ShaderPath(name, true, true));

	return hasCached;
}



bool ShaderCache::Cache(string path, char* buffer, size_t length)
{
	FILE *bin;
	bin = fopen(path.c_str(), "w+b");
	fwrite(buffer, sizeof(char), length, bin);
	fclose(bin);
	return true;
}



bool ShaderCache::CanCache()
{
	// Shader caching is only available on GPUs which support at least one binary format.
	GLint formats;
	glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
	return formats > 0;
}
