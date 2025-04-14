// Dear ImGui uses old-style casts in its headers for some inline functions,
// disable warnings for those.
// INFO: If your compiler gives a warning for this, add an exception here.
#pragma GCC diagnostic ignored "-Wold-style-cast"

#include "Debug.h"

#include "text/Truncate.h"
#include "Rectangle.h"
#include "Color.h"
#include <limits>

#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl2.h>
#include <misc/cpp/imgui_stdlib.h>
#include <SDL.h>



using namespace std;

namespace {
	static string DEBUG_LOG;
}


void Debug::Init(void *window, void *sdlGlContext)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	ImGui_ImplSDL2_InitForOpenGL(static_cast<SDL_Window *>(window), sdlGlContext);
	ImGui_ImplOpenGL3_Init();
}



void Debug::Destroy()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}



bool Debug::HandleEvent(void *sdlEvent)
{
	SDL_Event *event = static_cast<SDL_Event *>(sdlEvent);

	ImGui_ImplSDL2_ProcessEvent(event);

	ImGuiIO &io = ImGui::GetIO();
	return (io.WantCaptureMouse && event->type >= SDL_MOUSEMOTION && event->type <= SDL_MOUSEWHEEL)
		||	(io.WantCaptureKeyboard && event->type >= SDL_KEYDOWN && event->type <= SDL_TEXTEDITING_EXT);
}



void Debug::StartFrame()
{
	ImGui_ImplSDL2_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
	ImGui::Begin("System Log", nullptr);
	if(ImGui::Button("clear"))
		DEBUG_LOG.clear();
	ImGui::TextWrapped("%s", DEBUG_LOG.c_str());
	ImGui::End();

	ImGui::ShowDemoWindow();
}



void Debug::EndFrame(bool skipRender)
{
	ImGui::Render();
	if(!skipRender)
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}



void Debug::Log(const std::string &s)
{
	DEBUG_LOG.append("[LOGGER] ");
	DEBUG_LOG.append(s);
	DEBUG_LOG.push_back('\n');
}



void Debug::SectionBegin(const string &s)
{
	ImGui::BeginGroup();
	ImGui::Text("%s", s.c_str());
}



void Debug::SectionEnd()
{
	ImGui::EndGroup();
}



bool Debug::CollapsableSectionBegin(const string &s, void *id)
{
	ImGui::PushID(id);
	return ImGui::CollapsingHeader(s.c_str());
}



void Debug::CollapsableSectionEnd()
{
	ImGui::PopID();
}



void Debug::IndentBegin(void *id)
{
	ImGui::TreePush(id);
}



void Debug::IndentEnd()
{
	ImGui::TreePop();
}



template<>
void Debug::Value(const string &s, double *value)
{
	ImGui::PushID(value);
	ImGui::DragScalar(s.c_str(), ImGuiDataType_Double, value);
	ImGui::PopID();
}


template<>
void Debug::Value(const string &s, int *value)
{
	ImGui::PushID(value);
	ImGui::DragInt(s.c_str(), value);
	ImGui::PopID();
}

template<>
void Debug::Value(const std::string &s, Point *value)
{
	ImGui::PushID(value);
	ImGui::Text("%s", s.c_str());
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::CalcItemWidth() / 2.5);
	Value("x", &value->X());
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::CalcItemWidth() / 2.5);
	Value("y", &value->Y());
	ImGui::PopID();
}

template<>
void Debug::Value(const std::string &s, Rectangle *value)
{
	ImGui::PushID(value);
	SectionBegin(s);
	Debug::Value("center", &value->center);
	Debug::Value("dimensions", &value->dimensions);
	SectionEnd();
	ImGui::PopID();
}

template<>
void Debug::Value(const std::string &s, string *value)
{
	ImGui::PushID(value);
	ImGui::InputText(s.c_str(), value);
	ImGui::PopID();
}

template<>
void Debug::Value(const std::string &s, const string *value)
{
	ImGui::PushID(value);
	ImGui::LabelText(s.c_str(), "%s", value->c_str());
	ImGui::PopID();
}

template<>
void Debug::Value(const string &s, bool *value)
{
	ImGui::PushID(value);
	ImGui::Checkbox(s.c_str(), value);
	ImGui::PopID();
}

template<>
void Debug::Value(const string &s, char *value)
{
	ImGui::PushID(value);
	ImGui::InputText(s.c_str(), value, 1);
	ImGui::PopID();
}

template<>
void Debug::Value(const string &s, Truncate *value)
{
	ImGui::PushID(value);
	ImGui::Combo(s.c_str(), reinterpret_cast<int *>(value), "None\0Front\0Middle\0Back");
	ImGui::PopID();
}

template<>
void Debug::Value(const string &s, Color *value)
{
	ImGui::PushID(value);
	ImGui::ColorEdit4(s.c_str(), const_cast<float *>(value->Get()));
	ImGui::PopID();
}

template<>
void Debug::Value(const string &s, const Color *value)
{
	ImGui::PushID(value);
	ImGui::ColorEdit4(s.c_str(), const_cast<float *>(value->Get()), ImGuiColorEditFlags_NoInputs);
	ImGui::PopID();
}

template<>
void Debug::Value(const string &s, float *value)
{
	ImGui::PushID(value);
	ImGui::DragFloat(s.c_str(), value);
	ImGui::PopID();
}
