#pragma once
#include <string>
#include <SDL2/SDL.h>
#include "VideoPlayer.h"
#include <ctime>   


class App{
    public: 
        App();
        ~App();
        bool init(); //initialises Sdl and checks for errors default 
        void run();

    private:
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;

        const int width = 800;
        const int heignt = 600;
        bool isRunning = false;

        std::string selectedFilePath;
        bool showFileDialog = false;

        void handelEvents();
        void update();
        void render();
        VideoPlayer videoPlayer;
        std::string loadedFilePath = "";
        bool startDecoding = false;

        bool isPaused = false;
};


