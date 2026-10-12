#pragma once
#include <SFML/Graphics.hpp>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#endif
#include <SFML/OpenGL.hpp>
#include "MaxPlushRaster.h"
// Internal PNG surface: texture coordinates stay fixed while connected vertices turn in depth.
class MaxPlushSurface {
 struct Vertex {float x,y,z,u,v;};std::vector<Vertex> frontMesh,backMesh;sf::RenderTexture target;sf::Shader shader;bool attempted=false,ready=false;int sourceW=0,sourceH=0;std::uint64_t revision=0;
public:
 const sf::Texture& Texture()const{return target.getTexture();}
 bool Draw(const sf::Texture& front,sf::IntRect frontRect,const sf::Texture& back,sf::IntRect backRect,const std::vector<float>& depth,std::uint64_t shapeRevision,float roundness,float yaw,int width,int height,const sf::Texture& maskTexture,sf::IntRect maskRect){
 if(!attempted){attempted=true;ready=sf::Shader::isAvailable()&&shader.loadFromMemory(R"GLSL(
 #version 130
 void main(){gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;gl_TexCoord[0]=gl_TextureMatrix[0]*gl_MultiTexCoord0;gl_TexCoord[1]=gl_MultiTexCoord0;}
 )GLSL",R"GLSL(
 #version 130
 uniform sampler2D texture,bodyMask;uniform vec4 maskRect;
 void main(){vec4 pixel=texture2D(texture,gl_TexCoord[0].xy);float mask=texture2D(bodyMask,maskRect.xy+gl_TexCoord[1].xy*maskRect.zw).a;float alpha=min(pixel.a,mask);if(alpha<.001)discard;gl_FragColor=vec4(pixel.rgb*(alpha/max(.001,pixel.a)),alpha);}
 )GLSL");shader.setUniform("texture",sf::Shader::CurrentTexture);}if(!ready)return false;auto maskSize=maskTexture.getSize();shader.setUniform("bodyMask",maskTexture);shader.setUniform("maskRect",sf::Glsl::Vec4(float(maskRect.left)/maskSize.x,float(maskRect.top)/maskSize.y,float(maskRect.width)/maskSize.x,float(maskRect.height)/maskSize.y));
 // Supersampling keeps small antenna strokes and alpha edges steady during subpixel turns.
 int rw=std::min(2048,width*2),rh=std::min(2048,height*2);
 if(target.getSize()!=sf::Vector2u(rw,rh)){sf::ContextSettings settings;settings.depthBits=24;if(!target.create(rw,rh,settings))return false;target.setSmooth(true);}
 if(sourceW!=frontRect.width||sourceH!=frontRect.height||revision!=shapeRevision||frontMesh.empty()){
 sourceW=frontRect.width;sourceH=frontRect.height;revision=shapeRevision;frontMesh.clear();backMesh.clear();int columns=std::min(192,sourceW),rows=std::min(192,sourceH);
 auto vertex=[&](int x,int y,bool rear){float u=float(x)/columns,v=float(y)/rows,px=u*sourceW,py=v*sourceH;return Vertex{px,py,MaxPlushDepthAt(depth,sourceW,sourceH,px,py)*(rear?-1.f:1.f),u,v};};
 for(bool rear:{false,true}){auto& mesh=rear?backMesh:frontMesh;mesh.reserve(columns*rows*6);for(int y=0;y<rows;++y)for(int x=0;x<columns;++x){auto a=vertex(x,y,rear),b=vertex(x+1,y,rear),c=vertex(x+1,y+1,rear),d=vertex(x,y+1,rear);for(auto point:{a,b,c,a,c,d})mesh.push_back(point);}}
 }
 target.setActive(true);target.pushGLStates();glViewport(0,0,rw,rh);glClearColor(0,0,0,0);glClearDepth(1);glDepthMask(GL_TRUE);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDisable(GL_CULL_FACE);glEnable(GL_TEXTURE_2D);glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);glDisable(GL_SCISSOR_TEST);
 float planeWidth=sourceW*std::max(1.f,roundness),left=(sourceW-planeWidth)*.5f;
 glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(left,left+planeWidth,sourceH,0,-sourceW*3,sourceW*3);glMatrixMode(GL_MODELVIEW);glLoadIdentity();glTranslatef(sourceW*.5f,0,0);glRotatef(-yaw,0,1,0);glScalef(1,1,roundness);glTranslatef(-sourceW*.5f,0,0);
 glDisableClientState(GL_COLOR_ARRAY);glEnableClientState(GL_VERTEX_ARRAY);glEnableClientState(GL_TEXTURE_COORD_ARRAY);glColor4ub(255,255,255,255);
 auto draw=[&](const sf::Texture& texture,sf::IntRect rect,const std::vector<Vertex>& mesh,bool rear){
 glMatrixMode(GL_TEXTURE);glLoadIdentity();glMatrixMode(GL_MODELVIEW);sf::Texture::bind(&texture,sf::Texture::Normalized);sf::Shader::bind(&shader);auto size=texture.getSize();
 glMatrixMode(GL_TEXTURE);if(rear){glTranslatef(float(rect.left+rect.width)/size.x,float(rect.top)/size.y,0);glScalef(-float(rect.width)/size.x,float(rect.height)/size.y,1);}else{glTranslatef(float(rect.left)/size.x,float(rect.top)/size.y,0);glScalef(float(rect.width)/size.x,float(rect.height)/size.y,1);}
 // SFML render textures have the opposite native texture orientation.
 
 glMatrixMode(GL_MODELVIEW);glVertexPointer(3,GL_FLOAT,sizeof(Vertex),&mesh.front().x);glTexCoordPointer(2,GL_FLOAT,sizeof(Vertex),&mesh.front().u);glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(mesh.size()));};
 draw(back,backRect,backMesh,true);draw(front,frontRect,frontMesh,false);sf::Shader::bind(nullptr);sf::Texture::bind(nullptr);target.popGLStates();target.display();return true;
 }
};
