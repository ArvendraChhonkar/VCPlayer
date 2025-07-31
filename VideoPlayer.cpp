#include "VideoPlayer.h"
#include <iostream>


// Load and initialize resources for the selected media file (video and audio)
bool VideoPlayer::load(const std::string& filepath, SDL_Renderer* renderer) {
    cleanup(); // Always clean any previous video, decoder, and texture state before loading new file

    // Open the media file (all formats, let ffmpeg auto-detect container)
    if (avformat_open_input(&fmtCtx, filepath.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Failed to open input file\n";
        return false;
    }
    // Scan for all stream info (finds audio, video, subtitle, etc.)
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        std::cerr << "Failed to find stream info\n";
        return false;
    }

    // ==================== AUDIO SETUP ====================
    // Find and open audio stream/codec if present (optional: may not exist)
    audioStreamIndex = -1; // Default: no audio found
    for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
        // Check if the stream type is audio
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }
    // Initialize all audio members to null/zero for safety
    AudioCodecCtx = nullptr; audioFrame = nullptr; audioDevice = 0;

    if (audioStreamIndex != -1) { // Audio found!
        AVCodecParameters* audioPar = fmtCtx->streams[audioStreamIndex]->codecpar;
        const AVCodec* audioCodec = avcodec_find_decoder(audioPar->codec_id); // Find decoder for this audio

        AudioCodecCtx = avcodec_alloc_context3(audioCodec); // Allocate audio decoder context
        avcodec_parameters_to_context(AudioCodecCtx, audioPar); // Copy params/settings to decoder
        avcodec_open2(AudioCodecCtx, audioCodec, nullptr); // Actually open the audio decoder

        // ----- Get audio channel count (modern FFmpeg: ch_layout.nb_channels), fallback to 2 if not present
        channels2 = AudioCodecCtx->ch_layout.nb_channels > 0 ? AudioCodecCtx->ch_layout.nb_channels : 2;

        // Prepare SDL for audio output matching the decoded audio (try to match ideal: stereo/float, else S16)
        SDL_AudioSpec wanted, obtained; // desired and actually got
        wanted.freq = AudioCodecCtx->sample_rate;
        wanted.channels = channels2;
        wanted.format = (AudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_FLT) ? AUDIO_F32SYS : AUDIO_S16SYS;
        wanted.silence = 0;
        wanted.samples = 2048; // buffer length (2048 sample-frames)
        wanted.callback = nullptr; // use SDL_QueueAudio

        // Open the SDL audio device
        audioDevice = SDL_OpenAudioDevice(nullptr, 0, &wanted, &obtained, 0);
        if (!audioDevice) {
            std::cerr << "SDL could not open audio device: " << SDL_GetError() << std::endl;
            avcodec_free_context(&AudioCodecCtx); AudioCodecCtx = nullptr;
        } else {
            SDL_PauseAudioDevice(audioDevice, 0); // Start playback immediately
        }
        audioFrame = av_frame_alloc(); // Creates empty audio frame to receive decoded PCM
    }

    // ==================== VIDEO SETUP ====================
    // Find and open the video stream/codec
    videoStreamIndex = -1; // Reset to -1 for safety
    for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i; // Mark this as our video stream
            break;
        }
    }
    if (videoStreamIndex == -1) {
        std::cerr << "No video stream found\n";
        return false; // can't play files without video
    }

    AVCodecParameters* codecPar = fmtCtx->streams[videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    CodecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(CodecCtx, codecPar);
    avcodec_open2(CodecCtx, codec, nullptr);

    width = CodecCtx->width;
    height = CodecCtx->height;

    // Set up pixel format conversion context (planar YUV → packed RGB) for SDL
    swsCtx = sws_getContext(
        width, height, CodecCtx->pix_fmt,                  // input: width, height, pixfmt from codec
        width, height, AV_PIX_FMT_RGB24,                   // output: width, height, pixel format RGB24
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    frame = av_frame_alloc();     // Allocates space for raw video frame (decoded)
    rgbFrame = av_frame_alloc();  // Will hold RGB24 data for SDL
    packet = av_packet_alloc();   // Large-enough space for compressed packet

    // Allocate buffer for RGB frame, and associate with frame's data pointers/strides
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes); // raw buffer for converted RGB data
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer,
                         AV_PIX_FMT_RGB24, width, height, 1);

    // Actually create the SDL texture: the GPU-visible surface we update each frame
    texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING, width, height
    );

    // Reset state for playback loop
    frameReady = false;
    isPaused = false;
    currentPts = 0.0;
    seekTargetTime = -1.0f;

    return true;
}

// Decodes the next available video frame (and any queued audio packets) to ready for display
void VideoPlayer::decodeNextFrame() {
    frameReady = false;
    while (true) {
        if (av_read_frame(fmtCtx, packet) < 0) {
            // End of stream or read error
            return;
        }
        // --- Video packet? decode and push to screen ---
        if (packet->stream_index == videoStreamIndex) {
            avcodec_send_packet(CodecCtx, packet);
            if (avcodec_receive_frame(CodecCtx, frame) == 0) {
                float pts = frame->pts * av_q2d(fmtCtx->streams[videoStreamIndex]->time_base); // pts → seconds
                currentPts = pts;
                // If user recently sought: skip frames until we're at/playhead
                if (seekTargetTime >= 0.0f) {
                    if (pts < seekTargetTime) { av_packet_unref(packet); continue; }
                    else seekTargetTime = -1.0f; // Arrived at or past seek point
                }
                // Convert YUV to RGB24 and update GPU video texture
                sws_scale(
                    swsCtx, frame->data, frame->linesize, 0, height,
                    rgbFrame->data, rgbFrame->linesize
                );
                SDL_UpdateTexture(texture, nullptr, rgbFrame->data[0], rgbFrame->linesize[0]);
                frameReady = true; // ready for renderFrame()
                av_packet_unref(packet);
                return;
            }
        }
        // --- Audio packet? Decode and play sound (if device available) ---
        if (AudioCodecCtx && audioDevice && packet->stream_index == audioStreamIndex) {
            avcodec_send_packet(AudioCodecCtx, packet);
            while (avcodec_receive_frame(AudioCodecCtx, audioFrame) == 0) {
                // Compute required raw buffer size for the decoded PCM
                int data_size = av_samples_get_buffer_size(
                    nullptr,
                    channels2, // Use our detected channel count
                    audioFrame->nb_samples,
                    AudioCodecCtx->sample_fmt,
                    1
                );
                // Only direct-play for planar float or s16 (most common for mp4, avi, mkv)
                if (AudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_FLT) {
                    float* samples = (float*)audioFrame->data[0]; //it is just a pointer to the first channel
                    int num_samples = audioFrame->nb_samples * channels2; // total samples in all channels
                    // Scale volume if needed
                    for(int i = 0; i < num_samples; i++){
                        samples[i] *= volume;
                    }
                         
                    SDL_QueueAudio(audioDevice,audioFrame->data[0], data_size);
                } 
                else if (AudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_S16) {
                    int16_t* samples = (int16_t*)audioFrame->data[0]; // pointer to first channel
                    int num_samples = audioFrame->nb_samples * channels2; // total samples in all channels
                    
                    for (short i = 0; i < num_samples; i++)
                    {
                        //since it is 16 bit , we need to consider the overflow
                        int32_t v = samples[i] * volume;
                        if(v > INT16_MAX) v = INT16_MAX;
                        else if(v < INT16_MIN) v = INT16_MIN;
                        samples[i] = static_cast<int16_t>(v); // scale volume
                    }
                    
                    SDL_QueueAudio(audioDevice, audioFrame->data[0], data_size);
                } 
                
                
                
                else {
                    // Audio is skipped if sample_fmt is not directly compatible (rare in popular files)
                }
            }
        }
        av_packet_unref(packet); // Always unref the packet after processing (FFmpeg requirement)
    }
}

// Render the current video frame to the SDL window (draws last decoded frame or decodes new one)
void VideoPlayer::renderFrame(SDL_Renderer* renderer) {
    if (isPaused) {
        // If paused, simply blit the current texture to the screen
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        return;
    }
    if (!frameReady) decodeNextFrame(); // If not ready, decode a new frame (may update frameReady)
    SDL_RenderCopy(renderer, texture, nullptr, nullptr); // Display current frame
    frameReady = false; // Mark frame as stale; will decode next one on next call
}

// Seek forward/backward by a certain number of seconds (relative seek)
void VideoPlayer::seek(float seconds) {
    if (!fmtCtx || videoStreamIndex < 0) return;
    AVStream* stream = fmtCtx->streams[videoStreamIndex];
    float newTime = getcurrentTime() + seconds;
    if (newTime < 0) newTime = 0;
    float duration = getDuration();
    if (newTime > duration) newTime = duration - 0.01f; // Clamp
    // Calculate the corresponding PTS in stream's time_base units
    int64_t targetTimestamp = av_rescale_q(
        static_cast<int64_t>(newTime * AV_TIME_BASE),
        AV_TIME_BASE_Q, stream->time_base
    );
    // Instruct FFmpeg to seek
    av_seek_frame(fmtCtx, videoStreamIndex, targetTimestamp, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(CodecCtx); // Discard all already-decoded data
    if (AudioCodecCtx) avcodec_flush_buffers(AudioCodecCtx);
    seekTargetTime = newTime;  // Set the target time for frame-accurate seeking in decodeNextFrame
    frameReady = false;
}

// Seek to a specific time in the video, in seconds (absolute seek)
void VideoPlayer::seekTo(float time) {
    if (!fmtCtx || videoStreamIndex < 0) return;
    AVStream* stream = fmtCtx->streams[videoStreamIndex];
    float seekTime = time;
    if (seekTime < 0) seekTime = 0;
    float duration = getDuration();
    if (seekTime > duration) seekTime = duration - 0.01f;
    int64_t targetTimestamp = av_rescale_q(
        static_cast<int64_t>(seekTime * AV_TIME_BASE),
        AV_TIME_BASE_Q, stream->time_base
    );
    av_seek_frame(fmtCtx, videoStreamIndex, targetTimestamp, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(CodecCtx);
    if (AudioCodecCtx) avcodec_flush_buffers(AudioCodecCtx);
    seekTargetTime = seekTime;
    frameReady = false;
}

// Get the current playback time, in seconds
float VideoPlayer::getcurrentTime() {
    return static_cast<float>(currentPts);
}

// Get the total duration, in seconds
float VideoPlayer::getDuration() {
    if (!fmtCtx || fmtCtx->duration == AV_NOPTS_VALUE) return 0.0f;
    return static_cast<float>(fmtCtx->duration) / AV_TIME_BASE;
}

// Toggle video playback (pause/resume), and pause/resume audio if available
void VideoPlayer::togglePause() {
    isPaused = !isPaused;
    if (AudioCodecCtx && audioDevice) 
        SDL_PauseAudioDevice(audioDevice, isPaused ? 1 : 0);
}

// Is playback currently paused?
bool VideoPlayer::getPauseState() { return isPaused; }

//volume control
// Set the volume level (0.0 to 2.0)
void VideoPlayer::changeVolume(float diffVolume , bool setDefault){
    if(setDefault){
        if(volume == 1.0f){
            volume = 0.0f;
        }
        else{
        volume = 1.0f; // Reset to default volume
        }
    }else{
        volume += diffVolume; 
        if (volume < 0.0f) volume = 0.0f; // Clamp to minimum
        else if (volume > 2.0f) volume = 2.0f; // Clamp to maximum
    }
     if (audioDevice){
        SDL_ClearQueuedAudio(audioDevice);
     }

}





// Cleanup all dynamically allocated resources for this file
void VideoPlayer::cleanup() {
    if (packet) av_packet_free(&packet);
    if (frame) av_frame_free(&frame);
    if (rgbFrame) { av_free(rgbFrame->data[0]); av_frame_free(&rgbFrame); }
    if (texture) { SDL_DestroyTexture(texture); texture = nullptr; }
    if (CodecCtx) avcodec_free_context(&CodecCtx);
    if (fmtCtx) avformat_close_input(&fmtCtx);
    if (swsCtx) sws_freeContext(swsCtx);
    if (audioDevice) SDL_CloseAudioDevice(audioDevice);
    if (AudioCodecCtx) avcodec_free_context(&AudioCodecCtx);
    if (audioFrame) av_frame_free(&audioFrame);
    audioDevice = 0;
    AudioCodecCtx = nullptr;
    audioFrame = nullptr;
    frameReady = false;
    seekTargetTime = -1.0f;
    audioStreamIndex = -1;
    videoStreamIndex = -1;
}
