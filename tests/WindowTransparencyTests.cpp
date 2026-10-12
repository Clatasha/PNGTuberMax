#define WIN32_LEAN_AND_MEAN
#include "../RahiTuber/MaxWindowTransparency.h"
#include <stdexcept>
#include <iostream>
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"PNGMaxTransparencyTest";check(RegisterClassW(&wc)!=0,"Window class failed");HWND w=CreateWindowExW(WS_EX_APPWINDOW,wc.lpszClassName,L"Transparency test",WS_OVERLAPPEDWINDOW,100,100,480,320,nullptr,nullptr,wc.hInstance,nullptr);check(w!=nullptr,"Window creation failed");auto style=GetWindowLongPtr(w,GWL_STYLE),ex=GetWindowLongPtr(w,GWL_EXSTYLE);
 for(int i=0;i<20;++i){MaxSetWindowTransparency(w,i%2==0);check(GetWindowLongPtr(w,GWL_STYLE)==style,"Transparency changed normal window style");check(GetWindowLongPtr(w,GWL_EXSTYLE)==ex,"Transparency changed extended window style");POINT left{10,10},right{110,10};ClientToScreen(w,&left);ClientToScreen(w,&right);check(right.x>left.x,"Mouse coordinates were mirrored");check(IsWindowEnabled(w),"Window stopped accepting input");}
 SetWindowLongPtr(w,GWL_EXSTYLE,ex|WS_EX_LAYOUTRTL|WS_EX_RTLREADING|WS_EX_TRANSPARENT);MaxSetWindowTransparency(w,true);check(GetWindowLongPtr(w,GWL_EXSTYLE)==ex,"Existing mirrored/click-through flags were not cleared");DestroyWindow(w);UnregisterClassW(wc.lpszClassName,wc.hInstance);std::cout<<"PASS: repeated transparency toggles preserve styles, caption direction and mouse coordinates; corrupted flags recover\n";}
