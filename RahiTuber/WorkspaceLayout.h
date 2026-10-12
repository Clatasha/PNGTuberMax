// PNGTuber Max workspace geometry and raw audio meter, by Clatasha.
#pragma once
#include <algorithm>
#include <cmath>
struct MaxRect {float x,y,w,h;};
struct MaxWorkspace {
 MaxRect left,right,canvas,fit;
 static MaxWorkspace Make(float w,float h,float sceneW,float sceneH){
  float gap=12,left=std::min(360.f,w*.25f),right=std::min(420.f,w*.30f);MaxWorkspace out;
  out.left={gap,gap,left,std::max(100.f,h-2*gap)};out.right={w-right-gap,gap,right,std::max(100.f,h-2*gap)};
  out.canvas={left+2*gap,54,std::max(1.f,w-left-right-4*gap),std::max(1.f,h-78)};
  float scale=std::min(out.canvas.w/std::max(1.f,sceneW),out.canvas.h/std::max(1.f,sceneH));
  out.fit={out.canvas.x+(out.canvas.w-sceneW*scale)*.5f,out.canvas.y+(out.canvas.h-sceneH*scale)*.5f,sceneW*scale,sceneH*scale};return out;
 }
};
inline int MaxMeterBlocks(float peak){if(!std::isfinite(peak)||peak<=0)return 0;const float limits[]={.002f,.008f,.032f,.126f,.355f,.708f};int blocks=0;for(float threshold:limits)if(peak>=threshold)blocks++;return blocks;}
