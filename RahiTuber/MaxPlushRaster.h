// Continuous PNG surface sampling; no triangle strips or mipmap averaging.
#pragma once
#include "MaxPlushMath.h"
#include <cstdint>
#include <thread>
#include <array>
struct MaxPlushPixels {const unsigned char* pixels;int textureW,textureH,x,y,w,h;const std::vector<MaxPlushSpan>* rows;};
inline MaxPlushSpan MaxPlushRow(const std::vector<MaxPlushSpan>& rows,float centerY){
 if(rows.empty())return {};float y=std::clamp(centerY-.5f,0.f,float(rows.size()-1));int a=int(y),b=std::min(a+1,int(rows.size()-1));
 if(!rows[a].valid)return rows[b];if(!rows[b].valid)return rows[a];float t=y-a;return {rows[a].left+(rows[b].left-rows[a].left)*t,rows[a].right+(rows[b].right-rows[a].right)*t,true};
}
inline void MaxPlushBilinear(const MaxPlushPixels& image,float x,float y,float coverage,unsigned char* out){
 x=std::clamp(x-.5f,float(image.x),float(image.x+image.w-1));y=std::clamp(y-.5f,float(image.y),float(image.y+image.h-1));int ix=int(x),iy=int(y),nx=std::min(ix+1,image.x+image.w-1),ny=std::min(iy+1,image.y+image.h-1);float tx=x-ix,ty=y-iy;
 const auto* a=image.pixels+(iy*image.textureW+ix)*4;const auto* b=image.pixels+(iy*image.textureW+nx)*4;const auto* c=image.pixels+(ny*image.textureW+ix)*4;const auto* d=image.pixels+(ny*image.textureW+nx)*4;
 for(int channel=0;channel<4;channel++){float top=a[channel]+(b[channel]-a[channel])*tx,bottom=c[channel]+(d[channel]-c[channel])*tx;out[channel]=static_cast<unsigned char>(std::clamp((top+(bottom-top)*ty)*coverage+.5f,0.f,255.f));}
}
inline std::vector<unsigned char> MaxPlushRaster(const MaxPlushPixels& front,const MaxPlushPixels& back,float roundness,float yaw,int width,int height){
 if(width<=0||height<=0||!front.pixels||!back.pixels||!front.rows||!back.rows)return {};
 std::vector<unsigned char> pixels(std::size_t(width)*height*4,0);float rad=yaw*.017453292519943295f,c=std::cos(rad),s=std::sin(rad),rs=roundness*s,radius2=c*c+rs*rs,radius=std::sqrt(radius2);
 if(radius2<1e-8f)return pixels;float planeWidth=front.w*std::max(1.f,roundness),left=(front.w-planeWidth)*.5f;
 auto renderRows=[&](int first,int last){for(int py=first;py<last;py++){float y=(py+.5f)*front.h/height;auto shape=MaxPlushRow(*front.rows,y);if(!shape.valid||shape.right<=shape.left)continue;float half=(shape.right-shape.left)*.5f,center=(shape.right+shape.left)*.5f,footprint=planeWidth/width/half;float rearY=y*back.h/front.h;auto rear=MaxPlushRow(*back.rows,rearY);
  for(int px=0;px<width;px++){float q=(left+(px+.5f)*planeWidth/width-center)/half;float coverage=std::clamp((radius-std::fabs(q))/footprint+.5f,0.f,1.f);if(coverage<=0)continue;q=std::clamp(q,-radius,radius);float root=std::sqrt(std::max(0.f,radius2-q*q));float u=std::clamp((q*c-rs*root)/radius2,-1.f,1.f),v=(q*rs+c*root)/radius2;bool rearFace=v<0;const auto& image=rearFace?back:front;auto span=rearFace?rear:shape;if(!span.valid||span.right<=span.left)continue;float t=(u+1)*.5f;if(rearFace)t=1-t;float spanWidth=span.right-span.left,margin=std::min(3.f,spanWidth*.2f)/spanWidth*std::fabs(s);t=margin+(1-2*margin)*t;
   MaxPlushBilinear(image,image.x+span.left+spanWidth*t,image.y+(rearFace?rearY:y),coverage,pixels.data()+(std::size_t(py)*width+px)*4);
  }}};
 // Independent scanlines share immutable PNG pixels; bounded workers keep large turns responsive.
 unsigned workers=width*height>=512*512?std::min(4u,std::max(1u,std::thread::hardware_concurrency())):1;
 std::array<std::thread,3> threads;unsigned launched=0;
 try{for(unsigned i=1;i<workers;++i){threads[i-1]=std::thread(renderRows,int(height*i/workers),int(height*(i+1)/workers));++launched;}}
 catch(...){for(unsigned i=launched+1;i<workers;++i)renderRows(int(height*i/workers),int(height*(i+1)/workers));}
 renderRows(0,int(height/workers));for(unsigned i=0;i<launched;++i)threads[i].join();return pixels;
}
