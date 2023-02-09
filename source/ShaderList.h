#ifndef SHADER_LIST_H
#define SHADER_LIST_H

#include "PostProcess.h"

#include <string>
#include <vector>
#include <unordered_map>

class ShaderList
{
public:
	static bool HasDuplicate(std::string name);
	static void AddShader(std::string name);
	static void RemoveShader(std::string name);
	static void DrawList(const std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::vector<double>>>>> &shaders
						, GLuint texture);
	static bool IsEmpty();
	static void Clear();
private:
	static std::unordered_map<std::string, PostProcessShader> shaderList;
};

#endif
