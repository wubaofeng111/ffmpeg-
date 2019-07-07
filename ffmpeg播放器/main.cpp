//
//  main.cpp
//  ffmpeg播放器
//
//  Created by wu baofeng on 2019/7/7.
//  Copyright © 2019 wu baofeng. All rights reserved.
//

#include <iostream>
#include "player.hpp"
extern "C"
{
#include "avformat.h"
#include "swscale.h"
#include "SDL.h"
};

char *outputFile = "/Users/wubaofeng/Desktop/my_github/ffmpeg播放器/ffmpeg播放器/videoinfo.txt";
char *output264File = "/Users/wubaofeng/Desktop/my_github/ffmpeg播放器/ffmpeg播放器/videodata.h264";
char *filePath   = "/Users/wubaofeng/Desktop/file1.mp4";

int main(int argc, const char * argv[]) {
   
    bf_player(filePath, output264File);
    player_main1(filePath);
    
    return 0;
  
}
