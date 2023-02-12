#include "opengl.h"

#include <string>

#ifndef SHADER_CACHE_H
#define SHADER_CACHE_H

class ShaderCache {
public:
	static bool WriteCache(const std::string &name, GLuint program);

	static GLint CacheFormats();

	static bool CanCache();

	static bool ReadCache(const std::string &name, GLuint program);
};

#endif
