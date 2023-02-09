#ifndef POST_PROCESS_H_
#define POST_PROCESS_H_

#include "Shader.h"

#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "opengl.h"
#include "SDL2/SDL.h"

class CustomShader
{
public:
	CustomShader(std::string name);
	std::string GetName();
	void Delete();
	GLuint GetShader();
	void LoadShader(std::string name);
	void Draw(GLuint texture, std::vector<std::pair<std::string, std::vector<double>>> uniforms);
private:
	Shader shader;
	GLint bufferImage;

	GLint clipI;
	GLint alphaI;
	GLint blurI;
	GLint clipI;
	GLint alphaI;

	GLuint vao;
	GLuint vbo;
	bool loaded = false;
	std::string name;
//	std::vector<std::pair(std::string, std::vector<double>)> uniforms;
};

class ShaderData
{
public:
	std::string name;
	// String is the uniform name.
	// Double is the value.
	// String is the switch index for custom inputs.
	std::vector<std::tuple<std::string, std::vector<double>, std::string>> uniforms;
};

#endif
