// Tests use OBS's actual capture library; the app itself does not depend on OBS.
#include <windows.h>
#include <dshowcapture.hpp>
#include "../RahiTuber/CameraFrame.h"
#include <atomic>
#include <thread>
#include <cstdio>
#include <cwchar>
#include <cstdlib>
#include <cstring>
static void check(bool ok,const char* what){if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
int main(){check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM startup");
 DShow::SetLogCallback([](DShow::LogType,const wchar_t* text,void*){std::fwprintf(stderr,L"OBS capture: %ls\n",text);},nullptr);
 std::vector<DShow::VideoDevice> devices;check(DShow::Device::EnumVideoDevices(devices),"OBS capture library device enumeration");DShow::VideoDevice camera;bool found=false;
 for(const auto& device:devices)if(device.name==L"PNGTuber Max Camera"){camera=device;found=true;break;}
 check(found&&!camera.caps.empty(),"PNGTuber Max appears in OBS capture library with video capabilities");
 HANDLE map=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,64+MaxCameraFrame::Bytes,L"Local\\PNGTuberMaxFrames");check(map!=nullptr,"publisher map");auto bytes=(unsigned char*)MapViewOfFile(map,FILE_MAP_WRITE,0,0,64+MaxCameraFrame::Bytes);check(bytes!=nullptr,"publisher view");HANDLE mutex=CreateMutexW(nullptr,FALSE,L"Local\\PNGTuberMaxFrameLock");check(mutex!=nullptr,"publisher mutex");
 std::atomic<bool> stop{false};std::thread producer([&]{while(!stop){WaitForSingleObject(mutex,INFINITE);for(int i=64;i<64+MaxCameraFrame::Bytes;i+=4){bytes[i]=19;bytes[i+1]=77;bytes[i+2]=211;bytes[i+3]=255;}FILETIME time;GetSystemTimeAsFileTime(&time);ULARGE_INTEGER ticks;ticks.LowPart=time.dwLowDateTime;ticks.HighPart=time.dwHighDateTime;MaxCameraFrame::Header header;header.active=1;header.utcTicks=ticks.QuadPart+504911232000000000LL;std::memcpy(bytes,&header,64);ReleaseMutex(mutex);Sleep(25);}});
 Sleep(50);std::atomic<int> frames{0},patterns{0};
 for(int cycle=0;cycle<3;++cycle){DShow::Device receiver(DShow::InitGraph::True);check(receiver.Valid(),"OBS receiver graph");DShow::VideoConfig config;config.name=camera.name;config.path=camera.path;config.callback=[&](const DShow::VideoConfig&,unsigned char* data,size_t size,long long,long long,long){if(size==MaxCameraFrame::Bytes){++frames;if(data[0]==19&&data[1]==77&&data[2]==211)++patterns;}};
 check(receiver.SetVideoConfig(&config),"OBS receiver default format negotiation");check(receiver.ConnectFilters(),"OBS receiver connection");int before=patterns;check(receiver.Start()==DShow::Result::Success,"OBS receiver start");Sleep(1000);receiver.Stop();check(patterns-before>=20,"OBS receiver receives publisher pixels after restart");}
 stop=true;producer.join();UnmapViewOfFile(bytes);CloseHandle(map);CloseHandle(mutex);CoUninitialize();std::printf("PASS: OBS capture library discovery and three receiving sessions; %d frames / %d publisher-pattern frames.\n",int(frames),int(patterns));return 0;}
