#include "Shader.h"

#include <string>
#include <cstring>

class ShaderCache
{
public:
	static bool HasCached(const std::string name, bool useShaderSwizzle);
	static bool Cache(std::string path, char *buffer, size_t length);
	static bool CanCache();
};
