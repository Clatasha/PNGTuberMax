// PNGTuber Max native DirectShow camera, by Clatasha.
// COM and streaming lifecycle use Microsoft's MIT-licensed DirectShow base classes.
#include <streams.h>
#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include <shlobj.h>
#include <strsafe.h>
#include <new>
#include <vector>
#include <string>
#include <cstring>
#include "../../RahiTuber/CameraFrame.h"
const CLSID CLSID_MaxCamera={0xeebee603,0xec24,0x4d77,{0x88,0x27,0xc8,0xf7,0x99,0xc8,0x10,0x26}};
static constexpr REFERENCE_TIME Interval=333333;
static void Trace(const wchar_t* text){wchar_t path[MAX_PATH]={};if(FAILED(SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA,nullptr,0,path)))return;std::wstring dir=std::wstring(path)+L"\\Clatasha";CreateDirectoryW(dir.c_str(),nullptr);dir+=L"\\PNGTuberMax";CreateDirectoryW(dir.c_str(),nullptr);HANDLE file=CreateFileW((dir+L"\\camera-stream-log.txt").c_str(),FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)return;SYSTEMTIME now;GetLocalTime(&now);wchar_t host[MAX_PATH]={};GetModuleFileNameW(nullptr,host,MAX_PATH);wchar_t line[2048];StringCchPrintfW(line,2048,L"%04u-%02u-%02u %02u:%02u:%02u native %u-bit pid=%lu host=%s %s\r\n",now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond,unsigned(sizeof(void*)*8),GetCurrentProcessId(),host,text);int count=WideCharToMultiByte(CP_UTF8,0,line,-1,nullptr,0,nullptr,nullptr);std::vector<char> bytes(count);WideCharToMultiByte(CP_UTF8,0,line,-1,bytes.data(),count,nullptr,nullptr);DWORD written;WriteFile(file,bytes.data(),count-1,&written,nullptr);CloseHandle(file);}
static HRESULT MakeType(CMediaType* type,bool rgb24=false){if(!type)return E_POINTER;type->InitMediaType();auto* header=(VIDEOINFOHEADER*)type->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));if(!header)return E_OUTOFMEMORY;ZeroMemory(header,sizeof(*header));int size=MaxCameraFrame::Width*MaxCameraFrame::Height*(rgb24?3:4);header->AvgTimePerFrame=Interval;header->dwBitRate=size*8*30;header->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);header->bmiHeader.biWidth=MaxCameraFrame::Width;header->bmiHeader.biHeight=MaxCameraFrame::Height;header->bmiHeader.biPlanes=1;header->bmiHeader.biBitCount=rgb24?24:32;header->bmiHeader.biCompression=BI_RGB;header->bmiHeader.biSizeImage=size;type->SetType(&MEDIATYPE_Video);type->SetSubtype(rgb24?&MEDIASUBTYPE_RGB24:&MEDIASUBTYPE_RGB32);type->SetFormatType(&FORMAT_VideoInfo);type->SetTemporalCompression(FALSE);type->SetSampleSize(size);return S_OK;}
class MaxStream final:public CSourceStream,public IAMStreamConfig,public IKsPropertySet{
 HANDLE map_=nullptr,mutex_=nullptr;const unsigned char* view_=nullptr;REFERENCE_TIME sequence_=0;ULONGLONG origin_=0;std::vector<unsigned char> pixels_;bool configured_=false;
 void CloseMap(){if(view_)UnmapViewOfFile(view_);if(map_)CloseHandle(map_);if(mutex_)CloseHandle(mutex_);view_=nullptr;map_=mutex_=nullptr;}
 HRESULT CurrentType(CMediaType& type){if(m_mt.FormatLength())type=m_mt;else return MakeType(&type);return S_OK;}
public:
 MaxStream(HRESULT* hr,CSource* filter):CSourceStream(NAME("PNGTuber Max Output"),hr,filter,L"Output"),pixels_(MaxCameraFrame::Bytes){Trace(L"Output pin constructed");}
 ~MaxStream() override {CloseMap();}
 DECLARE_IUNKNOWN;
 STDMETHODIMP NonDelegatingQueryInterface(REFIID iid,void** out) override {if(!out)return E_POINTER;if(iid==IID_IAMStreamConfig)return GetInterface((IAMStreamConfig*)this,out);if(iid==IID_IKsPropertySet)return GetInterface((IKsPropertySet*)this,out);return CSourceStream::NonDelegatingQueryInterface(iid,out);}
 HRESULT GetMediaType(int position,CMediaType* type) override {if(!type)return E_POINTER;if(position<0)return E_INVALIDARG;if(configured_){if(position>0)return VFW_S_NO_MORE_ITEMS;*type=m_mt;return S_OK;}if(position>=2)return VFW_S_NO_MORE_ITEMS;return MakeType(type,position==1);}
 HRESULT ValidateType(const CMediaType* type) {if(!type||type->majortype!=MEDIATYPE_Video||(type->subtype!=MEDIASUBTYPE_RGB32&&type->subtype!=MEDIASUBTYPE_RGB24)||type->formattype!=FORMAT_VideoInfo||!type->pbFormat||type->cbFormat<sizeof(VIDEOINFOHEADER))return VFW_E_TYPE_NOT_ACCEPTED;auto* h=(VIDEOINFOHEADER*)type->pbFormat;bool rgb24=type->subtype==MEDIASUBTYPE_RGB24;return h->bmiHeader.biWidth==640&&h->bmiHeader.biHeight==480&&h->bmiHeader.biBitCount==(rgb24?24:32)&&h->bmiHeader.biPlanes==1&&h->bmiHeader.biCompression==BI_RGB&&h->AvgTimePerFrame==Interval?S_OK:VFW_E_TYPE_NOT_ACCEPTED;}
 HRESULT CheckMediaType(const CMediaType* type) override {HRESULT hr=ValidateType(type);if(FAILED(hr))return hr;return configured_&&type->subtype!=m_mt.subtype?VFW_E_TYPE_NOT_ACCEPTED:S_OK;}
 HRESULT DecideBufferSize(IMemAllocator* allocator,ALLOCATOR_PROPERTIES* props) override {if(!allocator||!props)return E_POINTER;props->cBuffers=3;props->cbBuffer=m_mt.GetSampleSize();props->cbAlign=4;props->cbPrefix=0;ALLOCATOR_PROPERTIES actual={};HRESULT hr=allocator->SetProperties(props,&actual);if(FAILED(hr))return hr;return actual.cbBuffer>=props->cbBuffer?S_OK:E_FAIL;}
 HRESULT OnThreadCreate() override {sequence_=0;origin_=GetTickCount64();Trace(L"Stream started");return S_OK;}
 HRESULT OnThreadDestroy() override {CloseMap();Trace(L"Stream stopped");return S_OK;}
 HRESULT FillBuffer(IMediaSample* sample) override {if(!sample)return E_POINTER;ULONGLONG due=origin_+ULONGLONG(sequence_)*1000/30;ULONGLONG now=GetTickCount64();if(now>due+100){origin_=now-ULONGLONG(sequence_)*1000/30;due=now;}if(due>now)Sleep(DWORD(due-now));
  bool fresh=false;if(!map_){map_=OpenFileMappingW(FILE_MAP_READ,FALSE,L"Local\\PNGTuberMaxFrames");if(map_)view_=(const unsigned char*)MapViewOfFile(map_,FILE_MAP_READ,0,0,64+MaxCameraFrame::Bytes);mutex_=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,L"Local\\PNGTuberMaxFrameLock");if(!view_||!mutex_)CloseMap();}
  if(view_&&mutex_){DWORD wait=WaitForSingleObject(mutex_,5);if(wait==WAIT_OBJECT_0||wait==WAIT_ABANDONED){MaxCameraFrame::Header header;std::memcpy(&header,view_,sizeof(header));FILETIME time;GetSystemTimeAsFileTime(&time);ULARGE_INTEGER ticks;ticks.LowPart=time.dwLowDateTime;ticks.HighPart=time.dwHighDateTime;LONGLONG age=LONGLONG(ticks.QuadPart)+504911232000000000LL-header.utcTicks;if(header.magic==MaxCameraFrame::Magic&&header.width==640&&header.height==480&&header.active==1&&age>=0&&age<20000000){std::memcpy(pixels_.data(),view_+64,MaxCameraFrame::Bytes);fresh=true;}ReleaseMutex(mutex_);}}
  if(!fresh)for(int i=0;i<MaxCameraFrame::Bytes;i+=4){pixels_[i]=35;pixels_[i+1]=20;pixels_[i+2]=12;pixels_[i+3]=255;}
  BYTE* out=nullptr;HRESULT hr=sample->GetPointer(&out);if(FAILED(hr)||!out)return FAILED(hr)?hr:E_POINTER;bool rgb24=m_mt.subtype==MEDIASUBTYPE_RGB24;int channels=rgb24?3:4,size=640*480*channels;if(sample->GetSize()<size)return E_FAIL;
  for(int y=0;y<480;++y){const BYTE* src=pixels_.data()+y*640*4;BYTE* row=out+(479-y)*640*channels;if(!rgb24)std::memcpy(row,src,640*4);else for(int x=0;x<640;++x)std::memcpy(row+x*3,src+x*4,3);}
  REFERENCE_TIME start=sequence_*Interval,stop=start+Interval;++sequence_;sample->SetTime(&start,&stop);sample->SetActualDataLength(size);sample->SetSyncPoint(TRUE);sample->SetDiscontinuity(sequence_==1);return S_OK;
 }
 STDMETHODIMP SetFormat(AM_MEDIA_TYPE* type) override {Trace(L"SetFormat");CAutoLock lock(m_pFilter->pStateLock());FILTER_STATE state;HRESULT hr=m_pFilter->GetState(0,&state);if(FAILED(hr))return hr;if(state!=State_Stopped)return VFW_E_NOT_STOPPED;CMediaType requested;if(type)requested=*type;else {hr=MakeType(&requested);if(FAILED(hr))return hr;}hr=ValidateType(&requested);if(FAILED(hr))return hr;if(IsConnected()&&requested!=m_mt)return VFW_E_ALREADY_CONNECTED;hr=CBaseOutputPin::SetMediaType(&requested);if(SUCCEEDED(hr))configured_=true;return hr;}
 STDMETHODIMP GetFormat(AM_MEDIA_TYPE** out) override {if(!out)return E_POINTER;*out=nullptr;CAutoLock lock(m_pFilter->pStateLock());CMediaType type;HRESULT hr=CurrentType(type);if(FAILED(hr))return hr;*out=CreateMediaType(&type);return *out?S_OK:E_OUTOFMEMORY;}
 STDMETHODIMP GetNumberOfCapabilities(int* count,int* size) override {if(!count||!size)return E_POINTER;*count=2;*size=sizeof(VIDEO_STREAM_CONFIG_CAPS);return S_OK;}
 STDMETHODIMP GetStreamCaps(int index,AM_MEDIA_TYPE** out,BYTE* capsOut) override {Trace(L"GetStreamCaps");if(!out)return E_POINTER;*out=nullptr;if(!capsOut)return E_POINTER;if(index<0||index>=2)return E_INVALIDARG;CMediaType type;HRESULT hr=MakeType(&type,index==1);if(FAILED(hr))return hr;VIDEO_STREAM_CONFIG_CAPS caps={};caps.guid=FORMAT_VideoInfo;caps.InputSize=caps.MinCroppingSize=caps.MaxCroppingSize=caps.MinOutputSize=caps.MaxOutputSize={640,480};caps.CropGranularityX=caps.CropGranularityY=caps.CropAlignX=caps.CropAlignY=caps.OutputGranularityX=caps.OutputGranularityY=1;caps.MinFrameInterval=caps.MaxFrameInterval=Interval;caps.MinBitsPerSecond=caps.MaxBitsPerSecond=type.GetSampleSize()*8*30;std::memcpy(capsOut,&caps,sizeof(caps));*out=CreateMediaType(&type);return *out?S_OK:E_OUTOFMEMORY;}
 STDMETHODIMP Set(REFGUID,DWORD,void*,DWORD,void*,DWORD) override {return E_NOTIMPL;}
 STDMETHODIMP Get(REFGUID set,DWORD property,void*,DWORD,void* data,DWORD size,DWORD* returned) override {if(!returned)return E_POINTER;*returned=0;if(set!=AMPROPSETID_Pin)return E_PROP_SET_UNSUPPORTED;if(property!=AMPROPERTY_PIN_CATEGORY)return E_PROP_ID_UNSUPPORTED;*returned=sizeof(GUID);if(!data)return S_OK;if(size<sizeof(GUID))return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);std::memcpy(data,&PIN_CATEGORY_CAPTURE,sizeof(GUID));return S_OK;}
 STDMETHODIMP QuerySupported(REFGUID set,DWORD property,DWORD* support) override {if(!support)return E_POINTER;*support=0;if(set!=AMPROPSETID_Pin)return E_PROP_SET_UNSUPPORTED;if(property!=AMPROPERTY_PIN_CATEGORY)return E_PROP_ID_UNSUPPORTED;*support=KSPROPERTY_SUPPORT_GET;return S_OK;}
};
class MaxSource final:public CSource,public IAMFilterMiscFlags{
public:
 MaxSource(LPUNKNOWN outer,HRESULT* hr):CSource(NAME("PNGTuber Max Camera"),outer,CLSID_MaxCamera,hr){Trace(L"Native camera 0.1.7 constructed");auto* stream=new(std::nothrow) MaxStream(hr,this);if(!stream)*hr=E_OUTOFMEMORY;}
 DECLARE_IUNKNOWN;
 STDMETHODIMP NonDelegatingQueryInterface(REFIID iid,void** out) override {if(!out)return E_POINTER;if(iid==IID_IAMFilterMiscFlags)return GetInterface((IAMFilterMiscFlags*)this,out);return CSource::NonDelegatingQueryInterface(iid,out);}
 STDMETHODIMP_(ULONG) GetMiscFlags() override {return AM_FILTER_MISC_FLAGS_IS_SOURCE;}
 static CUnknown* WINAPI Create(LPUNKNOWN outer,HRESULT* hr){auto* source=new(std::nothrow) MaxSource(outer,hr);if(!source)*hr=E_OUTOFMEMORY;return source;}
};
CFactoryTemplate g_Templates[]={{L"PNGTuber Max Camera",&CLSID_MaxCamera,MaxSource::Create,nullptr,nullptr}};
int g_cTemplates=sizeof(g_Templates)/sizeof(g_Templates[0]);
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE,ULONG,LPVOID);
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved){return DllEntryPoint(instance,reason,reserved);}
