#include "PostProcess.h"

#include "Logger.h"

using namespace std;



PostProcessShader::PostProcessShader(string name) : name(name)
{
	LoadShader(name);
}



string PostProcessShader::GetName()
{
	return name;
}



void PostProcessShader::LoadShader(string name)
{
	Logger::LogError("Loading Shader " + name);
	shader = Shader(name, false, false);
	bufferImage = shader.Uniform("bufferTexture");

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLfloat vertexData[] = {
		-1.f, -1.f,
		 3.f, -1.f,
		-1.f,  3.f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	Logger::LogError("Loaded Shader");
	loaded = true;
}



void PostProcessShader::Draw(GLuint texture)
{
	if(loaded)
	{
		Logger::LogError("Drawing Shader");
		if(!shader.Object())
			throw runtime_error("PostProcess called before Init().");

		glUseProgram(shader.Object());
		glBindVertexArray(vao);

		glBindTexture(GL_TEXTURE_2D, texture);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);

		glBindVertexArray(0);
		glUseProgram(0);
		Logger::LogError("Drew Shader");
	}
}



void PostProcessShader::Delete()
{
	glDeleteShader(shader.Object());
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}



GLuint PostProcessShader::GetShader()
{
	return shader.Object();
}



bool PostProcessList::HasDuplicate(string name) const
{
	return postProcessList.find(name) != postProcessList.end();
}



void PostProcessList::AddShader(string name)
{
	Logger::LogError("Adding Shader");
	if(!HasDuplicate(name))
		postProcessList.insert(make_pair(name, PostProcessShader(name)));
	else
		Logger::LogError("Shader already loaded");
	Logger::LogError("Added Shader");
}



void PostProcessList::RemoveShader(string name)
{
	Logger::LogError("Removing " + name);
	postProcessList[name].Delete();
	postProcessList.erase(postProcessList.find(name));
}



void PostProcessList::DrawList(GLuint texture) const
{
	Logger::LogError("Drawing list");
	for(pair<string, PostProcessShader> post : postProcessList)
	{
		post.second.Draw(texture);
	}
}



bool PostProcessList::IsEmpty() const
{
	return postProcessList.empty();
}



void PostProcessList::Clear()
{
	Logger::LogError("Cleared List");
	for(pair<string, PostProcessShader> post : postProcessList)
	{
		post.second.Delete();
	}
	postProcessList.clear();
}
