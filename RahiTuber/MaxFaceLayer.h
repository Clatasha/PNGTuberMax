#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <cmath>
#include <algorithm>
#include <vector>
struct MaxFaceLayer {
 float zoom=2,panX=0,panY=0,cropWidth=1,cropHeight=1,cropRotation=0;
 float brightness=0,contrast=1,gamma=1,saturation=1,warmth=0,softness=.12f;
 float height=.135f,rotation=0;int mask=3;
 float precisionPadding=2.5f,precisionFeather=1.25f,precisionOffsetX=0,precisionOffsetY=0,precisionScaleX=1,precisionScaleY=1;
};
inline void MaxCopyFaceSettings(MaxFaceLayer& destination,const MaxFaceLayer& source){float height=destination.height,rotation=destination.rotation;destination=source;destination.height=height;destination.rotation=rotation;}
inline float MaxContourDistance(const unsigned char* field,int w,int h,float u,float v,const MaxFaceLayer& c){
 u=(u-.5f-c.precisionOffsetX/w)/std::max(.1f,c.precisionScaleX)+.5f;v=(v-.5f-c.precisionOffsetY/h)/std::max(.1f,c.precisionScaleY)+.5f;if(u<0||v<0||u>1||v>1)return -32;
 float x=std::clamp(u*w-.5f,0.f,float(w-1)),y=std::clamp(v*h-.5f,0.f,float(h-1));int a=int(x),b=int(y),d=std::min(a+1,w-1),e=std::min(b+1,h-1);float tx=x-a,ty=y-b;
 float value=(field[(b*w+a)*4]*(1-tx)+field[(b*w+d)*4]*tx)*(1-ty)+(field[(e*w+a)*4]*(1-tx)+field[(e*w+d)*4]*tx)*ty;
 return (value/255*64-32)*(c.precisionScaleX+c.precisionScaleY)*.5f;
}
inline float MaxContourAlpha(float distance,const MaxFaceLayer& c){float feather=std::max(.1f,c.precisionFeather),t=std::clamp((distance+c.precisionPadding+feather*.5f)/feather,0.f,1.f);return t*t*(3-2*t);}
inline std::vector<unsigned char> MaxFacePixels(const unsigned char* source,int sw,int sh,const MaxFaceLayer& config,int w=256,int h=192,const unsigned char* contour=nullptr,int cw=256,int ch=192,bool outline=false){
 std::vector<unsigned char> output(std::size_t(w)*h*4,0);float angle=config.cropRotation*.01745329252f,c=std::cos(angle),s=std::sin(angle);
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){float u=(x+.5f)/w,v=(y+.5f)/h,px=(u-.5f)*config.cropWidth/std::max(.1f,config.zoom),py=(v-.5f)*config.cropHeight/std::max(.1f,config.zoom),sx=(c*px-s*py+.5f+config.panX)*sw-.5f,sy=(s*px+c*py+.5f+config.panY)*sh-.5f;
 if(sx<-.5f||sy<-.5f||sx>sw-.5f||sy>sh-.5f)continue;sx=std::clamp(sx,0.f,float(sw-1));sy=std::clamp(sy,0.f,float(sh-1));int ix=int(sx),iy=int(sy),nx=std::min(ix+1,sw-1),ny=std::min(iy+1,sh-1);float tx=sx-ix,ty=sy-iy,rgba[4];
 for(int k=0;k<4;++k)rgba[k]=((source[(iy*sw+ix)*4+k]*(1-tx)+source[(iy*sw+nx)*4+k]*tx)*(1-ty)+(source[(ny*sw+ix)*4+k]*(1-tx)+source[(ny*sw+nx)*4+k]*tx)*ty)/255;
 float ex=std::abs(u-.5f)*2,ey=std::abs(v-.5f)*2,d=config.mask==0?std::hypot(ex,ey):std::max(ex,ey);if(config.mask==1){float dx=ex-.75f,dy=ey-.75f;d=std::hypot(std::max(dx,0.f),std::max(dy,0.f))+std::min(std::max(dx,dy),0.f)+.75f;}
 float t=std::clamp((d-1+std::max(.002f,config.softness))/std::max(.002f,config.softness),0.f,1.f),alpha=rgba[3]*(1-t*t*(3-2*t)),colour[3];if(alpha<=0&&config.mask!=3)continue;
 for(int k=0;k<3;++k)colour[k]=(std::pow(std::max(0.f,rgba[k]/std::max(.00001f,rgba[3])),1/std::max(.1f,config.gamma))*std::exp2(config.brightness)-.5f)*config.contrast+.5f;
 float contourDistance=-32;
 if(config.mask==3){contourDistance=contour?MaxContourDistance(contour,cw,ch,(sx+.5f)/sw,(sy+.5f)/sh,config):-32;alpha=rgba[3]*MaxContourAlpha(contourDistance,config);}
 float grey=colour[0]*.2126f+colour[1]*.7152f+colour[2]*.0722f;auto out=output.data()+(std::size_t(y)*w+x)*4;for(int k=0;k<3;++k){float warmth=k==0?config.warmth:k==2?-config.warmth:0;out[k]=static_cast<unsigned char>(std::clamp(grey+(colour[k]-grey)*config.saturation+warmth,0.f,1.f)*alpha*255+.5f);}out[3]=static_cast<unsigned char>(alpha*255+.5f);
 if(outline&&config.mask==3){float border=std::abs(contourDistance+config.precisionPadding)<.75f?1.f:0.f;for(int k=0;k<3;++k){float colour=rgba[k]*(alpha>.5f?1.f:.3f);float guide=k==0?0:1;out[k]=static_cast<unsigned char>(std::clamp(colour*(1-border)+guide*border,0.f,1.f)*255+.5f);}out[3]=255;}
 }return output;
}
inline void MaxFaceDraw(sf::RenderTarget& target,const sf::Texture& texture,const MaxFaceLayer& config,sf::FloatRect bounds,bool preserveBodyAlpha=false,const sf::Texture* contour=nullptr,bool outline=false){
 if(config.mask==3&&!contour)return;
 static sf::Shader shader;static bool loaded=sf::Shader::isAvailable()&&shader.loadFromMemory(R"GLSL(
 #version 130
 uniform sampler2D texture,contourTexture;
 uniform float precisionPadding,precisionFeather;uniform vec2 precisionOffset,precisionScale;uniform bool outline;
 uniform vec2 crop,pan;uniform float zoom,angle,brightness,contrast,gamma,saturation,warmth,softness;uniform int mask;
 // Interpolate the encoded field in float. Some drivers quantize filtered R8
 // values before decoding, visibly stepping a thin eyelid feather.
 float contourValue(vec2 uv){ivec2 size=textureSize(contourTexture,0);vec2 p=clamp(uv*vec2(size)-.5,vec2(0),vec2(size-ivec2(1)));ivec2 a=ivec2(floor(p)),b=min(a+ivec2(1),size-ivec2(1));vec2 t=fract(p);
 return mix(mix(texelFetch(contourTexture,a,0).r,texelFetch(contourTexture,ivec2(b.x,a.y),0).r,t.x),mix(texelFetch(contourTexture,ivec2(a.x,b.y),0).r,texelFetch(contourTexture,b,0).r,t.x),t.y);}
 void main(){
 vec2 uv=gl_TexCoord[0].xy;vec2 p=(uv-.5)*crop/max(.1,zoom);float c=cos(angle),s=sin(angle);vec2 q=vec2(c*p.x-s*p.y,s*p.x+c*p.y)+.5+pan;
 if(q.x<0.||q.x>1.||q.y<0.||q.y>1.){gl_FragColor=vec4(0);return;}
 vec4 texel=texture2D(texture,q);vec3 colour=texel.rgb/max(texel.a,.00001);
 colour=pow(max(colour,vec3(0)),vec3(1./max(.1,gamma)))*exp2(brightness);colour=(colour-.5)*contrast+.5;
 float grey=dot(colour,vec3(.2126,.7152,.0722));colour=mix(vec3(grey),colour,saturation)+vec3(warmth,0.,-warmth);colour=clamp(colour,0.,1.);
 vec2 edge=abs(uv-.5)*2.;float distance=mask==0?length(edge):max(edge.x,edge.y);
 if(mask==1){vec2 d=edge-vec2(.75);distance=length(max(d,0.))+min(max(d.x,d.y),0.)+.75;}
 float alpha=texel.a*(1.-smoothstep(1.-max(.002,softness),1.,distance));float signedDistance=-32.;
 if(mask==3){vec2 m=(q-.5-precisionOffset)/precisionScale+.5;if(m.x>=0.&&m.x<=1.&&m.y>=0.&&m.y<=1.)signedDistance=(contourValue(m)*64.-32.)*(precisionScale.x+precisionScale.y)*.5;
 alpha=texel.a*smoothstep(-max(.1,precisionFeather)*.5,max(.1,precisionFeather)*.5,signedDistance+precisionPadding);}
 if(outline&&mask==3){float border=1.-smoothstep(.4,.9,abs(signedDistance+precisionPadding));gl_FragColor=vec4(mix(colour*(alpha>.5?1.:.3),vec3(0.,1.,1.),border),1.);}
 else gl_FragColor=vec4(colour*alpha,alpha);
 })GLSL",sf::Shader::Fragment);
 if(!loaded){auto source=texture.copyToImage();auto size=source.getSize();sf::Image field;if(contour)field=contour->copyToImage();auto fs=field.getSize();auto pixels=MaxFacePixels(source.getPixelsPtr(),size.x,size.y,config,256,192,contour?field.getPixelsPtr():nullptr,fs.x?fs.x:256,fs.y?fs.y:192,outline);sf::Image image;image.create(256,192,pixels.data());sf::Texture adjusted;adjusted.loadFromImage(image);adjusted.setSmooth(true);sf::Sprite sprite(adjusted);sprite.setOrigin(128,96);sprite.setPosition(bounds.left,bounds.top);sprite.setScale(bounds.width/256,bounds.height/192);sprite.setRotation(config.rotation);sf::RenderStates state;state.blendMode=preserveBodyAlpha?sf::BlendMode(sf::BlendMode::DstAlpha,sf::BlendMode::OneMinusSrcAlpha,sf::BlendMode::Add,sf::BlendMode::Zero,sf::BlendMode::One,sf::BlendMode::Add):sf::BlendMode(sf::BlendMode::One,sf::BlendMode::OneMinusSrcAlpha);target.draw(sprite,state);return;}
 if(contour)shader.setUniform("contourTexture",*contour);else shader.setUniform("contourTexture",texture);
 shader.setUniform("precisionPadding",config.precisionPadding);shader.setUniform("precisionFeather",config.precisionFeather);shader.setUniform("precisionOffset",sf::Glsl::Vec2(config.precisionOffsetX/256,config.precisionOffsetY/192));shader.setUniform("precisionScale",sf::Glsl::Vec2(config.precisionScaleX,config.precisionScaleY));shader.setUniform("outline",outline);
 shader.setUniform("texture",sf::Shader::CurrentTexture);shader.setUniform("crop",sf::Glsl::Vec2(config.cropWidth,config.cropHeight));shader.setUniform("pan",sf::Glsl::Vec2(config.panX,config.panY));
 shader.setUniform("zoom",config.zoom);shader.setUniform("angle",config.cropRotation*.01745329252f);shader.setUniform("brightness",config.brightness);shader.setUniform("contrast",config.contrast);shader.setUniform("gamma",config.gamma);shader.setUniform("saturation",config.saturation);shader.setUniform("warmth",config.warmth);shader.setUniform("softness",config.softness);shader.setUniform("mask",config.mask);
 sf::Sprite sprite(texture);auto size=texture.getSize();sprite.setPosition(bounds.left,bounds.top);sprite.setScale(bounds.width/size.x,bounds.height/size.y);sprite.setOrigin(size.x*.5f,size.y*.5f);sprite.setRotation(config.rotation);
 sf::RenderStates state;state.shader=&shader;
 state.blendMode=preserveBodyAlpha?sf::BlendMode(sf::BlendMode::DstAlpha,sf::BlendMode::OneMinusSrcAlpha,sf::BlendMode::Add,sf::BlendMode::Zero,sf::BlendMode::One,sf::BlendMode::Add):sf::BlendMode(sf::BlendMode::One,sf::BlendMode::OneMinusSrcAlpha);
 target.draw(sprite,state);
}
struct MaxFaceOverlay {const sf::Texture* texture=nullptr;MaxFaceLayer settings;float x=.5f,y=.5f,width=.18f;const sf::Texture* contour=nullptr;};
