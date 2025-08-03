#include "App.h"
#include "FileDialog.h"


#include <iostream>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>






App::App(){
    //initialise members if needed 
}

App::~App(){
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    //clean up SDL
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

bool App::init(){
    //checks everything ok if not returns false 
    if(SDL_Init(SDL_INIT_VIDEO| SDL_INIT_AUDIO) != 0){
        std::cerr<<"SDL_Init Error:" <<SDL_GetError() <<std::endl;
        return false;
    }

    window = window = SDL_CreateWindow("VCPlayer",
                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                          width, heignt,
                          SDL_WINDOW_SHOWN | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE);

    //Sdl window flags ---> i can use to do full screan , visible(SDl_WINDOW_SHOW) etc etc...
    if(!window){
        std::cerr<<"Window Error:"<<SDL_GetError()<<std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window,-1, SDL_RENDERER_ACCELERATED  /*can use this to do software rendering or hardware acceleration etc */);
    if (!renderer) {
        std::cerr << "Renderer Error: " << SDL_GetError() << std::endl;
        return false;
    }

    //Dear Imgui context 
    IMGUI_CHECKVERSION();
    ImGui::CreateContext(); //holds all internal states of imgui
    ImGui_ImplSDL2_InitForSDLRenderer(window,renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
    ImGui::StyleColorsDark();


    isRunning = true;
    return true;
}


//Main loop 
void App::run(){
    while (isRunning)
    {
        handelEvents();
        update();
        render();
        SDL_Delay(1000/60); //60 fps  
        //need to research about this ?? can add feature to decude frames to run the video 

    }
    
}

//Handel Sdl events 
void App::handelEvents(){
    SDL_Event event;
    while (SDL_PollEvent(&event)){
        ImGui_ImplSDL2_ProcessEvent(&event); //importatnt to handel the navigation gui


        if (event.type == SDL_QUIT)
        {
            isRunning = false;
        }
        
        if(event.type == SDL_KEYDOWN){
            if(event.key.keysym.sym == SDLK_SPACE){
                videoPlayer.togglePause();
            }
            if(event.key.keysym.sym == SDLK_f){
                //toggle fullscreen
                Uint32 flags = SDL_GetWindowFlags(window);
                if (flags & SDL_WINDOW_FULLSCREEN) {
                    SDL_SetWindowFullscreen(window, 0); // Exit fullscreen
                } else {
                    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP); // Enter fullscreen
                }
            }
            if( event.key.keysym.sym == SDLK_o){
                if(SDL_GetModState() & KMOD_CTRL){
                    showFileDialog = true; //show file dialog
                }
            }

            //farward and backword
            if(event.key.keysym.sym == SDLK_LEFT){
                videoPlayer.seek(-5.0f); //backword 5 seconds
            }
            if(event.key.keysym.sym == SDLK_RIGHT){
                videoPlayer.seek(5.0f); //forward 5 seconds
            }
            //volume control
            if(event.key.keysym.sym == SDLK_DOWN){
                videoPlayer.changeVolume(-0.1f); //decrease volume
            }
            if(event.key.keysym.sym == SDLK_UP){
                videoPlayer.changeVolume(0.1f); //increase volume
            }
        
        }
    }
    
}


//Update LOgc (placeHolder)

void App::update(){
    //logic
}

void App::render(){
    
//importat values
float currentTime = videoPlayer.getcurrentTime();
float duration = videoPlayer.getDuration();

    
    //start imgui frames 
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    //menue bar;
    if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open")) {
            showFileDialog = true;
        }
        if (ImGui::MenuItem("Exit")) {
            isRunning = false;
        }
        ImGui::EndMenu();
    }
    if(ImGui::MenuItem(videoPlayer.getPauseState()? "Play": "Pause")){
        videoPlayer.togglePause();
    }
    if (ImGui::MenuItem("<< Rewind")) {
        videoPlayer.seek(-5.0f);
    }   
    // === Current time display ===
    ImGui::Text("%.2d:%.2d / %.2d:%.2d", 
    (int)currentTime / 60, (int)currentTime % 60,
    (int)duration / 60, (int)duration % 60);


    if (ImGui::MenuItem(">> Forward")) {
       videoPlayer.seek(5.0f);
    }
     if (ImGui::MenuItem("V--"))
    {
        videoPlayer.changeVolume(-0.1f);
    }
    
    std::string volumeText = std::to_string((int) (videoPlayer.volume * 100));
    if (ImGui::MenuItem(volumeText.c_str()))
    {
        videoPlayer.changeVolume(0,true);
    }
   if (ImGui::MenuItem("++V"))
    {
        videoPlayer.changeVolume(0.1f);
    }

    ImGui::EndMainMenuBar();
}



//slider ::::----->>>>>>
static float scrubberValue = 0.0f;
scrubberValue = (currentTime / duration) * 100.0f; // Percentage
static float volume = 0;
volume = videoPlayer.volume * 100.0f; // Percentage

// Bottom timeline container
ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - 50));
ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 50));

ImGui::Begin("TimeLineControl", nullptr, 
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);

ImGui::SetCursorPos(ImVec2(10, 15)); // Add some top margin

// === Timeline slider (long) ===
float totalWidth = ImGui::GetContentRegionAvail().x;
float timelineWidth = totalWidth * 0.75f;
ImGui::PushItemWidth(timelineWidth);
if (ImGui::SliderFloat("##TimeLine", &scrubberValue, 0.0f, 100.0f, "%.1f%%", ImGuiSliderFlags_AlwaysClamp)) {
    videoPlayer.seekTo((scrubberValue / 100.0f) * duration);
}
ImGui::PopItemWidth();


// === Volume slider (short) ===
ImGui::SameLine();
ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 120); // Push volume to right
ImGui::PushItemWidth(100); // Shorter volume slider
if (ImGui::SliderFloat("##Volume", &volume, 0.0f, 200.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp)) {
    videoPlayer.changeVolume(volume / 100.0f - videoPlayer.volume, true);
    videoPlayer.volume = volume / 100.0f;
}
ImGui::PopItemWidth();

ImGui::End();



//file opener
drawFileDialogUI(showFileDialog, selectedFilePath);
    if (!selectedFilePath.empty()) {
        std::cout << "Selected file: " << selectedFilePath << std::endl;
        if(videoPlayer.load(selectedFilePath,renderer)){
            loadedFilePath = selectedFilePath;
        }else{
            std::cerr<<"Failed to load video!!!"<<std::endl;
        }
        selectedFilePath.clear();
    }

    ImGui::Render();

    SDL_SetRenderDrawColor(renderer, 30,30,30,255);
    
    
    //render frames from video 
    if(!loadedFilePath.empty()){
        videoPlayer.renderFrame(renderer);  ///play's video
    }
    //SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(),renderer);
    SDL_RenderPresent(renderer);

    

}
