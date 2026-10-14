#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// Values are ratios to the eye span, measured in the face's 3D landmark space.
struct MaxMouthSample {
    float open=0, width=.7f;
    bool valid=false;
    bool Usable() const { return valid && std::isfinite(open) && std::isfinite(width) && open>=0 && open<=1.5f && width>=.1f && width<=2.f; }
};
enum class MaxLipSource { Audio=0, Camera=1, Combined=2 };
// Sprite mask values match the app's five existing phoneme slots.
enum class MaxLipShape { Idle=0, A=1, E=2, S=4, PB=8, OO=16 };
inline const char* MaxLipName(MaxLipShape shape) {
    switch(shape){case MaxLipShape::A:return "A";case MaxLipShape::E:return "E";case MaxLipShape::S:return "S";case MaxLipShape::PB:return "P/B";case MaxLipShape::OO:return "OO";default:return "Idle";}
}
struct MaxMouthCalibration {
    // Closed, tall/open A, wide E, rounded OO. S is a narrow neutral opening.
    std::array<MaxMouthSample,4> poses{{{.01f,.70f,true},{.30f,.68f,true},{.10f,.90f,true},{.15f,.40f,true}}};
    unsigned captured=0;
    void Sanitize(){MaxMouthCalibration defaults;captured&=15;for(int i=0;i<4;++i){if(!poses[i].Usable()){poses[i]=defaults.poses[i];captured&=~(1u<<i);}}}
    float Openness(MaxMouthSample m)const{return std::clamp((m.open-poses[0].open)/std::max(.08f,poses[1].open-poses[0].open),0.f,1.f);}
};
struct MaxLipResult { MaxLipShape shape=MaxLipShape::Idle;bool talking=false, camera=false;float amount=0; };
class MaxLipSelector {
    MaxLipShape current=MaxLipShape::Idle,pending=MaxLipShape::Idle;
    float age=0;bool primed=false;
public:
    void Reset(){current=pending=MaxLipShape::Idle;age=0;primed=false;}
    MaxLipResult Update(MaxLipSource source,MaxMouthSample mouth,const MaxMouthCalibration& calibration,
                        MaxLipShape audio,bool audioTalking,float audioAmount,float dt,float hold=.04f){
        if(source==MaxLipSource::Audio||!mouth.Usable()){
            Reset();return {audioTalking?audio:MaxLipShape::Idle,audioTalking,false,audioAmount};
        }
        const auto& p=calibration.poses;
        float span=std::max(.08f,p[1].open-p[0].open), widthSpan=std::max(.12f,p[2].width-p[3].width);
        // Closing lips is immediate. Reopening has a small dead band.
        float closed=p[0].open+std::max(.012f,span*.055f);
        bool closing=mouth.open<=closed+(current==MaxLipShape::Idle||current==MaxLipShape::PB?.004f:0.f);
        MaxLipShape candidate=MaxLipShape::Idle;
        if(closing){candidate=source==MaxLipSource::Combined&&audioTalking?MaxLipShape::PB:MaxLipShape::Idle;}
        else{
            std::array<MaxMouthSample,4> shapes{{p[1],p[2],{p[0].open+span*.16f,p[0].width,true},p[3]}};
            const MaxLipShape labels[]={MaxLipShape::A,MaxLipShape::E,MaxLipShape::S,MaxLipShape::OO};
            std::array<float,4> scores{};int best=0;
            for(int i=0;i<4;++i){float o=(mouth.open-shapes[i].open)/span,w=(mouth.width-shapes[i].width)/widthSpan;scores[i]=o*o+w*w;if(scores[i]<scores[best])best=i;}
            // Audio breaks close E/S ties only; it cannot override a clearly
            // rounded, tall, or closed mouth or a camera-only expression.
            if(source==MaxLipSource::Combined&&audioTalking&&(audio==MaxLipShape::E||audio==MaxLipShape::S)){
                int hint=audio==MaxLipShape::E?1:2;
                if((best==1||best==2)&&scores[hint]-scores[best]<.08f)best=hint;
            }
            candidate=labels[best];
        }
        dt=std::isfinite(dt)?std::clamp(dt,0.f,.1f):0.f;hold=std::isfinite(hold)?std::clamp(hold,0.f,.15f):.04f;
        if(!primed||closing||hold==0){current=pending=candidate;age=0;primed=true;}
        else if(candidate==current){pending=current;age=0;}
        else{if(candidate!=pending){pending=candidate;age=0;}age+=dt;if(age>=hold){current=candidate;age=0;}}
        bool talking=current!=MaxLipShape::Idle;
        return {current,talking,true,talking?std::max(.1f,calibration.Openness(mouth)):0.f};
    }
};
// Average several fresh camera frames; repeated render frames never count twice.
struct MaxMouthCapture {
    int slot=-1,count=0;float elapsed=0,open=0,width=0;bool failed=false;
    void Start(int index){slot=index;count=0;elapsed=open=width=0;failed=false;}
    bool Feed(MaxMouthSample sample,bool fresh,float dt,MaxMouthCalibration& calibration){
        if(slot<0)return false;elapsed+=std::clamp(dt,0.f,.1f);
        if(fresh&&sample.Usable()){open+=sample.open;width+=sample.width;++count;}
        if(elapsed<.6f)return false;
        failed=count<5;
        if(!failed){calibration.poses[slot]={open/count,width/count,true};calibration.captured|=1u<<slot;}
        slot=-1;return !failed;
    }
};
