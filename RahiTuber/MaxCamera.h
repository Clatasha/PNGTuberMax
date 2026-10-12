// Native camera publisher for PNGTuber Max, by Clatasha.
#pragma once
#include "CameraFrame.h"
#include "SFML/Graphics.hpp"
#include <string>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#endif
class MaxCamera {
    bool on_=false;std::string status_="Camera output off";sf::RenderTexture output_;sf::Clock clock_;MaxCameraFrame::Cadence cadence_;
#ifdef _WIN32
    HANDLE setup_=nullptr;std::string resultPath_;
    HANDLE map_=nullptr,lock_=nullptr,owner_=nullptr;unsigned char* memory_=nullptr;
#endif
public:
    MaxCamera()=default;MaxCamera(const MaxCamera&)=delete;MaxCamera& operator=(const MaxCamera&)=delete;
    ~MaxCamera(){Stop();
#ifdef _WIN32
if(setup_)CloseHandle(setup_);
#endif
}
    bool On() const {return on_;}const std::string& Status() const{return status_;}
    bool Installed() const {
#ifdef _WIN32
        HKEY key=nullptr;LONG result=RegOpenKeyExA(HKEY_CLASSES_ROOT,"CLSID\\{EEBEE603-EC24-4D77-8827-C8F799C81026}\\InprocServer32",0,KEY_READ,&key);if(key)RegCloseKey(key);return result==ERROR_SUCCESS;
#else
        return false;
#endif
    }
    void Poll() {
#ifdef _WIN32
        if(setup_&&WaitForSingleObject(setup_,0)==WAIT_OBJECT_0){DWORD code=0;GetExitCodeProcess(setup_,&code);CloseHandle(setup_);setup_=nullptr;std::ifstream result(resultPath_);std::stringstream text;text<<result.rdbuf();status_=text.str().empty()?(code==0?"Camera setup completed":"Setup failed. Check camera-install-log.txt in Local AppData/Clatasha/PNGTuberMax"):text.str();}
#endif
    }
    void Setup(const std::string& directory) {
#ifdef _WIN32
        if(setup_){status_="Camera setup is already running";return;}
        char appData[MAX_PATH]={};GetEnvironmentVariableA("LOCALAPPDATA",appData,MAX_PATH);std::string parent=std::string(appData)+"\\Clatasha";CreateDirectoryA(parent.c_str(),nullptr);parent+="\\PNGTuberMax";CreateDirectoryA(parent.c_str(),nullptr);resultPath_=parent+"\\camera-setup-result.txt";DeleteFileA(resultPath_.c_str());
        std::string parameters="-NoProfile -NonInteractive -WindowStyle Hidden -ExecutionPolicy Bypass -File \""+directory+"Install-Camera.ps1\" -ResultPath \""+resultPath_+"\"";
        SHELLEXECUTEINFOA launch={};launch.cbSize=sizeof launch;launch.fMask=SEE_MASK_NOCLOSEPROCESS;launch.lpVerb="runas";launch.lpFile="powershell.exe";launch.lpParameters=parameters.c_str();launch.lpDirectory=directory.c_str();launch.nShow=SW_HIDE;
        if(ShellExecuteExA(&launch)){setup_=launch.hProcess;status_="Installing / repairing camera...";}else status_="Camera setup was canceled or could not open";
#else
        status_="Virtual camera is supported on Windows in this preview";
#endif
    }
    void Check(const std::string& directory){
#ifdef _WIN32
        if(!Installed()){status_="Install the camera before checking delivery";return;}if(!on_&&!Start())return;auto result=ShellExecuteA(nullptr,"open",(directory+"CameraDiagnostic.exe").c_str(),nullptr,directory.c_str(),SW_SHOW);if(reinterpret_cast<INT_PTR>(result)<=32)status_="Camera checker is missing. Extract the complete application folder.";
#endif
    }
    bool Start() {
#ifdef _WIN32
        if(on_)return true;if(!Installed()){status_="Install the camera first";return false;}
        owner_=CreateMutexA(nullptr,TRUE,"Local\\PNGTuberMaxProducer");if(!owner_||GetLastError()==ERROR_ALREADY_EXISTS){if(owner_)CloseHandle(owner_);owner_=nullptr;status_="Another PNGTuber Max window is publishing the camera";return false;}
        map_=CreateFileMappingA(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,64+MaxCameraFrame::Bytes,"Local\\PNGTuberMaxFrames");
        if(map_)memory_=static_cast<unsigned char*>(MapViewOfFile(map_,FILE_MAP_WRITE,0,0,64+MaxCameraFrame::Bytes));
        lock_=CreateMutexA(nullptr,FALSE,"Local\\PNGTuberMaxFrameLock");
        if(!memory_||!lock_||!output_.create(MaxCameraFrame::Width,MaxCameraFrame::Height)){Stop();status_="Could not create camera output";return false;}
        on_=true;clock_.restart();cadence_={};status_="Sending 640 x 480 / 30 FPS";return true;
#else
        status_="Virtual camera is supported on Windows in this preview";return false;
#endif
    }
    void Stop() {
#ifdef _WIN32
        if(memory_&&lock_){DWORD wait=WaitForSingleObject(lock_,10);if(wait==WAIT_OBJECT_0||wait==WAIT_ABANDONED){MaxCameraFrame::Header header;std::memcpy(memory_,&header,sizeof header);ReleaseMutex(lock_);}}
        if(memory_)UnmapViewOfFile(memory_);if(map_)CloseHandle(map_);if(lock_)CloseHandle(lock_);if(owner_){ReleaseMutex(owner_);CloseHandle(owner_);}memory_=nullptr;map_=lock_=owner_=nullptr;
#endif
        on_=false;status_="Camera output off";
    }
    void Publish(const sf::Texture& scene) {
#ifdef _WIN32
        if(!on_||!cadence_.Due(clock_.getElapsedTime().asMicroseconds()))return;auto size=scene.getSize();if(!size.x||!size.y)return;
        output_.clear(sf::Color(12,20,35));sf::Sprite sprite(scene);float scale=std::min(float(MaxCameraFrame::Width)/size.x,float(MaxCameraFrame::Height)/size.y);sprite.setScale(scale,scale);sprite.setPosition((MaxCameraFrame::Width-size.x*scale)*.5f,(MaxCameraFrame::Height-size.y*scale)*.5f);
        sf::RenderStates state;state.blendMode=sf::BlendMode(sf::BlendMode::One,sf::BlendMode::OneMinusSrcAlpha,sf::BlendMode::Add,sf::BlendMode::One,sf::BlendMode::OneMinusSrcAlpha,sf::BlendMode::Add);output_.draw(sprite,state);output_.display();auto image=output_.getTexture().copyToImage();
        if(image.getSize().x!=MaxCameraFrame::Width||image.getSize().y!=MaxCameraFrame::Height){Stop();status_="Camera frame readback failed";return;}
        DWORD wait=WaitForSingleObject(lock_,0);if(wait==WAIT_OBJECT_0||wait==WAIT_ABANDONED){MaxCameraFrame::RgbaToBgra(image.getPixelsPtr(),memory_+64,MaxCameraFrame::Width*MaxCameraFrame::Height);FILETIME time;GetSystemTimeAsFileTime(&time);ULARGE_INTEGER ticks;ticks.LowPart=time.dwLowDateTime;ticks.HighPart=time.dwHighDateTime;MaxCameraFrame::Header header;header.active=1;header.utcTicks=static_cast<std::int64_t>(ticks.QuadPart)+504911232000000000LL;std::memcpy(memory_,&header,sizeof header);ReleaseMutex(lock_);}
#endif
    }
};
