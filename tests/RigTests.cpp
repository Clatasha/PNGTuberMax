#include "../RahiTuber/MaxRig.h"
#include <cassert>
#include <iostream>
int main(){auto bones=MaxRigPreset();for(int y=0;y<=20;++y)for(int x=0;x<=20;++x){MaxRigPoint p{float(x)/20,float(y)/20};auto q=MaxRigWarp(p,bones,{});assert(std::abs(q.x-p.x)<1e-6&&std::abs(q.y-p.y)<1e-6);auto a=MaxRigWarp(p,bones,{20,15,10});assert(std::isfinite(a.x)&&std::isfinite(a.y));}auto root=MaxRigWarp({.5f,.9f},bones,{25,0,0});assert(std::abs(root.x-.5f)<1e-6);auto head=MaxRigWarp({.5f,.15f},bones,{25,0,0});assert(head.x>.55f);std::cout<<"PASS: neutral identity, finite mesh, anchored root, head response\n";}
