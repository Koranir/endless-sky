#include "KeybindDisplay.h"

#include "Color.h"
#include "GameData.h"
#include "Panel.h"
#include "Screen.h"
#include "text/Font.h"
#include "text/FontSet.h"

#include <sstream>

#include <SDL_keyboard.h>

using namespace std;



void KeybindDisplay::DrawForPanel(Panel *panel)
{
  auto font = FontSet::Get(14);
  auto bright = *GameData::Colors().Get("bright");

  auto binds = panel->KeybindDescs();
  stringstream textss;
  bool after = false;
  for(const auto &[key, desc] : binds)
  {
    if(after)
      textss << " ";
    else
      after = true;
    textss << '[' << key << "]:" << desc;
  }
  auto text = textss.str();

  auto width = font.Width(text);

  auto pos = Screen::BottomRight() - Point(width + 20, font.Height() + 4);

  font.Draw(text, pos, bright);
}
