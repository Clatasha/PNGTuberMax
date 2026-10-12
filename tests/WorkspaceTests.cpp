#include "../RahiTuber/WorkspaceLayout.h"
#include <stdexcept>
#include <iostream>
int main(){for(auto size:{std::pair<float,float>{1000,700},{1440,900},{1920,1080},{2560,1440}}){auto x=MaxWorkspace::Make(size.first,size.second,1440,900);if(x.left.x+x.left.w>=x.canvas.x||x.canvas.x+x.canvas.w>=x.right.x||x.fit.x<x.canvas.x||x.fit.x+x.fit.w>x.canvas.x+x.canvas.w+.01f)throw std::runtime_error("Preview overlaps a dock");if(std::fabs(x.fit.w/x.fit.h-1.6f)>.001f)throw std::runtime_error("Avatar stretched");}if(MaxMeterBlocks(0)!=0||MaxMeterBlocks(.04f)!=3||MaxMeterBlocks(.5f)!=5||MaxMeterBlocks(.9f)!=6||MaxMeterBlocks(NAN)!=0)throw std::runtime_error("Meter levels are wrong");std::cout<<"PASS: three-column separation, aspect ratio and six-block audio meter\n";}
