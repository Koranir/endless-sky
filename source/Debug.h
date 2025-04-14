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
	template<typename ...T>
	static void Value(const std::string &s, T *...value);
	static void SectionEnd();
};

// template<>
// void Debug::Value(const std::string &s, double *value);

// class Point;
// template<>
// void Debug::Value(const std::string &s, Point *value);

// class Rectangle;
// template<>
// void Debug::Value(const std::string &s, Rectangle *value);

// class AnchoredPoint;
// template<>
// void Debug::Value(const std::string &s, AnchoredPoint *value);
