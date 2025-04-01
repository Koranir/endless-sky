#pragma once

struct SDL_Window;
struct SDL_GL_Context;

class Debug {
public:
	static void Init(SDL_Window *window, SDL_GL_Context *glContext);
	static void Destroy();
};
