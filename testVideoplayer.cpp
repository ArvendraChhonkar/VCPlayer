#include "VideoPlayer.h"
#include <iostream>

bool VideoPlayer::load(const std::string& filepath, SDL_Renderer* renderer) {
    cleanup();

    if (avformat_open_input(&fmtCtx, filepath.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Failed to open input file\n";
        return false;
    }
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        std::cerr << "Failed to find stream info\n";
        return false;
    }

    // --- Audio: Find and open audio stream/codec ---
    audioStreamIndex = -1;
    for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }
    AudioCodecCtx = nullptr; audioFrame = nullptr; audioDevice = 0;
    if (audioStreamIndex != -1) {
        AVCodecParameters* audioPar = fmtCtx->streams[audioStreamIndex]->codecpar;
        const AVCodec* audioCodec = avcodec_find_decoder(audioPar->codec_id);
        AudioCodecCtx = avcodec_alloc_context3(audioCodec);
        avcodec_parameters_to_context(AudioCodecCtx, audioPar);
        avcodec_open2(AudioCodecCtx, audioCodec, nullptr);
        channels2 = AudioCodecCtx->ch_layout.nb_channels;
        SDL_AudioSpec wanted, obtained;
        wanted.freq = AudioCodecCtx->sample_rate;
        wanted.channels = channels2;
        wanted.format = (AudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_FLT) ? AUDIO_F32SYS : AUDIO_S16SYS;
        wanted.silence = 0;
        wanted.samples = 4096;
        wanted.callback = nullptr;

        audioDevice = SDL_OpenAudioDevice(nullptr, 0, &wanted, &obtained, 0);
        if (!audioDevice) {
            std::cerr << "SDL could not open audio device: " << SDL_GetError() << std::endl;
            avcodec_free_context(&AudioCodecCtx); AudioCodecCtx = nullptr;
        } else {
            SDL_PauseAudioDevice(audioDevice, 0);
        }
        audioFrame = av_frame_alloc();
    }

    // --- Video: Find and open video stream/codec ---
    videoStreamIndex = -1;
    for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }
    if (videoStreamIndex == -1) {
        std::cerr << "No video stream found\n";
        return false;
    }
    AVCodecParameters* codecPar = fmtCtx->streams[videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    CodecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(CodecCtx, codecPar);
    avcodec_open2(CodecCtx, codec, nullptr);
    width = CodecCtx->width;
    height = CodecCtx->height;

    swsCtx = sws_getContext(width, height, CodecCtx->pix_fmt,
                            width, height, AV_PIX_FMT_RGB24,
                            SWS_BILINEAR, nullptr, nullptr, nullptr);
    frame = av_frame_alloc();
    rgbFrame = av_frame_alloc();
    packet = av_packet_alloc();

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes);
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer,
                         AV_PIX_FMT_RGB24, width, height, 1);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                SDL_TEXTUREACCESS_STREAMING, width, height);

    frameReady = false;
    isPaused = false;
    currentPts = 0.0;
    seekTargetTime = -1.0f;

    return true;
}

void VideoPlayer::decodeNextFrame() {
    frameReady = false;
    while (true) {
        if (av_read_frame(fmtCtx, packet) < 0) {
            return;
        }
        // Video decode?
        if (packet->stream_index == videoStreamIndex) {
            avcodec_send_packet(CodecCtx, packet);
            if (avcodec_receive_frame(CodecCtx, frame) == 0) {
                float pts = frame->pts * av_q2d(fmtCtx->streams[videoStreamIndex]->time_base);
                currentPts = pts;
                if (seekTargetTime >= 0.0f) {
                    if (pts < seekTargetTime) { av_packet_unref(packet); continue; }
                    else seekTargetTime = -1.0f;
                }
                sws_scale(swsCtx, frame->data, frame->linesize, 0, height,
                          rgbFrame->data, rgbFrame->linesize);
                SDL_UpdateTexture(texture, nullptr, rgbFrame->data[0], rgbFrame->linesize[0]);
                frameReady = true;
                av_packet_unref(packet);
                return;
            }
        }
        // Audio decode?
        if (AudioCodecCtx && audioDevice && packet->stream_index == audioStreamIndex) {
            avcodec_send_packet(AudioCodecCtx, packet);
            while (avcodec_receive_frame(AudioCodecCtx, audioFrame) == 0) {
                int data_size = av_samples_get_buffer_size(
                    nullptr,
                    channels2,
                    audioFrame->nb_samples,
                    AudioCodecCtx->sample_fmt,
                    1
                );
                if (AudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_FLT ||
                    AudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_S16) {
                    SDL_QueueAudio(audioDevice, audioFrame->data[0], data_size);
                } else {
                    // Audio will be skipped if format not supported
                }
            }
        }
        av_packet_unref(packet);
    }
}

void VideoPlayer::renderFrame(SDL_Renderer* renderer) {
    if (isPaused) {
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        return;
    }
    if (!frameReady) decodeNextFrame();
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    frameReady = false;
}

void VideoPlayer::seek(float seconds) {
    if (!fmtCtx || videoStreamIndex < 0) return;
    AVStream* stream = fmtCtx->streams[videoStreamIndex];
    float newTime = getcurrentTime() + seconds;
    if (newTime < 0) newTime = 0;
    float duration = getDuration();
    if (newTime > duration) newTime = duration - 0.01f;
    int64_t targetTimestamp = av_rescale_q(
        static_cast<int64_t>(newTime * AV_TIME_BASE),
        AV_TIME_BASE_Q, stream->time_base
    );
    av_seek_frame(fmtCtx, videoStreamIndex, targetTimestamp, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(CodecCtx);
    if (AudioCodecCtx) avcodec_flush_buffers(AudioCodecCtx);
    seekTargetTime = newTime;
    frameReady = false;
}

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

float VideoPlayer::getcurrentTime() {
    return static_cast<float>(currentPts);
}

float VideoPlayer::getDuration() {
    if (!fmtCtx || fmtCtx->duration == AV_NOPTS_VALUE) return 0.0f;
    return static_cast<float>(fmtCtx->duration) / AV_TIME_BASE;
}

void VideoPlayer::togglePause() {
    isPaused = !isPaused;
    if (AudioCodecCtx && audioDevice) 
        SDL_PauseAudioDevice(audioDevice, isPaused ? 1 : 0);
}
bool VideoPlayer::getPauseState() { return isPaused; }

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
}
