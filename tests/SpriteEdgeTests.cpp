#include "../RahiTuber/MaxSpriteEdges.h"
#include <iostream>
#include <stdexcept>
static void check(bool good,const char* why){if(!good)throw std::runtime_error(why);}
int main(){try{
 const int w=15,h=15;std::vector<unsigned char> source(w*h*4);for(int y=2;y<=12;++y)for(int x=2;x<=12;++x){auto p=&source[(y*w+x)*4];p[0]=100;p[1]=150;p[2]=200;p[3]=255;}
 auto d=MaxSpriteEdgeDistances(source.data(),w,h,0,0,w,h);check(std::abs(d[2*w+7]-.5f)<.001f&&std::abs(d[7*w+7]-5.5f)<.001f,"Exterior distances are wrong");
 auto unchanged=source;MaxApplySpriteEdges(unchanged.data(),w,h,0,0,w,h,d,0,0);check(unchanged==source,"Disabled edges change original bytes");auto soft=source;MaxApplySpriteEdges(soft.data(),w,h,0,0,w,h,d,3,0);check(soft[(2*w+7)*4+3]>0&&soft[(2*w+7)*4+3]<64&&soft[(7*w+7)*4+3]==255,"Feather does not preserve sharp interior or soften silhouette");for(int i=0;i<w*h;++i)for(int c=0;c<3;++c)check(soft[i*4+c]<=soft[i*4+3],"Feather creates a premultiplied-alpha halo");
 auto trim=source;MaxApplySpriteEdges(trim.data(),w,h,0,0,w,h,d,0,1);check(trim[(2*w+7)*4+3]==0&&trim[(7*w+7)*4+3]==255,"Trim does not remove the outer outline");
 auto hole=source;for(int c=0;c<4;++c)hole[(7*w+7)*4+c]=0;auto hd=MaxSpriteEdgeDistances(hole.data(),w,h,0,0,w,h);MaxApplySpriteEdges(hole.data(),w,h,0,0,w,h,hd,3,0);check(hole[(7*w+6)*4+3]==255,"Enclosed transparency damages interior detail");
 // Check exact Euclidean distances against a brute-force exterior for an irregular silhouette.
 std::vector<unsigned char> shape(9*9*4);for(int y=1;y<8;++y)for(int x=1;x<8;++x)if((x-4)*(x-4)+(y-4)*(y-4)<=10)shape[(y*9+x)*4+3]=255;auto sd=MaxSpriteEdgeDistances(shape.data(),9,9,0,0,9,9);for(int y=0;y<9;++y)for(int x=0;x<9;++x){float best=1000;for(int b=-1;b<=9;++b)for(int a=-1;a<=9;++a)if(a<0||b<0||a>=9||b>=9||!shape[(b*9+a)*4+3])best=std::min(best,std::hypot(float(x-a),float(y-b)));check(std::abs(sd[y*9+x]-std::max(0.f,best-.5f))<.001f,"Distance transform misses diagonal outline");}
 std::vector<unsigned char> atlas(8*4*4,255);auto ad=MaxSpriteEdgeDistances(atlas.data(),8,4,0,0,4,4);MaxApplySpriteEdges(atlas.data(),8,4,0,0,4,4,ad,2,0);check(atlas[(1*8+3)*4+3]<255&&atlas[(1*8+4)*4+3]==255,"Sprite frame processing bleeds into its neighbor");
 std::cout<<"PASS: exact exterior-outline distances, disabled identity, inward feather/trim, preserved interior holes, premultiplied alpha and frame isolation\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
