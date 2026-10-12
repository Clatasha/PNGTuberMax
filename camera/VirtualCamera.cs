// PNGTuber Max camera backend by Clatasha. Experimental DirectShow source.
using System;
using System.IO.MemoryMappedFiles;
using System.Runtime.InteropServices;
using System.Threading;
using DirectShowLib;
[assembly:ComVisible(false)]
namespace Clatasha.Camera {
[ComImport,Guid("56a8689d-0ad4-11ce-b03a-0020af0ba770"),InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface IMemInputPin {
 [PreserveSig]int GetAllocator(out IMemAllocator allocator);
 [PreserveSig]int NotifyAllocator(IMemAllocator allocator,[MarshalAs(UnmanagedType.Bool)]bool readOnly);
 [PreserveSig]int GetAllocatorRequirements(AllocatorProperties props);
 [PreserveSig]int Receive(IMediaSample sample);
 [PreserveSig]int ReceiveMultiple([MarshalAs(UnmanagedType.LPArray,SizeParamIndex=1)]IMediaSample[] samples,int count,out int processed);
 [PreserveSig]int ReceiveCanBlock();
}
[ComVisible(true),Guid("EEBEE603-EC24-4D77-8827-C8F799C81026"),ClassInterface(ClassInterfaceType.None)]
public sealed class PuppetCamera:IBaseFilter,DirectShowLib.IMediaFilter,IAMFilterMiscFlags {
 public static readonly Guid Id=new Guid("EEBEE603-EC24-4D77-8827-C8F799C81026");
 internal FilterState State=FilterState.Stopped;internal IReferenceClock Clock;IFilterGraph graph;string name="PNGTuber Max Camera";readonly CameraPin pin;
 public PuppetCamera(){pin=new CameraPin(this);}
 public int GetClassID(out Guid id){id=Id;return 0;}public int Stop(){State=FilterState.Stopped;pin.Stop();return 0;}
 public int Pause(){State=FilterState.Paused;pin.Start();return 0;}
 public int Run(long start){State=FilterState.Running;pin.Start();return 0;}
 public int GetState(int timeout,out FilterState state){state=State;return 0;}
 public int SetSyncSource(IReferenceClock clock){Clock=clock;return 0;}public int GetSyncSource(out IReferenceClock clock){clock=Clock;return 0;}
 public int EnumPins(out IEnumPins pins){pins=new PinEnumerator(pin);return 0;}
 public int FindPin(string id,out IPin found){found=id=="Output"?pin:null;return found==null?unchecked((int)0x80040216):0;}
 public int QueryFilterInfo(out FilterInfo info){info=new FilterInfo{achName=name,pGraph=graph};return 0;}
 public int JoinFilterGraph(IFilterGraph g,string n){graph=g;if(n!=null)name=n;return 0;}
 public int QueryVendorInfo(out string vendor){vendor="Clatasha";return 0;}public int GetMiscFlags(){return 1;}
}
[ComVisible(true),ClassInterface(ClassInterfaceType.None)]
public sealed class CameraPin:IPin,IAMStreamConfig,IKsPropertySet {
 const int W=640,H=480,N=W*H*4;const long Interval=333333;
 static readonly Guid Video=new Guid("73646976-0000-0010-8000-00aa00389b71"),Rgb32=new Guid("e436eb7e-524f-11ce-9f53-0020af0ba770"),Info=new Guid("05589f80-c356-11ce-bf01-00aa0055595a");
 static readonly Guid Property=new Guid("9b00f101-1567-11d1-b3f1-00aa003761c5"),Capture=new Guid("fb6c4281-0353-11d1-905f-0000c0cc16ba");
 PuppetCamera owner;IPin connected;IMemInputPin input;IMemAllocator allocator;Thread worker;volatile bool stopping;readonly object gate=new object();
 public CameraPin(PuppetCamera filter){owner=filter;}
 AMMediaType Type(){var header=new VideoInfoHeader{AvgTimePerFrame=Interval,BitRate=N*8*30,BmiHeader=new BitmapInfoHeader{Size=40,Width=W,Height=H,Planes=1,BitCount=32,Compression=0,ImageSize=N}};var mt=new AMMediaType{majorType=Video,subType=Rgb32,fixedSizeSamples=true,temporalCompression=false,sampleSize=N,formatType=Info,formatSize=Marshal.SizeOf(typeof(VideoInfoHeader))};mt.formatPtr=Marshal.AllocCoTaskMem(mt.formatSize);Marshal.StructureToPtr(header,mt.formatPtr,false);return mt;}
 bool Accept(AMMediaType mt){if(mt==null||mt.majorType!=Video||mt.subType!=Rgb32||mt.formatType!=Info||mt.formatPtr==IntPtr.Zero||mt.formatSize<Marshal.SizeOf(typeof(VideoInfoHeader)))return false;var h=(VideoInfoHeader)Marshal.PtrToStructure(mt.formatPtr,typeof(VideoInfoHeader));return h.BmiHeader.Width==W&&h.BmiHeader.Height==H&&h.BmiHeader.BitCount==32&&h.BmiHeader.Compression==0&&h.AvgTimePerFrame==Interval;}
 public int Connect(IPin other,IntPtr proposedPtr){var proposed=proposedPtr==IntPtr.Zero?null:(AMMediaType)Marshal.PtrToStructure(proposedPtr,typeof(AMMediaType));lock(gate){if(connected!=null)return unchecked((int)0x80040204);if(proposed!=null&&!Accept(proposed))return unchecked((int)0x8004022a);var mt=Type();try{IntPtr pointer=Marshal.AllocCoTaskMem(Marshal.SizeOf(typeof(AMMediaType)));int hr;try{Marshal.StructureToPtr(mt,pointer,false);hr=other.ReceiveConnection(this,pointer);}finally{Marshal.FreeCoTaskMem(pointer);}if(hr<0)return hr;input=other as IMemInputPin;if(input==null){other.Disconnect();return unchecked((int)0x80004002);}hr=input.GetAllocator(out allocator);if(hr<0||allocator==null)allocator=(IMemAllocator)Activator.CreateInstance(System.Type.GetTypeFromCLSID(new Guid("1e651cc0-b199-11d0-8212-00c04fc32c45")));
 var requested=new AllocatorProperties{cBuffers=3,cbBuffer=N,cbAlign=1};var actual=new AllocatorProperties();hr=allocator.SetProperties(requested,actual);if(hr<0||actual.cbBuffer<N){other.Disconnect();allocator=null;input=null;return hr<0?hr:unchecked((int)0x80004005);}hr=input.NotifyAllocator(allocator,false);if(hr<0){other.Disconnect();allocator=null;input=null;return hr;}connected=other;return 0;}finally{DsUtils.FreeAMMediaType(mt);}}}
 public int ReceiveConnection(IPin p,IntPtr t){return unchecked((int)0x80040209);}
 public int Disconnect(){Stop();lock(gate){connected=null;input=null;allocator=null;}return 0;}
 public int ConnectedTo(out IPin p){p=connected;return p==null?unchecked((int)0x80040209):0;}
 public int ConnectionMediaType(IntPtr mt){if(mt==IntPtr.Zero)return unchecked((int)0x80004003);if(connected==null)return unchecked((int)0x80040209);Marshal.StructureToPtr(Type(),mt,false);return 0;}
 static void Copy(AMMediaType src,AMMediaType dest){dest.majorType=src.majorType;dest.subType=src.subType;dest.fixedSizeSamples=src.fixedSizeSamples;dest.temporalCompression=src.temporalCompression;dest.sampleSize=src.sampleSize;dest.formatType=src.formatType;dest.formatSize=src.formatSize;dest.formatPtr=src.formatPtr;dest.unkPtr=IntPtr.Zero;}
 public int QueryPinInfo(out PinInfo info){info=new PinInfo{filter=owner,dir=PinDirection.Output,name="Output"};return 0;}
 public int QueryDirection(out PinDirection dir){dir=PinDirection.Output;return 0;}public int QueryId(out string id){id="Output";return 0;}
 public int QueryAccept(IntPtr mt){return mt!=IntPtr.Zero&&Accept((AMMediaType)Marshal.PtrToStructure(mt,typeof(AMMediaType)))?0:1;}
 public int EnumMediaTypes(out IEnumMediaTypes e){e=new MediaEnumerator(Type);return 0;}
 public int QueryInternalConnections(IntPtr pins,ref int count){count=0;return unchecked((int)0x80004001);}
 public int EndOfStream(){return 0;}public int BeginFlush(){return 0;}public int EndFlush(){return 0;}public int NewSegment(long start,long stop,double rate){return 0;}
 public int SetFormat(AMMediaType mt){return Accept(mt)?0:unchecked((int)0x80070057);}
 public int GetFormat(out AMMediaType mt){mt=Type();return 0;}
 public int GetNumberOfCapabilities(out int count,out int size){count=1;size=Marshal.SizeOf(typeof(VideoStreamConfigCaps));return 0;}
 public int GetStreamCaps(int index,out AMMediaType mt,IntPtr caps){mt=null;if(index!=0)return unchecked((int)0x80070057);mt=Type();var size=new System.Drawing.Size(W,H);var c=new VideoStreamConfigCaps{guid=Info,InputSize=size,MinCroppingSize=size,MaxCroppingSize=size,MinOutputSize=size,MaxOutputSize=size,MinFrameInterval=Interval,MaxFrameInterval=Interval,MinBitsPerSecond=N*8*30,MaxBitsPerSecond=N*8*30};Marshal.StructureToPtr(c,caps,false);return 0;}
 public int Set(Guid p,int id,IntPtr instance,int ilen,IntPtr data,int len){return unchecked((int)0x80004001);}
 public int Get(Guid p,int id,IntPtr instance,int ilen,IntPtr data,int len,out int returned){returned=0;if(p!=Property||id!=0)return unchecked((int)0x80070490);returned=16;if(data==IntPtr.Zero)return 0;if(len<16)return unchecked((int)0x8007007a);Marshal.Copy(Capture.ToByteArray(),0,data,16);return 0;}
 public int QuerySupported(Guid p,int id,out KSPropertySupport support){support=(KSPropertySupport)0;if(p!=Property||id!=0)return unchecked((int)0x80070490);support=KSPropertySupport.Get;return 0;}
 internal void Start(){lock(gate){if(worker!=null&&worker.IsAlive)return;if(allocator==null||input==null){Trace("Start: pin is not connected.");return;}stopping=false;int result=allocator.Commit();Trace("Allocator commit: 0x"+result.ToString("X8"));if(result<0)return;worker=new Thread(Pump){IsBackground=true,Name="PNGTuber Max virtual camera"};worker.Start();}}
 internal void Stop(){stopping=true;var a=allocator;if(a!=null)a.Decommit();var t=worker;if(t!=null&&t!=Thread.CurrentThread)t.Join(1500);}
 static void Trace(string text){try{string directory=System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Clatasha","PNGTuberMax");System.IO.Directory.CreateDirectory(directory);System.IO.File.AppendAllText(System.IO.Path.Combine(directory,"camera-stream-log.txt"),DateTime.Now+" "+text+Environment.NewLine);}catch{}}
 void Pump(){MemoryMappedFile map=null;MemoryMappedViewAccessor read=null;Mutex mutex=null;byte[] bgra=new byte[N],frame=new byte[N];long sequence=0;var sw=System.Diagnostics.Stopwatch.StartNew();try{while(!stopping){bool fresh=false;try{if(map==null){map=MemoryMappedFile.OpenExisting("Local\\PNGTuberMaxFrames",MemoryMappedFileRights.Read);read=map.CreateViewAccessor(0,64+N,MemoryMappedFileAccess.Read);mutex=Mutex.OpenExisting("Local\\PNGTuberMaxFrameLock");}if(mutex.WaitOne(5)){try{if(read.ReadInt32(0)==0x504D5831&&read.ReadInt32(4)==W&&read.ReadInt32(8)==H&&read.ReadInt32(12)==1&&DateTime.UtcNow.Ticks-read.ReadInt64(16)<TimeSpan.TicksPerSecond*2){read.ReadArray(64,bgra,0,N);fresh=true;}}finally{mutex.ReleaseMutex();}}}catch{if(read!=null)read.Dispose();if(map!=null)map.Dispose();if(mutex!=null)mutex.Dispose();read=null;map=null;mutex=null;}
 if(fresh){for(int y=0;y<H;y++)Buffer.BlockCopy(bgra,y*W*4,frame,(H-1-y)*W*4,W*4);}else{for(int i=0;i<N;i+=4){frame[i]=26;frame[i+1]=17;frame[i+2]=12;frame[i+3]=255;}}
 IMediaSample sample;int hr=allocator.GetBuffer(out sample,0,0,AMGBF.None);if(hr<0||sample==null){Trace("GetBuffer: 0x"+hr.ToString("X8"));break;}try{IntPtr data;sample.GetPointer(out data);if(sample.GetSize()<N){Trace("Allocated sample is too small.");break;}Marshal.Copy(frame,0,data,N);sample.SetActualDataLength(N);sample.SetSyncPoint(true);sample.SetDiscontinuity(sequence==0);sample.SetTime(new DsLong(sequence*Interval),new DsLong((sequence+1)*Interval));int received=input.Receive(sample);if(received<0){Trace("Receive: 0x"+received.ToString("X8"));break;}}finally{Marshal.ReleaseComObject(sample);}sequence++;int wait=(int)(sequence*1000.0/30-sw.ElapsedMilliseconds);if(wait>0)Thread.Sleep(Math.Min(wait,34));}}
 catch(Exception e){Trace(e.ToString());}finally{Trace("Stream ended after "+sequence+" frames.");if(read!=null)read.Dispose();if(map!=null)map.Dispose();if(mutex!=null)mutex.Dispose();}}
}
[ComVisible(true),ClassInterface(ClassInterfaceType.None)]public class PinEnumerator:IEnumPins {IPin pin;int index;public PinEnumerator(IPin p,int i=0){pin=p;index=i;}public int Next(int n,IPin[] p,IntPtr fetched){int count=0;if(n>0&&index==0){p[0]=pin;index=1;count=1;}if(fetched!=IntPtr.Zero)Marshal.WriteInt32(fetched,count);return count==n?0:1;}public int Skip(int n){int old=index;index=Math.Min(1,index+n);return index-old==n?0:1;}public int Reset(){index=0;return 0;}public int Clone(out IEnumPins e){e=new PinEnumerator(pin,index);return 0;}}
[ComVisible(true),ClassInterface(ClassInterfaceType.None)]public class MediaEnumerator:IEnumMediaTypes {Func<AMMediaType> factory;int index;public MediaEnumerator(Func<AMMediaType> f,int i=0){factory=f;index=i;}public int Next(int n,IntPtr types,IntPtr fetched){if(types==IntPtr.Zero)return unchecked((int)0x80004003);int count=0;if(n>0&&index==0){var type=factory();IntPtr pointer=Marshal.AllocCoTaskMem(Marshal.SizeOf(typeof(AMMediaType)));Marshal.StructureToPtr(type,pointer,false);Marshal.WriteIntPtr(types,pointer);index=1;count=1;}if(fetched!=IntPtr.Zero)Marshal.WriteInt32(fetched,count);return count==n?0:1;}public int Skip(int n){int old=index;index=Math.Min(1,index+n);return index-old==n?0:1;}public int Reset(){index=0;return 0;}public int Clone(out IEnumMediaTypes e){e=new MediaEnumerator(factory,index);return 0;}}
}
