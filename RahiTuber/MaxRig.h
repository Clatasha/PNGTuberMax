#pragma once
#include <cmath>
#include <algorithm>
#include <vector>
struct MaxRigPoint { float x=0,y=0; };
struct MaxRigBone { MaxRigPoint start{.5f,.85f},end{.5f,.4f}; float radius=.45f,strength=1; int role=0; };
struct MaxRigPose { float lean=0,nod=0,turn=0; };
inline std::vector<MaxRigBone> MaxRigPreset(){return {{{.5f,.9f},{.5f,.5f},.65f,.7f,0},{{.5f,.5f},{.5f,.3f},.45f,.7f,1},{{.5f,.3f},{.5f,.12f},.35f,1,2}};}
inline MaxRigPoint MaxRigWarp(MaxRigPoint p,const std::vector<MaxRigBone>& bones,MaxRigPose pose){
 MaxRigPoint result=p;
 for(const auto& b:bones){float dx=b.end.x-b.start.x,dy=b.end.y-b.start.y,len=dx*dx+dy*dy;
 float t=len>1e-8f?std::clamp(((p.x-b.start.x)*dx+(p.y-b.start.y)*dy)/len,0.f,1.f):0;
 float ex=p.x-b.start.x-t*dx,ey=p.y-b.start.y-t*dy;
 float w=std::clamp(1-std::sqrt(ex*ex+ey*ey)/std::max(.01f,b.radius),0.f,1.f);w=w*w*(3-2*w)*t*b.strength;
 float angle=pose.lean*.01745329252f*(b.role==0?.45f:b.role==1?.65f:1.f);
 float x=p.x-b.start.x,y=p.y-b.start.y;
 result.x+=w*(std::cos(angle)*x-std::sin(angle)*y-x+pose.turn*.0015f);
 result.y+=w*(std::sin(angle)*x+std::cos(angle)*y-y+pose.nod*.0015f*(b.role==0?.3f:1.f));
 }return result;
}
