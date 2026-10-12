// DirectShow source-side COM contracts, by Clatasha.
// Raw pointers are required for optional media types and native enumerator output arrays.
using System;using System.Runtime.InteropServices;using DirectShowLib;
namespace Clatasha.Camera {
[ComVisible(true),Guid("56a86895-0ad4-11ce-b03a-0020af0ba770"),InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface IBaseFilter {
 [PreserveSig]int GetClassID(out Guid id);
 [PreserveSig]int Stop();[PreserveSig]int Pause();[PreserveSig]int Run(long start);
 [PreserveSig]int GetState(int timeout,out FilterState state);
 [PreserveSig]int SetSyncSource(IReferenceClock clock);[PreserveSig]int GetSyncSource(out IReferenceClock clock);
 [PreserveSig]int EnumPins(out IEnumPins pins);
 [PreserveSig]int FindPin([MarshalAs(UnmanagedType.LPWStr)]string id,out IPin pin);
 [PreserveSig]int QueryFilterInfo(out FilterInfo info);
 [PreserveSig]int JoinFilterGraph(IFilterGraph graph,[MarshalAs(UnmanagedType.LPWStr)]string name);
 [PreserveSig]int QueryVendorInfo([MarshalAs(UnmanagedType.LPWStr)]out string vendor);
}
[StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)]public struct PinInfo {
 public IBaseFilter filter;public PinDirection dir;[MarshalAs(UnmanagedType.ByValTStr,SizeConst=128)]public string name;
}
[ComVisible(true),Guid("56a86891-0ad4-11ce-b03a-0020af0ba770"),InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface IPin {
 [PreserveSig]int Connect(IPin other,IntPtr mediaType);
 [PreserveSig]int ReceiveConnection(IPin other,IntPtr mediaType);
 [PreserveSig]int Disconnect();[PreserveSig]int ConnectedTo(out IPin other);
 [PreserveSig]int ConnectionMediaType(IntPtr mediaType);
 [PreserveSig]int QueryPinInfo(out PinInfo info);[PreserveSig]int QueryDirection(out PinDirection direction);
 [PreserveSig]int QueryId([MarshalAs(UnmanagedType.LPWStr)]out string id);
 [PreserveSig]int QueryAccept(IntPtr mediaType);[PreserveSig]int EnumMediaTypes(out IEnumMediaTypes types);
 [PreserveSig]int QueryInternalConnections(IntPtr pins,ref int count);
 [PreserveSig]int EndOfStream();[PreserveSig]int BeginFlush();[PreserveSig]int EndFlush();[PreserveSig]int NewSegment(long start,long stop,double rate);
}
[ComVisible(true),Guid("56a86892-0ad4-11ce-b03a-0020af0ba770"),InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface IEnumPins {
 [PreserveSig]int Next(int count,[Out,MarshalAs(UnmanagedType.LPArray,SizeParamIndex=0)]IPin[] pins,IntPtr fetched);
 [PreserveSig]int Skip(int count);[PreserveSig]int Reset();[PreserveSig]int Clone(out IEnumPins clone);
}
[ComVisible(true),Guid("89c31040-846b-11ce-97d3-00aa0055595a"),InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface IEnumMediaTypes {
 [PreserveSig]int Next(int count,IntPtr mediaTypes,IntPtr fetched);
 [PreserveSig]int Skip(int count);[PreserveSig]int Reset();[PreserveSig]int Clone(out IEnumMediaTypes clone);
}
// Source-side output pointers are caller-owned CoTaskMem, not CLR LPStruct outputs.
[ComVisible(true),Guid("C6E13340-30AC-11d0-A18C-00A0C9118956"),InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface IAMStreamConfig {
 [PreserveSig]int SetFormat(IntPtr mediaType);
 [PreserveSig]int GetFormat(IntPtr mediaTypeOut);
 [PreserveSig]int GetNumberOfCapabilities(out int count,out int size);
 [PreserveSig]int GetStreamCaps(int index,IntPtr mediaTypeOut,IntPtr caps);
}
}
