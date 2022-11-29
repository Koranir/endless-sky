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
	bool HasDuplicate(std::string name) const;
	void AddShader(std::string name);
	void RemoveShader(std::string name);
	void DrawList(GLuint texture) const;
	bool IsEmpty() const;
	void Clear();
private:
	std::unordered_map<std::string, PostProcessShader> postProcessList;
};

#endif
