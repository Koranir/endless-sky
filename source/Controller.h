#include <SDL_gamecontroller.h>

#include <optional>

class Controller {
public:
    static std::optional<SDL_GameController *> Get();
    static bool TryConnect();
    static double Deadzone();
    static void SetDeadzone(double deadzone);
    static bool IsValidVal(double percentage);

};
