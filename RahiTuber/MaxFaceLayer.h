#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <cmath>
#include <algorithm>
#include <vector>
struct MaxFaceLayer {
 float zoom=2,panX=0,panY=0,cropWidth=1,cropHeight=1,cropRotation=0;
 float brightness=0,contrast=1,gamma=1,saturation=1,warmth=0,softness=.12f;
 float height=.135f,rotation=0;int mask=0;
};
inline std::vector<unsigned char> MaxFacePixels(const unsigned char* source,int sw,int sh,const MaxFaceLayer& config,int w=256,int h=192){
 std::vector<unsigned char> output(std::size_t(w)*h*4,0);float angle=config.cropRotation*.01745329252f,c=std::cos(angle),s=std::sin(angle);
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){float u=(x+.5f)/w,v=(y+.5f)/h,px=(u-.5f)*config.cropWidth/std::max(.1f,config.zoom),py=(v-.5f)*config.cropHeight/std::max(.1f,config.zoom),sx=(c*px-s*py+.5f+config.panX)*sw-.5f,sy=(s*px+c*py+.5f+config.panY)*sh-.5f;
 if(sx<-.5f||sy<-.5f||sx>sw-.5f||sy>sh-.5f)continue;sx=std::clamp(sx,0.f,float(sw-1));sy=std::clamp(sy,0.f,float(sh-1));int ix=int(sx),iy=int(sy),nx=std::min(ix+1,sw-1),ny=std::min(iy+1,sh-1);float tx=sx-ix,ty=sy-iy,rgba[4];
 for(int k=0;k<4;++k)rgba[k]=((source[(iy*sw+ix)*4+k]*(1-tx)+source[(iy*sw+nx)*4+k]*tx)*(1-ty)+(source[(ny*sw+ix)*4+k]*(1-tx)+source[(ny*sw+nx)*4+k]*tx)*ty)/255;
 float ex=std::abs(u-.5f)*2,ey=std::abs(v-.5f)*2,d=config.mask==0?std::hypot(ex,ey):std::max(ex,ey);if(config.mask==1){float dx=ex-.75f,dy=ey-.75f;d=std::hypot(std::max(dx,0.f),std::max(dy,0.f))+std::min(std::max(dx,dy),0.f)+.75f;}
 float t=std::clamp((d-1+std::max(.002f,config.softness))/std::max(.002f,config.softness),0.f,1.f),alpha=rgba[3]*(1-t*t*(3-2*t)),colour[3];if(alpha<=0)continue;
 for(int k=0;k<3;++k)colour[k]=(std::pow(std::max(0.f,rgba[k]/std::max(.00001f,rgba[3])),1/std::max(.1f,config.gamma))*std::exp2(config.brightness)-.5f)*config.contrast+.5f;
 float grey=colour[0]*.2126f+colour[1]*.7152f+colour[2]*.0722f;auto out=output.data()+(std::size_t(y)*w+x)*4;for(int k=0;k<3;++k){float warmth=k==0?config.warmth:k==2?-config.warmth:0;out[k]=static_cast<unsigned char>(std::clamp(grey+(colour[k]-grey)*config.saturation+warmth,0.f,1.f)*alpha*255+.5f);}out[3]=static_cast<unsigned char>(alpha*255+.5f);
 }return output;
}
inline void MaxFaceDraw(sf::RenderTarget& target,const sf::Texture& texture,const MaxFaceLayer& config,sf::FloatRect bounds,bool preserveBodyAlpha=false){
 static sf::Shader shader;static bool loaded=sf::Shader::isAvailable()&&shader.loadFromMemory(R"GLSL(
 #version 130
 uniform sampler2D texture;
 uniform vec2 crop,pan;uniform float zoom,angle,brightness,contrast,gamma,saturation,warmth,softness;uniform int mask;
 void main(){
 vec2 uv=gl_TexCoord[0].xy;vec2 p=(uv-.5)*crop/max(.1,zoom);float c=cos(angle),s=sin(angle);vec2 q=vec2(c*p.x-s*p.y,s*p.x+c*p.y)+.5+pan;
 if(q.x<0.||q.x>1.||q.y<0.||q.y>1.){gl_FragColor=vec4(0);return;}
 vec4 texel=texture2D(texture,q);vec3 colour=texel.rgb/max(texel.a,.00001);
 colour=pow(max(colour,vec3(0)),vec3(1./max(.1,gamma)))*exp2(brightness);colour=(colour-.5)*contrast+.5;
 float grey=dot(colour,vec3(.2126,.7152,.0722));colour=mix(vec3(grey),colour,saturation)+vec3(warmth,0.,-warmth);colour=clamp(colour,0.,1.);
 vec2 edge=abs(uv-.5)*2.;float distance=mask==0?length(edge):max(edge.x,edge.y);
 if(mask==1){vec2 d=edge-vec2(.75);distance=length(max(d,0.))+min(max(d.x,d.y),0.)+.75;}
 float alpha=texel.a*(1.-smoothstep(1.-max(.002,softness),1.,distance));gl_FragColor=vec4(colour*alpha,alpha);
 })GLSL",sf::Shader::Fragment);
 if(!loaded){auto source=texture.copyToImage();auto size=source.getSize();auto pixels=MaxFacePixels(source.getPixelsPtr(),size.x,size.y,config);sf::Image image;image.create(256,192,pixels.data());sf::Texture adjusted;adjusted.loadFromImage(image);adjusted.setSmooth(true);sf::Sprite sprite(adjusted);sprite.setOrigin(128,96);sprite.setPosition(bounds.left,bounds.top);sprite.setScale(bounds.width/256,bounds.height/192);sprite.setRotation(config.rotation);sf::RenderStates state;state.blendMode=preserveBodyAlpha?sf::BlendMode(sf::BlendMode::DstAlpha,sf::BlendMode::OneMinusSrcAlpha,sf::BlendMode::Add,sf::BlendMode::Zero,sf::BlendMode::One,sf::BlendMode::Add):sf::BlendMode(sf::BlendMode::One,sf::BlendMode::OneMinusSrcAlpha);target.draw(sprite,state);return;}
 shader.setUniform("texture",sf::Shader::CurrentTexture);shader.setUniform("crop",sf::Glsl::Vec2(config.cropWidth,config.cropHeight));shader.setUniform("pan",sf::Glsl::Vec2(config.panX,config.panY));
 shader.setUniform("zoom",config.zoom);shader.setUniform("angle",config.cropRotation*.01745329252f);shader.setUniform("brightness",config.brightness);shader.setUniform("contrast",config.contrast);shader.setUniform("gamma",config.gamma);shader.setUniform("saturation",config.saturation);shader.setUniform("warmth",config.warmth);shader.setUniform("softness",config.softness);shader.setUniform("mask",config.mask);
 sf::Sprite sprite(texture);auto size=texture.getSize();sprite.setPosition(bounds.left,bounds.top);sprite.setScale(bounds.width/size.x,bounds.height/size.y);sprite.setOrigin(size.x*.5f,size.y*.5f);sprite.setRotation(config.rotation);
 sf::RenderStates state;state.shader=&shader;
 state.blendMode=preserveBodyAlpha?sf::BlendMode(sf::BlendMode::DstAlpha,sf::BlendMode::OneMinusSrcAlpha,sf::BlendMode::Add,sf::BlendMode::Zero,sf::BlendMode::One,sf::BlendMode::Add):sf::BlendMode(sf::BlendMode::One,sf::BlendMode::OneMinusSrcAlpha);
 target.draw(sprite,state);
}
struct MaxFaceOverlay {const sf::Texture* texture=nullptr;MaxFaceLayer settings;float x=.5f,y=.5f,width=.18f;};
