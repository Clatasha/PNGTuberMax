#include "../RahiTuber/MaxMouthBank.h"
#include <iostream>
#include <stdexcept>
#include <set>
static void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
int main(){try{
 MaxMouthCalibration c;MaxMouthBank bank;MaxMouthBankSelector selector;constexpr unsigned all=(1u<<13)-1;
 std::set<int> masks;for(int i=0;i<13;++i){masks.insert(int(MaxMouthDefinitions[i].shape));check(MaxMouthIndex(MaxMouthDefinitions[i].shape)==i,"Shape lookup loses a slot");check(MaxMouthFileSlot(std::string("mouth_")+MaxMouthDefinitions[i].stem)==i,"Folder import does not map canonical filename");selector.Reset();auto p=bank.Pose(i,c);auto r=selector.Update(MaxLipSource::Combined,p,c,bank,all,MaxLipShape::Idle,i==1,.5f,1.f/60,0);if(r.shape!=MaxMouthDefinitions[i].shape)throw std::runtime_error(std::string("Suggested camera pose fails for ")+MaxMouthDefinitions[i].label);}
 check(masks.size()==13,"Shape masks collide");check(MaxMouthFileSlot("Mouth_T")==11&&MaxMouthFileSlot("mouth_P")==1&&MaxMouthFileSlot("mouth_idle")==-2&&MaxMouthFileSlot("unrelated")==-1,"Case, aliases, or unknown filenames fail");
 selector.Reset();auto r=selector.Update(MaxLipSource::Camera,c.poses[0],c,bank,all,MaxLipShape::A,false,0,.016f,0);check(r.shape==MaxLipShape::Idle&&!r.talking,"Closed camera expression should idle");
 int l=MaxMouthIndex(MaxLipShape::L);bank.targets[l].pose={.19f,.73f,true,.12f,.85f,true};bank.targets[l].captured=true;
 selector.Reset();r=selector.Update(MaxLipSource::Camera,bank.targets[l].pose,c,bank,all,MaxLipShape::S,false,0,.016f,0);check(r.shape==MaxLipShape::L,"Custom captured mouth pose is ignored");bank.targets[l].enabled=false;selector.Reset();r=selector.Update(MaxLipSource::Camera,bank.targets[l].pose,c,bank,all,MaxLipShape::S,false,0,.016f,0);check(r.shape!=MaxLipShape::L,"Disabled shape selected automatically");
 selector.Reset();r=selector.Update(MaxLipSource::Camera,bank.Pose(9,c),c,bank,1u<<0,MaxLipShape::OO,false,0,.016f,0);check(r.shape==MaxLipShape::A,"Camera selects an unloaded mouth image");
 selector.Reset();r=selector.Update(MaxLipSource::Combined,{},c,bank,all,MaxLipShape::OO,true,1,.016f,0);check(!r.camera&&r.shape==MaxLipShape::Q,"Audio fallback does not use expanded opening levels");
 selector.Reset();r=selector.Update(MaxLipSource::Audio,{},c,bank,all,MaxLipShape::E,true,.22f,.016f,0);check(!r.camera&&(r.shape==MaxLipShape::C||r.shape==MaxLipShape::E),"Audio hint chooses an unrelated shape family");
 selector.Reset();auto a=bank.Pose(0,c),q=bank.Pose(9,c);r=selector.Update(MaxLipSource::Camera,a,c,bank,all,MaxLipShape::A,false,0,.016f,.05f);for(int i=0;i<20;++i){r=selector.Update(MaxLipSource::Camera,i%2?a:q,c,bank,all,MaxLipShape::A,false,0,.016f,.05f);check(r.shape==MaxLipShape::A,"Expanded mouth bank flickers on unstable poses");}r=selector.Update(MaxLipSource::Camera,c.poses[0],c,bank,all,MaxLipShape::A,false,0,.016f,.15f);check(r.shape==MaxLipShape::Idle,"Expanded smoothing delays closure");
 MaxMouthCapture capture;capture.Start(12);MaxMouthSample averaged;int outcome=0;for(int i=0;i<40&&capture.slot>=0;++i)outcome=capture.FeedSample({.2f,.8f,true,.4f,.6f,true},i%2==0,.016f,averaged);check(outcome==1&&averaged.details&&std::abs(averaged.teeth-.4f)<.001f&&std::abs(averaged.tongue-.6f)<.001f,"Pose averaging loses detailed appearance cues");
 std::cout<<"PASS: 13 unique mouth shapes, folder naming, per-shape capture, disabled/unloaded exclusion, audio-family fallback, expanded smoothing and detailed calibration\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
