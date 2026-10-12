// Exercise the unmanaged recorder ABI, including caller-owned format buffers.
#include <windows.h>
#include <dshow.h>
#include <cstdio>
#include <cstdlib>
static void check(bool ok,const char* what){if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
static void freeType(AM_MEDIA_TYPE* mt){if(!mt)return;CoTaskMemFree(mt->pbFormat);if(mt->pUnk)mt->pUnk->Release();CoTaskMemFree(mt);}
int main(){check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM initialization");
 CLSID id;CLSIDFromString(L"{EEBEE603-EC24-4D77-8827-C8F799C81026}",&id);
 for(int cycle=0;cycle<20;++cycle){IBaseFilter* filter=nullptr;check(SUCCEEDED(CoCreateInstance(id,nullptr,CLSCTX_INPROC_SERVER,IID_IBaseFilter,(void**)&filter)),"create installed camera");
 IEnumPins* pins=nullptr;check(SUCCEEDED(filter->EnumPins(&pins)),"enumerate pins");IPin* pin=nullptr;ULONG fetched=0;check(pins->Next(1,&pin,&fetched)==S_OK&&fetched==1,"native pin enumeration");pins->Release();
 PIN_INFO info={};check(SUCCEEDED(pin->QueryPinInfo(&info))&&info.dir==PINDIR_OUTPUT&&info.pFilter,"pin info ABI");info.pFilter->Release();
 IAMStreamConfig* cfg=nullptr;check(SUCCEEDED(pin->QueryInterface(IID_IAMStreamConfig,(void**)&cfg)),"stream configuration");int count=0,size=0;check(SUCCEEDED(cfg->GetNumberOfCapabilities(&count,&size))&&count==1&&size==sizeof(VIDEO_STREAM_CONFIG_CAPS),"capability struct size");
 VIDEO_STREAM_CONFIG_CAPS caps={};AM_MEDIA_TYPE* mt=nullptr;
 check(cfg->GetStreamCaps(0,nullptr,(BYTE*)&caps)==E_POINTER,"null type output");
 check(cfg->GetStreamCaps(0,&mt,nullptr)==E_POINTER&&mt==nullptr,"null capability output");
 check(SUCCEEDED(cfg->GetStreamCaps(0,&mt,(BYTE*)&caps))&&mt&&mt->cbFormat>=sizeof(VIDEOINFOHEADER),"native capability allocation");
 auto video=(VIDEOINFOHEADER*)mt->pbFormat;check(video->bmiHeader.biWidth==640&&video->bmiHeader.biHeight==480&&video->bmiHeader.biBitCount==32,"RGB32 format ABI");check(caps.InputSize.cx==640&&caps.MinFrameInterval==333333,"capability fields");
 check(SUCCEEDED(cfg->SetFormat(mt)),"set advertised format");freeType(mt);mt=nullptr;
 check(SUCCEEDED(cfg->SetFormat(nullptr)),"restore default format");check(cfg->GetFormat(nullptr)==E_POINTER,"null format output");check(SUCCEEDED(cfg->GetFormat(&mt))&&mt,"native format allocation");freeType(mt);mt=nullptr;
 check(FAILED(cfg->GetStreamCaps(1,&mt,(BYTE*)&caps))&&mt==nullptr,"invalid capability index");
 IEnumMediaTypes* types=nullptr;check(SUCCEEDED(pin->EnumMediaTypes(&types)),"enumerate media types");check(types->Next(1,&mt,&fetched)==S_OK&&mt,"native type enumeration");freeType(mt);types->Release();
 check(SUCCEEDED(filter->Pause())&&SUCCEEDED(filter->Stop()),"unconnected state cycle");cfg->Release();pin->Release();filter->Release();}
 CoUninitialize();std::puts("Native recorder negotiation passed (20 device-selection cycles).");return 0;}
