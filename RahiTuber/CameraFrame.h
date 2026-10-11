// PNGTuber Max camera frame contract, by Clatasha.
#pragma once
#include <cstdint>
#include <cstddef>
namespace MaxCameraFrame {
constexpr int Width=640,Height=480,Bytes=Width*Height*4;
constexpr std::uint32_t Magic=0x504D5831;
struct Header {std::uint32_t magic=Magic;std::int32_t width=Width,height=Height,active=0;std::int64_t utcTicks=0;std::uint8_t reserved[40]={};};
static_assert(sizeof(Header)==64,"Camera header must be exactly 64 bytes");
static_assert(offsetof(Header,utcTicks)==16,"Timestamp must match the camera consumer");
struct Cadence {std::int64_t nextUs=0;bool Due(std::int64_t now){if(now<nextUs)return false;nextUs+=33333;if(nextUs<=now)nextUs=now+33333;return true;}};
inline void RgbaToBgra(const std::uint8_t* rgba,std::uint8_t* bgra,std::size_t pixels){for(std::size_t p=0;p<pixels;p++){std::size_t i=p*4;bgra[i]=rgba[i+2];bgra[i+1]=rgba[i+1];bgra[i+2]=rgba[i];bgra[i+3]=255;}}
}
