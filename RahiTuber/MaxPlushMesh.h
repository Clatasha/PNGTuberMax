#pragma once
#include "MaxPlushMath.h"
#include <SFML/Graphics.hpp>
inline sf::VertexArray MaxPlushMesh(const std::vector<MaxPlushSpan>& front,const std::vector<MaxPlushSpan>& image,const sf::IntRect& rect,float roundness,float yaw,bool back,const sf::Color& tint){
 sf::VertexArray mesh(sf::Triangles);if(front.empty()||image.empty())return mesh;const int columns=96;auto arc=MaxPlushVisibleArc(roundness,yaw,back);if(!arc.valid)return mesh;
 auto vertex=[&](int row,int col){float u=-std::cos(arc.begin+(arc.end-arc.begin)*col/columns);const auto& shape=front[row];float texY=float(row)*image.size()/front.size();int texRow=std::min(int(image.size())-1,int(texY));auto span=image[texRow];if(texRow+1<int(image.size())&&span.valid&&image[texRow+1].valid){float blend=texY-texRow;span.left+=(image[texRow+1].left-span.left)*blend;span.right+=(image[texRow+1].right-span.right)*blend;}if(!span.valid)span={0,float(rect.width),true};float t=(u+1)*.5f;if(back)t=1-t;
  // A joined seam is inside the plush, so do not stretch the transparent antialias fringe across it.
  // Neutral front/rear views keep their original UVs and outlines.
  float width=span.right-span.left;float margin=width>0?std::min(3.f,width*.2f)/width*std::fabs(std::sin(yaw*.017453292519943295f)):0;
  t=margin+(1-2*margin)*t;
  float half=(shape.right-shape.left)*.5f,center=(shape.left+shape.right)*.5f;
  return sf::Vertex({MaxPlushX(center,half,u,roundness,yaw,back),float(row)},tint,{rect.left+span.left+(span.right-span.left)*t,rect.top+texY});};
 // Bound tessellation cost for large PNGs; alpha stays in the original textures.
 const int step=std::max(1,int(front.size())/256);
 for(int row=0;row<int(front.size())-1;row+=step){int next=std::min(int(front.size())-1,row+step);if(!front[row].valid||!front[next].valid)continue;for(int col=0;col<columns;col++){
  // Tessellate only the visible arc: no folded, overlapping strips at the silhouette.
  auto a=vertex(row,col),b=vertex(row,col+1),c=vertex(next,col),d=vertex(next,col+1);mesh.append(a);mesh.append(b);mesh.append(c);mesh.append(b);mesh.append(d);mesh.append(c);
 }}return mesh;
}
