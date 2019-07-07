//
//  player.cpp
//  ffmpeg播放器
//
//  Created by wu baofeng on 2019/7/7.
//  Copyright © 2019 wu baofeng. All rights reserved.
//

#include "player.hpp"


/**
 * 最简单的基于FFmpeg的视频播放器
 * Simplest FFmpeg Player
 *
 * 雷霄骅 Lei Xiaohua
 * leixiaohua1020@126.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * 本程序实现了视频文件的解码和显示（支持HEVC，H.264，MPEG2等）。
 * 是最简单的FFmpeg视频解码方面的教程。
 * 通过学习本例子可以了解FFmpeg的解码流程。
 * This software is a simplest video player based on FFmpeg.
 * Suitable for beginner of FFmpeg.
 */


#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL/SDL.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "SDL.h"
#ifdef __cplusplus
};
#endif
#endif


#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
int thread_exit=0;


//Full Screen
#define SHOW_FULLSCREEN 0
//Output YUV420P
#define OUTPUT_YUV420P 0

typedef struct SDL_VideoInfo SDL_VideoInfo;
typedef struct SDL_Overlay SDL_Overlay;

int bf_sfp_refresh_thread(void *opaque)
{
    SDL_Event *event = (SDL_Event*)opaque;
    while (thread_exit==0) {
        event->type = SFM_REFRESH_EVENT;
        SDL_PushEvent(event);
        //Wait 40 ms
        SDL_Delay(40);
    }
    return 0;
}

int bf_player(char * filePath,char *output264File)
{
    av_register_all();
    AVFormatContext *pAVFormatContext;
    AVCodecContext  *pAVCodecContext;
    AVCodec         *pAVCodec;
    AVFrame  *pAVFrame,*pAVFrameYUV;
    
    
    pAVFormatContext = avformat_alloc_context();/// 实例化
    
    if (avformat_open_input(&pAVFormatContext, filePath, NULL, NULL) != 0) {
        perror("打开文件错误");
        return -1;
    }
    if(avformat_find_stream_info(pAVFormatContext,NULL)<0){
        perror("找不到文件流");
        return -1;
    }
    
    pAVCodecContext = pAVFormatContext->streams[0]->codec;
    pAVCodec        = avcodec_find_decoder(pAVCodecContext->codec_id);
    
    if(pAVCodec==NULL){
        printf("Codec not found.\n");
        return -1;
    }
    if(avcodec_open2(pAVCodecContext, pAVCodec,NULL)<0){
        printf("Could not open codec.\n");
        return -1;
    }
    
    pAVFrame = av_frame_alloc();
    pAVFrameYUV = av_frame_alloc();
    
    
    printf("%lld",pAVFormatContext->duration);
    printf("{width:%d,height:%d",pAVFormatContext->streams[0]->codec->width,pAVFormatContext->streams[0]->codec->height);
    
    
    
    
    AVPacket* pAVPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
    av_init_packet(pAVPacket);
    
    AVStream *videoStream = pAVFormatContext->streams[0];
    
   
    SwsContext *pSwsContext = sws_getContext(pAVCodecContext->width, pAVCodecContext->height, pAVCodecContext->pix_fmt, pAVCodecContext->width, pAVCodecContext->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    
    FILE* pFileH264 = fopen(output264File, "wb+");
    int yet,got_picture_ptr;
    
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    
    SDL_Window *pWindow;
    SDL_Renderer *pRenderer;
    SDL_Texture  *pTexture;
    SDL_Rect     *pRect;
    pRect = (SDL_Rect*)malloc(sizeof(SDL_Rect));
    
    pRect->x = 0;
    pRect->y = 0;
    pRect->w = pAVCodecContext->width;
    pRect->h = pAVCodecContext->height;
    
    pWindow = SDL_CreateWindow("播放视频", 0, 0, pAVCodecContext->width, pAVCodecContext->height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    pRenderer=SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED);
    pTexture= SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pAVCodecContext->width, pAVCodecContext->height);
    
    
    int y_size=pAVCodecContext->width*pAVCodecContext->height;
    int u_size=y_size/4;
    int v_size=y_size/4;
    
    SDL_Event event;
    
    SDL_Thread*thread_id = SDL_CreateThread(bf_sfp_refresh_thread, "wbf", &event);
    event.type = SFM_REFRESH_EVENT;
    SDL_PushEvent(&event);
    
    while (1) {
        SDL_WaitEvent(&event);
        if (event.type == SFM_REFRESH_EVENT||event.type == 4352) {
            while (av_read_frame(pAVFormatContext, pAVPacket)>=0) {
                if (pAVPacket->stream_index == 0) {
                    fwrite(pAVPacket->data, pAVPacket->size, 1, pFileH264);
                    //            printf("write frame");
                    yet = avcodec_decode_video2(pAVCodecContext, pAVFrame, &got_picture_ptr, pAVPacket);
                    
                    if (yet < 0) {
                        perror("解码错误");
                        return -1;
                    }
                    
                    
                    //            if (got_picture_ptr) {
                    //                int sws_scale(struct SwsContext *c, const uint8_t *const srcSlice[],
                    //                              const int srcStride[], int srcSliceY, int srcSliceH,
                    //                              uint8_t *const dst[], const int dstStride[]);
                    //                sws_scale(pSwsContext,
                    //                          (const uint8_t* const*)pAVFrame->data, pAVFrame->linesize, 0, pAVCodecContext->height, pAVFrameYUV->data, pAVFrameYUV->linesize);
                    //            }
                    SDL_UpdateYUVTexture(pTexture, pRect, pAVFrame->data[0], pAVFrame->linesize[0], pAVFrame->data[1], pAVFrame->linesize[1], pAVFrame->data[2], pAVFrame->linesize[2]);
                    SDL_RenderClear(pRenderer);
                    SDL_RenderCopy(pRenderer,pTexture, NULL, pRect);
                    SDL_RenderPresent(pRenderer);
                    SDL_Delay(10);
                    
                }
                av_free_packet(pAVPacket);
            }
        }else{
            thread_exit = 1;
            break;
        }
       
    }
    
   
    SDL_WaitEvent(NULL);
    
    SDL_DestroyTexture(pTexture);
    SDL_DestroyRenderer(pRenderer);
    SDL_DestroyWindow(pWindow);
    SDL_Quit();
    
    printf("bf_player_结束\n");
    
    return 0;
}


int player_main1(char* filePath)
{
    //FFmpeg
    AVFormatContext    *pFormatCtx;
    int                i, videoindex;
    AVCodecContext    *pCodecCtx;
    AVCodec            *pCodec;
    AVFrame    *pFrame,*pFrameYUV;
    AVPacket *packet;
    struct SwsContext *img_convert_ctx;
    //SDL
    int screen_w,screen_h;
    SDL_Surface *screen;
    SDL_VideoInfo *vi;
    SDL_Overlay *bmp;
    SDL_Rect rect;
    
    FILE *fp_yuv;
    int ret, got_picture;
    
    av_register_all();
    avformat_network_init();
    
    pFormatCtx = avformat_alloc_context();
    
    if(avformat_open_input(&pFormatCtx,filePath,NULL,NULL)!=0){
        printf("Couldn't open input stream.\n");
        return -1;
    }
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){
        printf("Couldn't find stream information.\n");
        return -1;
    }
    videoindex=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            videoindex=i;
            break;
        }
    if(videoindex==-1){
        printf("Didn't find a video stream.\n");
        return -1;
    }
    pCodecCtx=pFormatCtx->streams[videoindex]->codec;
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL){
        printf("Codec not found.\n");
        return -1;
    }
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
        printf("Could not open codec.\n");
        return -1;
    }
    
    pFrame=av_frame_alloc();
    pFrameYUV=av_frame_alloc();
    //uint8_t *out_buffer=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    //avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
    //SDL----------------------------
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    
    
    
#if SHOW_FULLSCREEN
    vi = SDL_GetVideoInfo();
    screen_w = vi->current_w;
    screen_h = vi->current_h;
    screen = SDL_SetVideoMode(screen_w, screen_h, 0,SDL_FULLSCREEN);
#else
    screen_w = pCodecCtx->width;
    screen_h = pCodecCtx->height;
    
//    screen = SDL_SetVideoMode(screen_w, screen_h, 0,0);
#endif
    
    if(!screen) {
        printf("SDL: could not set video mode - exiting:%s\n",SDL_GetError());
        return -1;
    }
    
//    bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height,SDL_YV12_OVERLAY, screen);
    
    rect.x = 0;
    rect.y = 0;
    rect.w = screen_w;
    rect.h = screen_h;
    //SDL End------------------------
    
    
    packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    //Output Information-----------------------------
    printf("------------- File Information ------------------\n");
    av_dump_format(pFormatCtx,0,filePath,0);
    printf("-------------------------------------------------\n");
    
#if OUTPUT_YUV420P
    fp_yuv=fopen("output.yuv","wb+");
#endif
    
//    SDL_WM_SetCaption("Simplest FFmpeg Player",NULL);
    
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, 
    pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    //------------------------------
    while(av_read_frame(pFormatCtx, packet)>=0){
        if(packet->stream_index==videoindex){
            //Decode
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if(ret < 0){
                printf("Decode Error.\n");
                return -1;
            }
            if(got_picture){
//                SDL_LockYUVOverlay(bmp);
//                pFrameYUV->data[0]=bmp->pixels[0];
//                pFrameYUV->data[1]=bmp->pixels[2];
//                pFrameYUV->data[2]=bmp->pixels[1];
//                pFrameYUV->linesize[0]=bmp->pitches[0];
//                pFrameYUV->linesize[1]=bmp->pitches[2];
//                pFrameYUV->linesize[2]=bmp->pitches[1];
                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
                          pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
//#if OUTPUT_YUV420P
//                int y_size=pCodecCtx->width*pCodecCtx->height;
//                fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y
//                fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
//                fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
//#endif
//
//                SDL_UnlockYUVOverlay(bmp);
//
//                SDL_DisplayYUVOverlay(bmp, &rect);
                //Delay 40ms
                SDL_Delay(40);
            }
        }
        av_free_packet(packet);
    }
    
    //FIX: Flush Frames remained in Codec
    while (1) {
        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
        if (ret < 0)
            break;
        if (!got_picture)
            break;
        sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
        
//        SDL_LockYUVOverlay(bmp);
//        pFrameYUV->data[0]=bmp->pixels[0];
//        pFrameYUV->data[1]=bmp->pixels[2];
//        pFrameYUV->data[2]=bmp->pixels[1];
//        pFrameYUV->linesize[0]=bmp->pitches[0];
//        pFrameYUV->linesize[1]=bmp->pitches[2];
//        pFrameYUV->linesize[2]=bmp->pitches[1];
#if OUTPUT_YUV420P
        int y_size=pCodecCtx->width*pCodecCtx->height;
        fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y
        fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
        fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
#endif
        
//        SDL_UnlockYUVOverlay(bmp);
//        SDL_DisplayYUVOverlay(bmp, &rect);
        //Delay 40ms
        SDL_Delay(40);
    }
    
    sws_freeContext(img_convert_ctx);
    
#if OUTPUT_YUV420P
    fclose(fp_yuv);
#endif
    
    SDL_Quit();
    
    //av_free(out_buffer);
    av_free(pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    
    return 0;
}

/*
1.1版之后，新添加了一个工程：simplest_ffmpeg_player_su（SU版）。


标准版在播放视频的时候，画面显示使用延时40ms的方式。这么做有两个后果：
（1）SDL弹出的窗口无法移动，一直显示是忙碌状态
（2）画面显示并不是严格的40ms一帧，因为还没有考虑解码的时间。SU（SDL Update）版在视频解码的过程中，不再使用延时40ms的方式，而是创建了一个线程，每隔40ms发送一个自定义的消息，告知主函数进行解码显示。这样做之后：
（1）SDL弹出的窗口可以移动了
（2）画面显示是严格的40ms一帧

simplest_ffmpeg_player_su（SU版）代码*/

/**
 * 最简单的基于FFmpeg的视频播放器SU(SDL升级版)
 * Simplest FFmpeg Player (SDL Update)
 *
 * 雷霄骅 Lei Xiaohua
 * leixiaohua1020@126.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * 本程序实现了视频文件的解码和显示（支持HEVC，H.264，MPEG2等）。
 * 是最简单的FFmpeg视频解码方面的教程。
 * 通过学习本例子可以了解FFmpeg的解码流程。
 * 本版本中使用SDL消息机制刷新视频画面。
 * This software is a simplest video player based on FFmpeg.
 * Suitable for beginner of FFmpeg.
 *
 * Version:1.2
 *
 * 备注:
 * 标准版在播放视频的时候，画面显示使用延时40ms的方式。这么做有两个后果：
 * （1）SDL弹出的窗口无法移动，一直显示是忙碌状态
 * （2）画面显示并不是严格的40ms一帧，因为还没有考虑解码的时间。
 * SU（SDL Update）版在视频解码的过程中，不再使用延时40ms的方式，而是创建了
 * 一个线程，每隔40ms发送一个自定义的消息，告知主函数进行解码显示。这样做之后：
 * （1）SDL弹出的窗口可以移动了
 * （2）画面显示是严格的40ms一帧
 * Remark:
 * Standard Version use's SDL_Delay() to control video's frame rate, it has 2
 * disadvantages:
 * (1)SDL's Screen can't be moved and always "Busy".
 * (2)Frame rate can't be accurate because it doesn't consider the time consumed
 * by avcodec_decode_video2()
 * SU（SDL Update）Version solved 2 problems above. It create a thread to send SDL
 * Event every 40ms to tell the main loop to decode and show video frames.
 */


#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL/SDL.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "SDL.h"
#ifdef __cplusplus
};
#endif
#endif

//Refresh


//Thread
int sfp_refresh_thread(void *opaque)
{
    SDL_Event event;
    while (thread_exit==0) {
        event.type = SFM_REFRESH_EVENT;
        SDL_PushEvent(&event);
        //Wait 40 ms
        SDL_Delay(40);
    }
    return 0;
}


int player_main(int argc, char* argv[])
{
    AVFormatContext    *pFormatCtx;
    int                i, videoindex;
    AVCodecContext    *pCodecCtx;
    AVCodec            *pCodec;
    AVFrame    *pFrame,*pFrameYUV;
    AVPacket *packet;
    struct SwsContext *img_convert_ctx;
    //SDL
    int ret, got_picture;
    int screen_w=0,screen_h=0;
    SDL_Surface *screen;
    SDL_Overlay *bmp;
    SDL_Rect rect;
    SDL_Thread *video_tid;
    SDL_Event event;
    
    char filepath[]="bigbuckbunny_480x272.h265";
    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    
    if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0){
        printf("Couldn't open input stream.\n");
        return -1;
    }
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){
        printf("Couldn't find stream information.\n");
        return -1;
    }
    videoindex=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            videoindex=i;
            break;
        }
    if(videoindex==-1){
        printf("Didn't find a video stream.\n");
        return -1;
    }
    pCodecCtx=pFormatCtx->streams[videoindex]->codec;
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL)
    {
        printf("Codec not found.\n");
        return -1;
    }
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
    {
        printf("Could not open codec.\n");
        return -1;
    }
    
    pFrame=av_frame_alloc();
    pFrameYUV=av_frame_alloc();
    //uint8_t *out_buffer=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    //avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
    //------------SDL----------------
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    
    
    screen_w = pCodecCtx->width;
    screen_h = pCodecCtx->height;
//    screen = SDL_SetVideoMode(screen_w, screen_h, 0,0);
    
    if(!screen) {
        printf("SDL: could not set video mode - exiting:%s\n",SDL_GetError());
        return -1;
    }
    
//    bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height,SDL_YV12_OVERLAY, screen);
    
    rect.x = 0;
    rect.y = 0;
    rect.w = screen_w;
    rect.h = screen_h;
    
    packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    
    printf("---------------File Information------------------\n");
    av_dump_format(pFormatCtx,0,filepath,0);
    printf("-------------------------------------------------\n");
    
    
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    //--------------
//    video_tid = SDL_CreateThread(sfp_refresh_thread,NULL);
    //
//    SDL_WM_SetCaption("Simple FFmpeg Player (SDL Update)",NULL);
    
    //Event Loop
    
    for (;;) {
        //Wait
        SDL_WaitEvent(&event);
        if(event.type==SFM_REFRESH_EVENT){
            //------------------------------
            if(av_read_frame(pFormatCtx, packet)>=0){
                if(packet->stream_index==videoindex){
                    ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
                    if(ret < 0){
                        printf("Decode Error.\n");
                        return -1;
                    }
                    if(got_picture){
                        
//                        SDL_LockYUVOverlay(bmp);
//                        pFrameYUV->data[0]=bmp->pixels[0];
//                        pFrameYUV->data[1]=bmp->pixels[2];
//                        pFrameYUV->data[2]=bmp->pixels[1];
//                        pFrameYUV->linesize[0]=bmp->pitches[0];
//                        pFrameYUV->linesize[1]=bmp->pitches[2];
//                        pFrameYUV->linesize[2]=bmp->pitches[1];
                        sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
                        
//                        SDL_UnlockYUVOverlay(bmp);
//
//                        SDL_DisplayYUVOverlay(bmp, &rect);
                        
                    }
                }
                av_free_packet(packet);
            }else{
                //Exit Thread
                thread_exit=1;
                break;
            }
        }
        
    }
    
    SDL_Quit();
    
    sws_freeContext(img_convert_ctx);
    
    //--------------
    //av_free(out_buffer);
    av_free(pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    
    return 0;
}

