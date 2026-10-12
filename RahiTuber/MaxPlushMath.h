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
