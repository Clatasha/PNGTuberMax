#include "../RahiTuber/CameraFrame.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
int main(){using namespace MaxCameraFrame;Header h;unsigned char bytes[64];std::memcpy(bytes,&h,64);std::uint32_t magic;std::memcpy(&magic,bytes,4);if(magic!=Magic||h.width!=640||h.height!=480||h.active!=0)throw std::runtime_error("Incorrect camera header");const unsigned char rgba[]={255,0,0,30,0,255,0,80,0,0,255,200,10,20,30,255};unsigned char bgra[16]={};RgbaToBgra(rgba,bgra,4);if(bgra[0]!=0||bgra[2]!=255||bgra[5]!=255||bgra[8]!=255||bgra[12]!=30||bgra[14]!=10)throw std::runtime_error("Wrong camera color order");for(int i=3;i<16;i+=4)if(bgra[i]!=255)throw std::runtime_error("Camera output must be opaque");std::cout<<"PASS: camera header layout, channel order and opaque output\n";}
