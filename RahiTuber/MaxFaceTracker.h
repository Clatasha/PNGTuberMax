#pragma once
#include "MaxRig.h"
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <array>
#include <string>
#include <cstring>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <dshow.h>
#endif
class MaxFaceTracker {
public:
 std::vector<std::pair<int,std::string>> devices;bool devicesScanned=false;
 void RefreshDevices(){devices.clear();devicesScanned=true;
#ifdef _WIN32
 HRESULT initialized=CoInitializeEx(nullptr,COINIT_MULTITHREADED);ICreateDevEnum* enumeration=nullptr;IEnumMoniker* monikers=nullptr;
 if(SUCCEEDED(CoCreateInstance(CLSID_SystemDeviceEnum,nullptr,CLSCTX_INPROC_SERVER,IID_ICreateDevEnum,reinterpret_cast<void**>(&enumeration)))){
 if(enumeration->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&monikers,0)==S_OK){IMoniker* moniker=nullptr;int index=0;while(monikers->Next(1,&moniker,nullptr)==S_OK){IPropertyBag* bag=nullptr;VARIANT name;VariantInit(&name);std::string label="Camera "+std::to_string(index);
 if(SUCCEEDED(moniker->BindToStorage(nullptr,nullptr,IID_IPropertyBag,reinterpret_cast<void**>(&bag)))){if(SUCCEEDED(bag->Read(L"FriendlyName",&name,nullptr))&&name.vt==VT_BSTR){int size=WideCharToMultiByte(CP_UTF8,0,name.bstrVal,-1,nullptr,0,nullptr,nullptr);if(size>0){label.resize(size);WideCharToMultiByte(CP_UTF8,0,name.bstrVal,-1,label.data(),size,nullptr,nullptr);label.pop_back();}}VariantClear(&name);bag->Release();}
 if(label.find("PNGTuber Max")==std::string::npos&&label.find("Clatasha Camera")==std::string::npos)devices.emplace_back(index,label);++index;moniker->Release();}monikers->Release();}enumeration->Release();}
 if(SUCCEEDED(initialized))CoUninitialize();
 if(!devices.empty()&&std::none_of(devices.begin(),devices.end(),[&](const auto& d){return d.first==device;}))device=devices.front().first;
#endif
 }
 MaxRigPose pose;std::array<sf::Texture,3> textures;bool available=false;int device=0;float smoothing=.15f;std::string status="Webcam stopped";
 ~MaxFaceTracker(){Stop();}
 bool Running()const{return running;}
 void Calibrate(){neutral=raw;pose={};}
 void Start(const std::string& appLocation){
 Stop();
#ifdef _WIN32
 auto root=std::filesystem::path(appLocation)/"tracker";auto exe=root/"pythonw.exe",script=root/"tracker.py";
 if(!std::filesystem::exists(exe)){status="Bundled tracker is missing. Extract the complete rig update into the app folder.";return;}
 name=L"Local\\PNGTuberMaxRig"+std::to_wstring(GetCurrentProcessId());
 lock=CreateMutexW(nullptr,FALSE,(name+L"Lock").c_str());map=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,bytes,name.c_str());
 data=map?static_cast<unsigned char*>(MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,bytes)):nullptr;
 if(!lock||!data){Stop();status="Unable to create tracking buffer";return;}
 std::memset(data,0,bytes);*reinterpret_cast<unsigned*>(data+28)=1;
 std::wstring command=L"\""+exe.wstring()+L"\" \""+script.wstring()+L"\" --name \""+name+L"\" --camera "+std::to_wstring(device);
 STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
 if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,root.c_str(),&si,&pi)){Stop();status="Could not start bundled tracker";return;}
 process=pi.hProcess;CloseHandle(pi.hThread);
 job=CreateJobObjectW(nullptr,nullptr);if(job){JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits));AssignProcessToJobObject(job,process);}running=true;status="Starting webcam...";clock.restart();
#else
 status="Webcam tracking is available in the Windows build";
#endif
 }
 void Stop(){
#ifdef _WIN32
 if(data){if(lock&&WaitForSingleObject(lock,200)==WAIT_OBJECT_0){*reinterpret_cast<unsigned*>(data+28)=0;ReleaseMutex(lock);}}
 if(process){if(WaitForSingleObject(process,1000)==WAIT_TIMEOUT)TerminateProcess(process,0);CloseHandle(process);process=nullptr;}
 if(job){CloseHandle(job);job=nullptr;}
 if(data){UnmapViewOfFile(data);data=nullptr;}if(map){CloseHandle(map);map=nullptr;}if(lock){CloseHandle(lock);lock=nullptr;}
#endif
 running=false;available=false;pose={};raw={};neutral={};sequence=0;status="Webcam stopped";
 }
 void Poll(){
 float dt=std::min(.1f,clock.restart().asSeconds());MaxRigPose target{};
#ifdef _WIN32
 if(running&&process&&WaitForSingleObject(process,0)==WAIT_OBJECT_0){running=false;available=false;status="Tracker stopped. Check webcam access in Windows Settings or choose another camera.";}
 if(running&&data&&WaitForSingleObject(lock,0)==WAIT_OBJECT_0){
 unsigned header[4];std::memcpy(header,data,16);
 if(header[0]==0x52494731&&header[1]!=sequence){sequence=header[1];fresh.restart();available=header[2]==1;
 if(available){std::memcpy(&raw,data+16,12);for(int i=0;i<3;++i){if(textures[i].getSize()!=sf::Vector2u(128,96)){textures[i].create(128,96);textures[i].setSmooth(true);}textures[i].update(data+64+i*128*96*4);}status="Face tracked";}else status=header[2]==3?"Camera unavailable. Close another app using it, or choose a different input.":"Looking for your face";}
 ReleaseMutex(lock);if(fresh.getElapsedTime().asSeconds()>1)available=false;
 }
#endif
 if(available)target={std::clamp(raw.lean-neutral.lean,-35.f,35.f),std::clamp(raw.nod-neutral.nod,-35.f,35.f),std::clamp(raw.turn-neutral.turn,-50.f,50.f)};
 float a=1-std::exp(-dt/std::max(.02f,smoothing));pose.lean+=(target.lean-pose.lean)*a;pose.nod+=(target.nod-pose.nod)*a;pose.turn+=(target.turn-pose.turn)*a;
 }
private:
 static constexpr unsigned bytes=64+3*128*96*4;bool running=false;MaxRigPose raw,neutral;unsigned sequence=0;sf::Clock clock,fresh;
#ifdef _WIN32
 HANDLE lock=nullptr,map=nullptr,process=nullptr,job=nullptr;unsigned char* data=nullptr;std::wstring name;
#endif
};
