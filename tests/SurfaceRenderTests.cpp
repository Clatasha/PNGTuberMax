#include "../RahiTuber/MaxPlushSurface.h"
#include "../RahiTuber/MaxFaceLayer.h"
#include <iostream>
#include <stdexcept>
#include <queue>
static void check(bool good,const char* message){if(!good)throw std::runtime_error(message);}
int main(){
 constexpr int w=256;sf::Image original;original.create(w,w,sf::Color::Transparent);
 for(int y=88;y<250;++y)for(int x=45;x<212;++x){float dx=(x-128.f)/80,dy=(y-169.f)/80;if(dx*dx+dy*dy<1)original.setPixel(x,y,{40,140,200,255});}
 auto line=[&](int ax,int ay,int bx,int by){int count=std::max(std::abs(bx-ax),std::abs(by-ay));for(int i=0;i<=count;++i){int x=ax+(bx-ax)*i/count,y=ay+(by-ay)*i/count;for(int k=-1;k<=1;++k)if(x+k>=0&&x+k<w)original.setPixel(x+k,y,{230,210,100,255});}};
 line(128,95,128,62);for(auto p:{sf::Vector2i(30,30),sf::Vector2i(225,35),sf::Vector2i(35,78),sf::Vector2i(230,73)})line(p.x,p.y,128,62);
 sf::Texture front,back;check(front.loadFromImage(original)&&back.loadFromImage(original),"Cannot load fixture");front.setSmooth(true);back.setSmooth(true);
 auto rows=MaxPlushContour(original.getPixelsPtr(),w,w,0,0,w,w);MaxPlushPixels pixels{original.getPixelsPtr(),w,w,0,0,w,w,&rows};auto depth=MaxPlushDepth(pixels);
 sf::Image feature;feature.create(256,192,{64,64,64,255});sf::Texture webcam;webcam.loadFromImage(feature);webcam.setSmooth(true);sf::RenderTexture composite;check(composite.create(w,w),"Cannot create facial composite");composite.clear(sf::Color::Transparent);sf::RenderStates state;state.blendMode=sf::BlendMode(sf::BlendMode::One,sf::BlendMode::OneMinusSrcAlpha);composite.draw(sf::Sprite(front),state);
 MaxFaceLayer config;config.zoom=1;config.brightness=1;MaxFaceDraw(composite,webcam,config,{128,169,40,30},true);composite.display();auto lit=composite.getTexture().copyToImage();check(std::abs(int(lit.getPixel(128,169).r)-128)<3,"Brightness adjustment or crop shader failed");check(lit.getPixel(0,0).a==0,"Facial composite lost body transparency");
 feature.create(256,192,{245,10,10,255});webcam.update(feature);composite.clear(sf::Color::Transparent);composite.draw(sf::Sprite(front),state);config.brightness=0;MaxFaceDraw(composite,webcam,config,{128,169,40,30},true);composite.display();
 MaxPlushSurface surface;sf::Image sheet;sheet.create(256*7,256,{20,30,45});int frame=0;
 for(float yaw:{-30.f,-15.f,-5.f,0.f,5.f,15.f,30.f}){
 check(surface.Draw(composite.getTexture(),{0,0,w,w},back,{0,0,w,w},depth,1,.45f,yaw,w,w,front,{0,0,w,w}),"Depth-buffered PNG surface unavailable");auto image=surface.Texture().copyToImage();auto size=image.getSize();double sum=0;int red=0;
 for(unsigned y=size.y/2;y<size.y*4/5;++y)for(unsigned x=0;x<size.x;++x){auto c=image.getPixel(x,y);if(c.r>180&&c.g<40&&c.a>200){sum+=x;++red;}}
 check(red>100,"Facial feature disappeared during turn");float radians=yaw*.01745329252f,z=MaxPlushDepthAt(depth,w,w,128,169)*.45f,expected=(128-z*std::sin(radians))*size.x/w;check(std::abs(sum/red-expected)<6,"Facial feature is floating instead of following the surface");
 // Four fine branches must remain connected to their central stem at modest turns.
 std::vector<bool> seen(size.x*size.y,false);int largeComponents=0;
 for(unsigned y=0;y<size.y*3/8;++y)for(unsigned x=0;x<size.x;++x){auto c=image.getPixel(x,y);if(c.r<150||c.g<150||c.b>150||c.a<80||seen[y*size.x+x])continue;std::queue<sf::Vector2i> queue;queue.push({int(x),int(y)});seen[y*size.x+x]=true;int count=0;
 while(!queue.empty()){auto p=queue.front();queue.pop();++count;for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){int nx=p.x+dx,ny=p.y+dy;if(nx<0||ny<0||nx>=int(size.x)||ny>=int(size.y*3/8))continue;auto i=ny*size.x+nx;if(seen[i])continue;auto value=image.getPixel(nx,ny);if(value.r>=150&&value.g>=150&&value.b<=150&&value.a>=80){seen[i]=true;queue.push({nx,ny});}}}if(count>8)++largeComponents;}
 check(largeComponents==1,"Antenna branches split while turning");sf::Texture view;view.loadFromImage(image);sf::RenderTexture tile;tile.create(256,256);tile.clear({20,30,45});sf::Sprite sprite(view);sprite.setScale(256.f/size.x,256.f/size.y);tile.draw(sprite,state);tile.display();sheet.copy(tile.getTexture().copyToImage(),frame++*256,0);
 }
 sheet.saveToFile("surface-render-check.png");std::cout<<"PASS: GPU continuous surface, connected four-point antenna, attached facial projection, crop brightness and alpha\n";
}
