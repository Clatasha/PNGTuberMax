#pragma once
#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
// Transparency changes DWM composition, never normal window style or layout direction.
inline void MaxSetWindowTransparency(HWND window,bool transparent){
 if(!window)return;
 LONG_PTR ex=GetWindowLongPtr(window,GWL_EXSTYLE);
 LONG_PTR interactive=ex&~(WS_EX_LAYOUTRTL|WS_EX_RTLREADING|WS_EX_LEFTSCROLLBAR|WS_EX_TRANSPARENT);
 if(interactive!=ex)SetWindowLongPtr(window,GWL_EXSTYLE,interactive);
 MARGINS margins=transparent?MARGINS{-1,-1,-1,-1}:MARGINS{0,0,0,0};DwmExtendFrameIntoClientArea(window,&margins);
 HRGN region=transparent?CreateRectRgn(-1,-1,0,0):nullptr;
 DWM_BLURBEHIND blur{};blur.dwFlags=DWM_BB_ENABLE|(transparent?DWM_BB_BLURREGION:0);blur.fEnable=transparent;blur.hRgnBlur=region;
 DwmEnableBlurBehindWindow(window,&blur);if(region)DeleteObject(region);
 EnableWindow(window,TRUE);SetWindowPos(window,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
}
#endif
