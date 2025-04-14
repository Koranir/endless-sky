#pragma once

#include <string>



class Debug {
public:
	static void Init(void *sdlWindow, void *sdlGlContext);
	static void Destroy();

	static void StartFrame();
	static void EndFrame(bool skipRender);

	static bool HandleEvent(void *sdlEvent);
	static void Log(const std::string &s);

	static void SectionBegin(const std::string &s);
	static bool CollapsableSectionBegin(const std::string &s, void *id);
	static void IndentBegin(void *id);
	template<typename ...T>
	static void Value(const std::string &s, T *...value);
	static void SectionEnd();
	static void CollapsableSectionEnd();
	static void IndentEnd();
};
