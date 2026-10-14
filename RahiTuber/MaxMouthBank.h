#pragma once
#include "MaxLipSync.h"
#include <string>
#include <cctype>
// The original five mask values and XML names are never renumbered.
struct MaxMouthDefinition {MaxLipShape shape;const char* label;const char* stem;float opening,width,teeth,tongue;MaxLipShape family;};
inline constexpr std::array<MaxMouthDefinition,13> MaxMouthDefinitions{{
 {MaxLipShape::A,"A","a",1.f,.56f,.25f,.25f,MaxLipShape::A},
 {MaxLipShape::PB,"B / P","b",0.f,.60f,0,0,MaxLipShape::PB},
 {MaxLipShape::C,"C","c",.22f,.58f,.38f,.25f,MaxLipShape::E},
 {MaxLipShape::E,"E","e",.22f,1.f,.50f,.15f,MaxLipShape::E},
 {MaxLipShape::F,"F","f",.10f,.55f,.70f,0,MaxLipShape::S},
 {MaxLipShape::G,"G","g",.60f,.85f,.45f,.10f,MaxLipShape::A},
 {MaxLipShape::L,"L","l",.27f,.80f,.35f,.55f,MaxLipShape::Idle},
 {MaxLipShape::N,"N","n",.65f,.35f,0,.80f,MaxLipShape::Idle},
 {MaxLipShape::OO,"OO","oo",.48f,0,0,.25f,MaxLipShape::OO},
 {MaxLipShape::Q,"Q","q",1.2f,.10f,.05f,.12f,MaxLipShape::OO},
 {MaxLipShape::S,"S","s",.12f,.88f,.80f,0,MaxLipShape::S},
 {MaxLipShape::T,"T","t",.25f,.92f,.50f,.50f,MaxLipShape::Idle},
 {MaxLipShape::U,"U","u",.63f,.90f,.50f,.30f,MaxLipShape::A}
}};
inline int MaxMouthIndex(MaxLipShape shape){for(int i=0;i<13;++i)if(MaxMouthDefinitions[i].shape==shape)return i;return -1;}
// Return -2 for idle and -1 for unrecognised files. Aliases keep older packs useful.
inline int MaxMouthFileSlot(std::string stem){for(char& c:stem)c=char(std::tolower(static_cast<unsigned char>(c)));if(stem.rfind("mouth_",0)==0)stem.erase(0,6);if(stem=="idle"||stem=="neutral")return -2;if(stem=="p"||stem=="pb"||stem=="p_b")stem="b";if(stem=="w")stem="oo";for(int i=0;i<13;++i)if(stem==MaxMouthDefinitions[i].stem)return i;return -1;}
struct MaxMouthTarget {MaxMouthSample pose;bool captured=false,enabled=true;MaxLipShape family=MaxLipShape::Idle;};
struct MaxMouthBank {
 std::array<MaxMouthTarget,13> targets{};
 MaxMouthBank(){for(int i=0;i<13;++i)targets[i].family=MaxMouthDefinitions[i].family;}
 MaxMouthSample Pose(int index,const MaxMouthCalibration& c)const{
  const auto& t=targets[index];if(t.captured&&t.pose.Usable())return t.pose;
  const auto& d=MaxMouthDefinitions[index];float span=std::max(.08f,c.poses[1].open-c.poses[0].open);
  return {c.poses[0].open+d.opening*span,c.poses[3].width+d.width*std::max(.12f,c.poses[2].width-c.poses[3].width),true,d.teeth,d.tongue,true};
 }
 void Sanitize(){for(int i=0;i<13;++i){auto& t=targets[i];if(t.captured&&!t.pose.Usable())t.captured=false;if(t.family!=MaxLipShape::Idle&&t.family!=MaxLipShape::A&&t.family!=MaxLipShape::E&&t.family!=MaxLipShape::S&&t.family!=MaxLipShape::PB&&t.family!=MaxLipShape::OO)t.family=MaxMouthDefinitions[i].family;}}
};
class MaxMouthBankSelector {
 MaxLipSelector fallback;MaxLipShape current=MaxLipShape::Idle,pending=MaxLipShape::Idle;float age=0;bool primed=false,wasCamera=false;
public:
 void Reset(){fallback.Reset();current=pending=MaxLipShape::Idle;age=0;primed=false;wasCamera=false;}
 MaxLipResult Update(MaxLipSource source,MaxMouthSample m,const MaxMouthCalibration& c,const MaxMouthBank& bank,unsigned available,
                     MaxLipShape audio,bool speech,float amount,float dt,float hold){
  bool camera=source!=MaxLipSource::Audio&&m.Usable();float span=std::max(.08f,c.poses[1].open-c.poses[0].open),ws=std::max(.12f,c.poses[2].width-c.poses[3].width);
  bool closed=camera&&m.open<=c.poses[0].open+std::max(.012f,span*.055f)+(current==MaxLipShape::Idle||current==MaxLipShape::PB?.004f:0);
  MaxLipShape candidate=MaxLipShape::Idle;
  if(closed)candidate=source==MaxLipSource::Combined&&speech&&(available&(1u<<1))&&bank.targets[1].enabled?MaxLipShape::PB:MaxLipShape::Idle;
  else if(camera||speech){
   float best=1e20f;int selected=-1;
   for(int i=0;i<13;++i){const auto& t=bank.targets[i];if(!(available&(1u<<i))||!t.enabled||MaxMouthDefinitions[i].shape==MaxLipShape::PB)continue;
    auto p=bank.Pose(i,c);float score=0;
    if(camera){float o=(m.open-p.open)/span,w=(m.width-p.width)/ws;score=o*o+w*w;
     if(m.details&&p.details){float teeth=m.teeth-p.teeth,tongue=m.tongue-p.tongue;score+=.5f*teeth*teeth+.65f*tongue*tongue;}
     if(source==MaxLipSource::Combined&&speech&&t.family==audio)score-=.045f;
    }else{
     if(t.family!=audio)continue;float o=c.Openness(p)-std::clamp(amount,0.f,1.f);score=o*o;
     // Prefer the existing family image when opening scores are essentially tied.
     if(MaxMouthDefinitions[i].shape==audio)score-=.01f;
    }
    if(score<best){best=score;selected=i;}
   }
   if(!camera&&audio==MaxLipShape::PB)candidate=(available&(1u<<1))&&bank.targets[1].enabled?MaxLipShape::PB:MaxLipShape::Idle;
   else if(selected>=0)candidate=MaxMouthDefinitions[selected].shape;
   else{Reset();if(camera)return {MaxLipShape::Idle,false,true,0};return fallback.Update(source,m,c,audio,speech,amount,dt,hold);}
  }
  dt=std::isfinite(dt)?std::clamp(dt,0.f,.1f):0;hold=std::isfinite(hold)?std::clamp(hold,0.f,.15f):.04f;
  if(!primed||camera!=wasCamera||closed||!speech&&!camera||hold==0){current=pending=candidate;age=0;primed=true;}
  else if(candidate==current){pending=current;age=0;}
  else{if(candidate!=pending){pending=candidate;age=0;}age+=dt;if(age>=hold){current=candidate;age=0;}}
  wasCamera=camera;bool talking=camera?current!=MaxLipShape::Idle:speech;
  return {current,talking,camera,camera?(talking?std::max(.1f,c.Openness(m)):0):amount};
 }
};
