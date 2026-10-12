// PNG-only plush projection. No skeleton or 3D model asset, by Clatasha.
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
struct MaxPlushSpan {float left=0,right=0;bool valid=false;};
inline std::vector<MaxPlushSpan> MaxPlushContour(const unsigned char* rgba,int imageW,int imageH,int x,int y,int w,int h){
 std::vector<MaxPlushSpan> rows(std::max(0,h));if(!rgba||w<=0||h<=0||x<0||y<0||x+w>imageW||y+h>imageH)return rows;
 for(int row=0;row<h;row++){int l=w,r=-1;for(int col=0;col<w;col++)if(rgba[((y+row)*imageW+x+col)*4+3]>0){l=std::min(l,col);r=col;}
  if(r>=l)rows[row]={float(l),float(r+1),true};}
 return rows;
}
inline float MaxPlushX(float center,float halfWidth,float u,float roundness,float yaw,bool back){
 float radians=yaw*.017453292519943295f;float z=halfWidth*roundness*std::sqrt(std::max(0.f,1-u*u));if(back)z=-z;
 return center+halfWidth*u*std::cos(radians)+z*std::sin(radians);
}
inline bool MaxPlushFacing(float u,float roundness,float yaw,bool back){float rad=yaw*.017453292519943295f;float nz=std::sqrt(std::max(0.f,1-u*u))*(back?-1.f:1.f);return nz*std::cos(rad)-roundness*u*std::sin(rad)>0;}

struct MaxPlushArc {float begin=0,end=0;bool valid=false;};
inline MaxPlushArc MaxPlushVisibleArc(float roundness,float yaw,bool back){
 float angle=yaw*.017453292519943295f;float shift=std::atan2(roundness*std::sin(angle),(back?-1.f:1.f)*std::cos(angle));
 float begin=std::max(0.f,-shift),end=std::min(3.14159265358979323846f,3.14159265358979323846f-shift);
 return {begin,end,end-begin>1e-5f};
}
inline float MaxPlushDragAngle(float yaw,float pixels,float viewportWidth){
 float result=yaw+pixels*360.f/std::max(100.f,viewportWidth);result=std::fmod(result+180.f,360.f);if(result<0)result+=360.f;return result-180.f;
}
