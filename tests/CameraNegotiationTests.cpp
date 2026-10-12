// Exercise the unmanaged recorder ABI, including caller-owned format buffers.
#include <windows.h>
#include <dshow.h>
#include <cstdio>
#include <cstdlib>
struct __declspec(uuid("31efac30-515c-11d0-a9aa-00aa0061be93")) MaxKsProperty: IUnknown {
 virtual HRESULT STDMETHODCALLTYPE Set(REFGUID,DWORD,void*,DWORD,void*,DWORD)=0;
 virtual HRESULT STDMETHODCALLTYPE Get(REFGUID,DWORD,void*,DWORD,void*,DWORD,DWORD*)=0;
 virtual HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID,DWORD,DWORD*)=0;
};
static void check(bool ok,const char* what){if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
static void freeType(AM_MEDIA_TYPE* mt){if(!mt)return;CoTaskMemFree(mt->pbFormat);if(mt->pUnk)mt->pUnk->Release();CoTaskMemFree(mt);}
int main(){check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM initialization");
 CLSID id;CLSIDFromString(L"{EEBEE603-EC24-4D77-8827-C8F799C81026}",&id);
 ICreateDevEnum* devices=nullptr;IEnumMoniker* monikers=nullptr;IMoniker* cameraMoniker=nullptr;
 check(SUCCEEDED(CoCreateInstance(CLSID_SystemDeviceEnum,nullptr,CLSCTX_INPROC_SERVER,IID_ICreateDevEnum,(void**)&devices)),"system device enumeration");
 check(devices->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&monikers,0)==S_OK,"camera category enumeration");IMoniker* current=nullptr;
 while(monikers->Next(1,&current,nullptr)==S_OK){IPropertyBag* bag=nullptr;VARIANT name;VariantInit(&name);if(SUCCEEDED(current->BindToStorage(nullptr,nullptr,IID_IPropertyBag,(void**)&bag))){if(SUCCEEDED(bag->Read(L"FriendlyName",&name,nullptr))&&name.vt==VT_BSTR&&wcscmp(name.bstrVal,L"PNGTuber Max Camera")==0)cameraMoniker=current;VariantClear(&name);bag->Release();}if(cameraMoniker)break;current->Release();}
 monikers->Release();devices->Release();check(cameraMoniker!=nullptr,"installed camera name in device list");
 for(int cycle=0;cycle<20;++cycle){IBaseFilter* filter=nullptr;check(SUCCEEDED(cameraMoniker->BindToObject(nullptr,nullptr,IID_IBaseFilter,(void**)&filter)),"bind installed camera through device moniker (OBS discovery path)");
 IEnumPins* pins=nullptr;check(SUCCEEDED(filter->EnumPins(&pins)),"enumerate pins");IPin* pin=nullptr;ULONG fetched=0;check(pins->Next(1,&pin,&fetched)==S_OK&&fetched==1,"native pin enumeration");pins->Release();
 PIN_INFO info={};check(SUCCEEDED(pin->QueryPinInfo(&info))&&info.dir==PINDIR_OUTPUT&&info.pFilter,"pin info ABI");info.pFilter->Release();
 MaxKsProperty* property=nullptr;check(SUCCEEDED(pin->QueryInterface(__uuidof(MaxKsProperty),(void**)&property)),"capture pin property interface");GUID set,category,expected;CLSIDFromString(L"{9b00f101-1567-11d1-b3f1-00aa003761c5}",&set);CLSIDFromString(L"{fb6c4281-0353-11d1-905f-0000c0cc16ba}",&expected);DWORD support=0,returned=0;
 check(SUCCEEDED(property->QuerySupported(set,0,&support))&&(support&1),"native category query support");check(SUCCEEDED(property->Get(set,0,nullptr,0,&category,sizeof(category),&returned))&&returned==16&&category==expected,"native capture category");property->Release();
 IAMStreamConfig* cfg=nullptr;check(SUCCEEDED(pin->QueryInterface(IID_IAMStreamConfig,(void**)&cfg)),"stream configuration");int count=0,size=0;check(SUCCEEDED(cfg->GetNumberOfCapabilities(&count,&size))&&count>=1&&size==sizeof(VIDEO_STREAM_CONFIG_CAPS),"capability struct size");
 VIDEO_STREAM_CONFIG_CAPS caps={};AM_MEDIA_TYPE* mt=nullptr;
 check(cfg->GetStreamCaps(0,nullptr,(BYTE*)&caps)==E_POINTER,"null type output");
 check(cfg->GetStreamCaps(0,&mt,nullptr)==E_POINTER&&mt==nullptr,"null capability output");
 check(SUCCEEDED(cfg->GetStreamCaps(0,&mt,(BYTE*)&caps))&&mt&&mt->cbFormat>=sizeof(VIDEOINFOHEADER),"native capability allocation");
 auto video=(VIDEOINFOHEADER*)mt->pbFormat;check(video->bmiHeader.biWidth==640&&video->bmiHeader.biHeight==480&&video->bmiHeader.biBitCount==32,"RGB32 format ABI");check(caps.InputSize.cx==640&&caps.MinFrameInterval==333333,"capability fields");
 check(SUCCEEDED(cfg->SetFormat(mt)),"set advertised format");freeType(mt);mt=nullptr;
 check(SUCCEEDED(cfg->SetFormat(nullptr)),"restore default format");check(cfg->GetFormat(nullptr)==E_POINTER,"null format output");check(SUCCEEDED(cfg->GetFormat(&mt))&&mt,"native format allocation");freeType(mt);mt=nullptr;
 check(FAILED(cfg->GetStreamCaps(count,&mt,(BYTE*)&caps))&&mt==nullptr,"invalid capability index");
 if(count>1){check(SUCCEEDED(cfg->GetStreamCaps(1,&mt,(BYTE*)&caps))&&mt,"secondary RGB24 capability");check(SUCCEEDED(cfg->SetFormat(mt)),"select RGB24");freeType(mt);mt=nullptr;check(SUCCEEDED(cfg->GetFormat(&mt))&&((VIDEOINFOHEADER*)mt->pbFormat)->bmiHeader.biBitCount==24,"selected format is retained");freeType(mt);mt=nullptr;check(SUCCEEDED(cfg->SetFormat(nullptr)),"reset RGB32 after RGB24");}
 IEnumMediaTypes* types=nullptr;check(SUCCEEDED(pin->EnumMediaTypes(&types)),"enumerate media types");check(types->Next(1,&mt,&fetched)==S_OK&&mt,"native type enumeration");freeType(mt);types->Release();
 check(SUCCEEDED(filter->Pause())&&SUCCEEDED(filter->Stop()),"unconnected state cycle");cfg->Release();
 if(cycle<3){IGraphBuilder* graph=nullptr;IBaseFilter* sink=nullptr;IMediaControl* control=nullptr;
 check(SUCCEEDED(CoCreateInstance(CLSID_FilterGraph,nullptr,CLSCTX_INPROC_SERVER,IID_IGraphBuilder,(void**)&graph)),"native graph");
 CLSID nullRenderer;CLSIDFromString(L"{C1F400A4-3F08-11D3-9F0B-006008039E37}",&nullRenderer);
 check(SUCCEEDED(CoCreateInstance(nullRenderer,nullptr,CLSCTX_INPROC_SERVER,IID_IBaseFilter,(void**)&sink)),"null receiver");
 check(SUCCEEDED(graph->AddFilter(filter,L"Max Camera"))&&SUCCEEDED(graph->AddFilter(sink,L"Receiver")),"add native filters");
 IEnumPins* sinkPins=nullptr;IPin* sinkPin=nullptr;sink->EnumPins(&sinkPins);check(sinkPins->Next(1,&sinkPin,nullptr)==S_OK,"receiver pin");sinkPins->Release();
 check(SUCCEEDED(graph->ConnectDirect(pin,sinkPin,nullptr)),"native stream connection");graph->QueryInterface(IID_IMediaControl,(void**)&control);
 for(int restart=0;restart<3;++restart){check(SUCCEEDED(control->Run()),"native receiver run");Sleep(200);check(SUCCEEDED(control->Stop()),"native receiver stop");}
 check(SUCCEEDED(graph->Disconnect(pin))&&SUCCEEDED(graph->Disconnect(sinkPin)),"native disconnect");
 control->Release();sinkPin->Release();graph->RemoveFilter(filter);graph->RemoveFilter(sink);sink->Release();graph->Release();}
 pin->Release();filter->Release();}
 check(GetModuleHandleW(L"clr.dll")==nullptr,"camera does not load CLR into native receiver");cameraMoniker->Release();CoUninitialize();std::puts("Native recorder negotiation passed (20 device-selection cycles).");return 0;}
