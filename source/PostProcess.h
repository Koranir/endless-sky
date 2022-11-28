#ifndef POST_PROCESS_H_
#define POST_PROCESS_H_

#include "Shader.h"

#include <string>
#include <unordered_map>
#include <vector>

class PostProcessShader
{
public:
	PostProcessShader(std::string name);
	std::string GetName();
	void LoadShader(std::string name);
	void Draw(GLuint texture);
private:
	std::string name;
};

class PostProcessList
{
public:
	static bool HasDuplicate(std::string name);
	static void AddShader(std::string name);
	static void RemoveShader(std::string name);
	static std::vector<PostProcessShader> GetShaders();
private:
	static std::unordered_map<std::string, PostProcessShader> postProcessList;
};

#endif
