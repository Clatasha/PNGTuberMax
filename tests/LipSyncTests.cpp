#include "../RahiTuber/MaxLipSync.h"
#include <iostream>
#include <stdexcept>
#include <limits>
static void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
int main(){try{
 MaxMouthCalibration c;MaxLipSelector s;
 auto pick=[&](MaxMouthSample m,MaxLipSource source=MaxLipSource::Camera,MaxLipShape audio=MaxLipShape::A,bool speech=false,float hold=0.f){return s.Update(source,m,c,audio,speech,.7f,1.f/60,hold);};
 check(pick(c.poses[0]).shape==MaxLipShape::Idle,"Closed camera mouth should be idle");
 check(pick(c.poses[1]).shape==MaxLipShape::A,"Tall mouth should choose A");
 check(pick(c.poses[2]).shape==MaxLipShape::E,"Wide mouth should choose E");
 check(pick(c.poses[3]).shape==MaxLipShape::OO,"Rounded mouth should choose OO");
 check(pick({.055f,.7f,true}).shape==MaxLipShape::S,"Narrow opening should choose S");
 auto closed=pick(c.poses[0],MaxLipSource::Combined,MaxLipShape::A,true);check(closed.shape==MaxLipShape::PB&&closed.camera,"Audio opens visibly closed lips");
 check(pick(c.poses[3],MaxLipSource::Combined,MaxLipShape::A,true).shape==MaxLipShape::OO,"Audio overrules a clear round mouth");
 check(pick(c.poses[1],MaxLipSource::Camera,MaxLipShape::Idle,false).talking,"Camera ignores silent mouth movement");
 auto fallback=pick({},MaxLipSource::Combined,MaxLipShape::E,true);check(!fallback.camera&&fallback.shape==MaxLipShape::E,"Lost camera fails to use audio");
 auto muted=pick({},MaxLipSource::Camera,MaxLipShape::A,false);check(!muted.camera&&!muted.talking&&muted.shape==MaxLipShape::Idle,"Tracking loss animates a silent mouth");
 check(!pick({std::numeric_limits<float>::quiet_NaN(),.7f,true},MaxLipSource::Camera,MaxLipShape::A,true).camera,"Nonfinite camera data should fall back");
 check(pick(c.poses[3],MaxLipSource::Audio,MaxLipShape::E,true).shape==MaxLipShape::E,"Audio mode changes existing phoneme decisions");
 s.Reset();pick(c.poses[1],MaxLipSource::Camera,MaxLipShape::Idle,false,.05f);
 for(int n=0;n<30;++n)check(pick(c.poses[n%2?1:2],MaxLipSource::Camera,MaxLipShape::Idle,false,.05f).shape==MaxLipShape::A,"Shape jitter causes flicker");
 for(int n=0;n<4;++n)pick(c.poses[2],MaxLipSource::Camera,MaxLipShape::Idle,false,.05f);
 check(pick(c.poses[2],MaxLipSource::Camera,MaxLipShape::Idle,false,.05f).shape==MaxLipShape::E,"Stable shape never changes");
 check(pick(c.poses[0],MaxLipSource::Camera,MaxLipShape::Idle,false,.15f).shape==MaxLipShape::Idle,"Smoothing delays lip closure");
 MaxMouthCapture capture;capture.Start(1);for(int n=0;n<40;++n)capture.Feed({.4f,.8f,true},n%2==0,1.f/60,c);
 check(capture.slot<0&&!capture.failed&&(c.captured&2)&&std::abs(c.poses[1].open-.4f)<.001f,"Calibration average or completion failed");
 capture.Start(2);for(int n=0;n<40;++n)capture.Feed(c.poses[2],false,1.f/60,c);check(capture.failed&&!(c.captured&4),"Duplicate render frames counted as calibration samples");
 c.poses[3].width=std::numeric_limits<float>::infinity();c.captured|=8;c.Sanitize();check(c.poses[3].Usable()&&!(c.captured&8),"Invalid saved calibration does not recover");
 std::cout<<"PASS: five camera shapes, silent expressions, audio/camera/combined selection, immediate closure, flicker suppression, lost/invalid tracking fallback and fresh-frame calibration\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
