// Continuous PNG surface sampling; no triangle strips or mipmap averaging.
#pragma once
#include "MaxPlushMath.h"
#include <cstdint>
#include <thread>
#include <array>
struct MaxPlushPixels {const unsigned char* pixels;int textureW,textureH,x,y,w,h;const std::vector<MaxPlushSpan>* rows;const std::vector<float>* depth=nullptr;};
inline MaxPlushSpan MaxPlushRow(const std::vector<MaxPlushSpan>& rows,float centerY){
 if(rows.empty())return {};float y=std::clamp(centerY-.5f,0.f,float(rows.size()-1));int a=int(y),b=std::min(a+1,int(rows.size()-1));
 if(!rows[a].valid)return rows[b];if(!rows[b].valid)return rows[a];float t=y-a;return {rows[a].left+(rows[b].left-rows[a].left)*t,rows[a].right+(rows[b].right-rows[a].right)*t,true};
}
inline void MaxPlushBilinear(const MaxPlushPixels& image,float x,float y,float coverage,unsigned char* out){
 x=std::clamp(x-.5f,float(image.x),float(image.x+image.w-1));y=std::clamp(y-.5f,float(image.y),float(image.y+image.h-1));int ix=int(x),iy=int(y),nx=std::min(ix+1,image.x+image.w-1),ny=std::min(iy+1,image.y+image.h-1);float tx=x-ix,ty=y-iy;
 const auto* a=image.pixels+(iy*image.textureW+ix)*4;const auto* b=image.pixels+(iy*image.textureW+nx)*4;const auto* c=image.pixels+(ny*image.textureW+ix)*4;const auto* d=image.pixels+(ny*image.textureW+nx)*4;
 for(int channel=0;channel<4;channel++){float top=a[channel]+(b[channel]-a[channel])*tx,bottom=c[channel]+(d[channel]-c[channel])*tx;out[channel]=static_cast<unsigned char>(std::clamp((top+(bottom-top)*ty)*coverage+.5f,0.f,255.f));}
}
// One continuous height field. Transparency masks artwork, never independently moves rows.
inline std::vector<float> MaxPlushDepth(const MaxPlushPixels& image,float flatness=.5f){
 const int w=image.w,h=image.h;std::vector<float> d(std::size_t(w)*h,0);
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){auto alpha=image.pixels[((y+image.y)*image.textureW+x+image.x)*4+3];if(alpha>32)d[y*w+x]=float(std::min({x+1,y+1,w-x,h-y}));}
 auto relax=[&](int x,int y,int dx,int dy,float cost){int nx=x+dx,ny=y+dy;if(nx>=0&&nx<w&&ny>=0&&ny<h)d[y*w+x]=std::min(d[y*w+x],d[ny*w+nx]+cost);};
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){relax(x,y,-1,0,1);relax(x,y,0,-1,1);relax(x,y,-1,-1,1.41421356f);relax(x,y,1,-1,1.41421356f);}
 for(int y=h-1;y>=0;--y)for(int x=w-1;x>=0;--x){relax(x,y,1,0,1);relax(x,y,0,1,1);relax(x,y,1,1,1.41421356f);relax(x,y,-1,1,1.41421356f);}
 float maximum=*std::max_element(d.begin(),d.end());if(maximum<=0)return d;
 // Thin details receive shallow depth instead of inheriting the body's full thickness.
 for(auto& value:d){float t=std::clamp((value-1)/std::max(1.f,maximum-1),0.f,1.f);float q=std::min(1.f,t/std::max(.2f,1-flatness*.8f));value=maximum*std::sqrt(q*(2-q))*std::min(1.f,t*8);}
 return d;
}
inline float MaxPlushDepthAt(const std::vector<float>& d,int w,int h,float x,float y){
 x=std::clamp(x-.5f,0.f,float(w-1));y=std::clamp(y-.5f,0.f,float(h-1));int a=int(x),b=int(y),c=std::min(a+1,w-1),e=std::min(b+1,h-1);float tx=x-a,ty=y-b;
 return (d[b*w+a]*(1-tx)+d[b*w+c]*tx)*(1-ty)+(d[e*w+a]*(1-tx)+d[e*w+c]*tx)*ty;
}
inline std::vector<unsigned char> MaxPlushRaster(const MaxPlushPixels& front,const MaxPlushPixels& back,float roundness,float yaw,int width,int height){
 if(width<=0||height<=0||front.w<=0||front.h<=0||!front.pixels||!back.pixels)return {};
 std::vector<unsigned char> pixels(std::size_t(width)*height*4,0);std::vector<float> buffer(std::size_t(width)*height,-1e30f);
 float rad=yaw*.017453292519943295f,c=std::cos(rad),s=std::sin(rad),planeWidth=front.w*std::max(1.f,roundness),left=(front.w-planeWidth)*.5f;
 if(std::abs(s)<1e-6f){const auto& image=c>=0?front:back;for(int y=0;y<height;++y)for(int x=0;x<width;++x){float u=(left+(x+.5f)*planeWidth/width)/front.w;MaxPlushBilinear(image,image.x+u*image.w,image.y+(y+.5f)*image.h/height,1,pixels.data()+(std::size_t(y)*width+x)*4);}return pixels;}
 auto built=front.depth?std::vector<float>{}:MaxPlushDepth(front);const auto& depth=front.depth?*front.depth:built;
 struct V{float x,y,z,u,v;};
 const int columns=std::min(192,front.w),rows=std::min(192,front.h);
 auto vertex=[&](int x,int y,bool rear){float u=float(x)/columns,v=float(y)/rows,px=u*front.w,py=v*front.h;
 float z=MaxPlushDepthAt(depth,front.w,front.h,px,py)*roundness*(rear?-1.f:1.f);
 return V{((px-front.w*.5f)*c-z*s+front.w*.5f-left)*width/planeWidth,py*height/front.h,(px-front.w*.5f)*s+z*c,u,v};};
 auto triangle=[&](V a,V b,V d,bool rear){float area=(b.x-a.x)*(d.y-a.y)-(b.y-a.y)*(d.x-a.x);if(std::abs(area)<1e-7f)return;
 int x0=std::max(0,int(std::floor(std::min({a.x,b.x,d.x})))),x1=std::min(width-1,int(std::ceil(std::max({a.x,b.x,d.x}))));
 int y0=std::max(0,int(std::floor(std::min({a.y,b.y,d.y})))),y1=std::min(height-1,int(std::ceil(std::max({a.y,b.y,d.y}))));
 const auto& image=rear?back:front;
 for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){float px=x+.5f,py=y+.5f;
 float wb=((px-a.x)*(d.y-a.y)-(py-a.y)*(d.x-a.x))/area,wd=((b.x-a.x)*(py-a.y)-(b.y-a.y)*(px-a.x))/area,wa=1-wb-wd;
 if(wa<-.00001f||wb<-.00001f||wd<-.00001f)continue;float z=wa*a.z+wb*b.z+wd*d.z;auto index=std::size_t(y)*width+x;if(z<buffer[index]-1e-5f&&pixels[index*4+3]==255)continue;
 float u=wa*a.u+wb*b.u+wd*d.u,v=wa*a.v+wb*b.v+wd*d.v;unsigned char sample[4];MaxPlushBilinear(image,image.x+(rear?1-u:u)*image.w,image.y+v*image.h,1,sample);
 if(!sample[3])continue;auto out=pixels.data()+index*4;
 if(std::abs(z-buffer[index])<1e-5f){if(sample[3]>=out[3])std::copy(sample,sample+4,out);continue;}
 if(z>=buffer[index]){float under=1-sample[3]/255.f;for(int ch=0;ch<4;++ch)out[ch]=static_cast<unsigned char>(std::min(255.f,sample[ch]+out[ch]*under+.5f));buffer[index]=z;}
 else{float under=1-out[3]/255.f;for(int ch=0;ch<4;++ch)out[ch]=static_cast<unsigned char>(std::min(255.f,out[ch]+sample[ch]*under+.5f));}
 }};
 // Back first: the front wins equal-depth pixels at transparent/thin boundaries.
 for(bool rear:{true,false})for(int y=0;y<rows;++y)for(int x=0;x<columns;++x){auto a=vertex(x,y,rear),b=vertex(x+1,y,rear),d=vertex(x+1,y+1,rear),e=vertex(x,y+1,rear);triangle(a,b,d,rear);triangle(a,d,e,rear);}
 return pixels;
}
