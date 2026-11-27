#include <iostream>
#include <string>

#include "Game.h"
#include "Scenes.h"

int main(int argc, char* argv[]) {
    if (argc >= 3 && argv[1] == std::string("-path")) {
        gResourcePath = argv[2];
    }
    std::cout << "Usage: " << argv[0] << " [-path resource_path]\n";
    std::cout << "Using resource path: " << gResourcePath << "\n";

    Engine engine(kWindowWidth, kWindowHeight, "Lightkeeper");

    // Sounds are shared by every scene, so they are loaded once here.
    auto& sounds = engine.GetSoundManager();
    sounds.RegisterSound(gResourcePath + "sfx_jump.ogg", "jump");
    sounds.RegisterSound(gResourcePath + "sfx_gem.ogg", "star");
    sounds.RegisterSound(gResourcePath + "footstep_snow_001.ogg", "wallslide");
    sounds.RegisterSound(gResourcePath + "laserLarge_000.ogg", "dash");
    sounds.RegisterSound(gResourcePath + "error_008.ogg", "hurt");
    sounds.RegisterSound(gResourcePath + "footstep_grass_003.ogg", "land");

    engine.AddScene(MakeMenuScene(engine));
    engine.Run();
    return 0;
}
