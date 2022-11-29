#ifndef POST_PROCESS_H_
#define POST_PROCESS_H_

#include "Shader.h"

#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "opengl.h"
#include "SDL2/SDL.h"

class PostProcessShader
{
public:
	PostProcessShader(std::string name);
	std::string GetName();
	void Delete();
	GLuint GetShader();
	void LoadShader(std::string name);
	void Draw(GLuint texture);
private:
	Shader shader;
	GLint bufferImage;

	GLuint vao;
	GLuint vbo;
	bool loaded = false;
	std::string name;
};

class PostProcessList
{
public:
	static bool HasDuplicate(std::string name);
	static void AddShader(std::string name);
	static void RemoveShader(std::string name);
	static void DrawList(std::vector<std::string> shaders, GLuint texture);
	static bool IsEmpty();
	static void Clear();
};

#endif
