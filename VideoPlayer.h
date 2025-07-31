#pragma once

#include <SDL2/SDL.h>
#include <string>


extern "C"{
    #include <libswresample/swresample.h>
    #include <libavutil/channel_layout.h> 
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/mem.h>
    #include <stdint.h>
}

class VideoPlayer
{
private:
    AVFormatContext* fmtCtx = nullptr;
    AVCodecContext* CodecCtx = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* rgbFrame = nullptr;
    AVPacket* packet = nullptr;//
    struct SwsContext* swsCtx = nullptr;//
    SDL_Texture* texture = nullptr;//
    int videoStreamIndex = -1;//
    int width = 0, height = 0;
    bool frameReady;
    bool isPaused = false;
    double currentPts = 0.0;
    float seekTargetTime = -1.0f;
    //audio
    int audioStreamIndex = -1;
    AVCodecContext* AudioCodecCtx = nullptr;
    SwrContext* swrCtx = nullptr;

    SDL_AudioDeviceID audioDevice = 0;
    SDL_AudioSpec audioSpec;
    SDL_AudioSpec wantSpec;
    AVFrame* audioFrame = nullptr;
    int channels2 = 1;

    
// For video resampler
struct SwsContext* swsCtxVideo = nullptr;




    
public:
    bool load(const std::string& filepath, SDL_Renderer* renderer);
    void renderFrame(SDL_Renderer* renderer);
    void decodeNextFrame();
    void cleanup();
    void togglePause();
    bool getPauseState();

    //seeking
    void seek(float seconds);

    //volume control

    //progress bar 
    float getcurrentTime();
    float getDuration();
    void seekTo(float time); //used by timeline slider

    void changeVolume(float diffVolume, bool setDefault = false);
    float volume = 2.0f; 
};
