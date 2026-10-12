#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <cmath>
struct MaxFaceLayer {
 float zoom=2,panX=0,panY=0,cropWidth=1,cropHeight=1,cropRotation=0;
 float brightness=0,contrast=1,gamma=1,saturation=1,warmth=0,softness=.12f;
 float height=.135f,rotation=0;int mask=0;
};
inline void MaxFaceDraw(sf::RenderTarget& target,const sf::Texture& texture,const MaxFaceLayer& config,sf::FloatRect bounds,bool preserveBodyAlpha=false){
 static sf::Shader shader;static bool loaded=shader.loadFromMemory(R"GLSL(
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
 if(!loaded)return;
 shader.setUniform("texture",sf::Shader::CurrentTexture);shader.setUniform("crop",sf::Glsl::Vec2(config.cropWidth,config.cropHeight));shader.setUniform("pan",sf::Glsl::Vec2(config.panX,config.panY));
 shader.setUniform("zoom",config.zoom);shader.setUniform("angle",config.cropRotation*.01745329252f);shader.setUniform("brightness",config.brightness);shader.setUniform("contrast",config.contrast);shader.setUniform("gamma",config.gamma);shader.setUniform("saturation",config.saturation);shader.setUniform("warmth",config.warmth);shader.setUniform("softness",config.softness);shader.setUniform("mask",config.mask);
 sf::Sprite sprite(texture);auto size=texture.getSize();sprite.setPosition(bounds.left,bounds.top);sprite.setScale(bounds.width/size.x,bounds.height/size.y);sprite.setOrigin(size.x*.5f,size.y*.5f);sprite.setRotation(config.rotation);
 sf::RenderStates state;state.shader=&shader;
 state.blendMode=preserveBodyAlpha?sf::BlendMode(sf::BlendMode::DstAlpha,sf::BlendMode::OneMinusSrcAlpha,sf::BlendMode::Add,sf::BlendMode::Zero,sf::BlendMode::One,sf::BlendMode::Add):sf::BlendMode(sf::BlendMode::One,sf::BlendMode::OneMinusSrcAlpha);
 target.draw(sprite,state);
}
struct MaxFaceOverlay {const sf::Texture* texture=nullptr;MaxFaceLayer settings;float x=.5f,y=.5f,width=.18f;};
