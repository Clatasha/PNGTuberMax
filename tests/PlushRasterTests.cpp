#include "../RahiTuber/MaxPlushRaster.h"
#include <iostream>
#include <stdexcept>
int main(){const int w=256,h=256;std::vector<unsigned char> front(w*h*4,0),back(w*h*4,0);
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){float dx=(x-128.f)/96,dy=(y-128.f)/112;int alpha=int(std::clamp((1-std::sqrt(dx*dx+dy*dy))*96,0.f,1.f)*255);int index=(y*w+x)*4;int value=(x/12)%2?255:0;front[index]=front[index+1]=front[index+2]=value*alpha/255;front[index+3]=alpha;back[index]=35*alpha/255;back[index+1]=82*alpha/255;back[index+2]=140*alpha/255;back[index+3]=alpha;}
 auto fc=MaxPlushContour(front.data(),w,h,0,0,w,h),bc=MaxPlushContour(back.data(),w,h,0,0,w,h);MaxPlushPixels f{front.data(),w,h,0,0,w,h,&fc},b{back.data(),w,h,0,0,w,h,&bc};
 auto neutral=MaxPlushRaster(f,b,.45f,0,w,h);for(std::size_t i=0;i<neutral.size();i++)if(std::abs(int(neutral[i])-int(front[i]))>2)throw std::runtime_error("Neutral PNG detail or alpha changed");
 for(float yaw:{-5.f,-2.f,2.f,5.f}){auto image=MaxPlushRaster(f,b,.45f,yaw,w,h);int darkest=255,brightest=0;for(int x=90;x<170;x++){auto at=(128*w+x)*4;darkest=std::min(darkest,int(image[at]));brightest=std::max(brightest,int(image[at]));if(image[at+3]<250)throw std::runtime_error("A stripe appeared in the opaque body");}if(darkest>10||brightest<245)throw std::runtime_error("Fine detail blurred at a slight turn");}
 for(float yaw:{45.f,90.f,135.f,180.f}){auto image=MaxPlushRaster(f,b,.45f,yaw,w,h);for(int y:{48,96,128,160,208}){int first=-1,last=-1;for(int x=0;x<w;x++)if(image[(y*w+x)*4+3]>180){if(first<0)first=x;last=x;}if(first<0)throw std::runtime_error("Empty plush view");for(int x=first+3;x<last-3;x++)if(image[(y*w+x)*4+3]<150)throw std::runtime_error("Joined surface has a gap");}}
 std::cout<<"PASS: original PNG detail/alpha, 2/5-degree contrast and joined rear/side surfaces\n";
}
