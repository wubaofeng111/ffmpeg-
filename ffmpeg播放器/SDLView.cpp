//
//  SDLView.cpp
//  ffmpeg播放器
//
//  Created by wu baofeng on 2019/7/7.
//  Copyright © 2019 wu baofeng. All rights reserved.
//

#include "SDLView.hpp"


int ShowWindow()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *pWindow = SDL_CreateWindow("1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 720, 1080, SDL_WINDOW_SHOWN);
    
    SDL_Renderer *pRender = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderClear(pRender);
    SDL_RenderPresent(pRender);
    
    SDL_Delay(2000);
    SDL_DestroyRenderer(pRender);
    SDL_DestroyWindow(pWindow);
    SDL_Quit();
    return 0;
}
