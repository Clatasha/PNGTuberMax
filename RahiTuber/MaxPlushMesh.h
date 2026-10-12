#pragma once
#include "MaxPlushMath.h"
#include <SFML/Graphics.hpp>
inline sf::VertexArray MaxPlushMesh(const std::vector<MaxPlushSpan>& front,const std::vector<MaxPlushSpan>& image,const sf::IntRect& rect,float roundness,float yaw,bool back,const sf::Color& tint){
 sf::VertexArray mesh(sf::Triangles);if(front.empty()||image.empty())return mesh;const int columns=64;
 auto vertex=[&](int row,int col){float u=-std::cos(3.14159265358979323846f*col/columns);const auto& shape=front[row];int texRow=std::min(int(image.size())-1,int(float(row)*image.size()/front.size()));auto span=image[texRow];if(!span.valid)span={0,float(rect.width),true};float t=(u+1)*.5f;if(back)t=1-t;
  // A joined seam is inside the plush, so do not stretch the transparent antialias fringe across it.
  // Neutral front/rear views keep their original UVs and outlines.
  float width=span.right-span.left;float margin=width>0?std::min(3.f,width*.2f)/width*std::fabs(std::sin(yaw*.017453292519943295f)):0;
  t=margin+(1-2*margin)*t;
  float half=(shape.right-shape.left)*.5f,center=(shape.left+shape.right)*.5f;
  return sf::Vertex({MaxPlushX(center,half,u,roundness,yaw,back),float(row)},tint,{rect.left+span.left+(span.right-span.left)*t,float(rect.top+texRow)});};
 // Bound tessellation cost for large PNGs; alpha stays in the original textures.
 const int step=std::max(1,int(front.size())/256);
 for(int row=0;row<int(front.size())-1;row+=step){int next=std::min(int(front.size())-1,row+step);if(!front[row].valid||!front[next].valid)continue;for(int col=0;col<columns;col++){
  float u=-std::cos(3.14159265358979323846f*(col+.5f)/columns);if(!MaxPlushFacing(u,roundness,yaw,back))continue;
  auto a=vertex(row,col),b=vertex(row,col+1),c=vertex(next,col),d=vertex(next,col+1);mesh.append(a);mesh.append(b);mesh.append(c);mesh.append(b);mesh.append(d);mesh.append(c);
 }}return mesh;
}
