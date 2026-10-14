#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
// Distances are measured inward from exterior transparency. Enclosed holes
// are kept unchanged, and RGB remains premultiplied with the new alpha.
inline std::vector<float> MaxSpriteEdgeDistances(const unsigned char* rgba,int iw,int ih,int ox,int oy,int w,int h){
 if(!rgba||w<=0||h<=0||ox<0||oy<0||ox+w>iw||oy+h>ih)return {};
 int pw=w+2,ph=h+2,n=pw*ph;std::vector<unsigned char> exterior(n);std::vector<int> queue;queue.reserve(n);exterior[0]=1;queue.push_back(0);
 auto transparent=[&](int x,int y){return x==0||y==0||x==pw-1||y==ph-1||rgba[((oy+y-1)*iw+ox+x-1)*4+3]==0;};
 for(std::size_t i=0;i<queue.size();++i){int a=queue[i],x=a%pw,y=a/pw;auto visit=[&](int b,int xx,int yy){if(!exterior[b]&&transparent(xx,yy)){exterior[b]=1;queue.push_back(b);}};if(x)visit(a-1,x-1,y);if(x+1<pw)visit(a+1,x+1,y);if(y)visit(a-pw,x,y-1);if(y+1<ph)visit(a+pw,x,y+1);}
 std::vector<float> work(n),line(std::max(pw,ph)),out(line.size()),limits(line.size()+1);std::vector<int> sites(line.size());
 auto transform=[&](int length){int k=-1;for(int q=0;q<length;++q){if(line[q]>=1e19f)continue;float split=-1e20f;while(k>=0){int v=sites[k];split=((line[q]+float(q)*q)-(line[v]+float(v)*v))/(2.f*(q-v));if(split>limits[k])break;--k;}++k;sites[k]=q;limits[k]=k?split:-1e20f;limits[k+1]=1e20f;}int cursor=0;for(int q=0;q<length;++q){while(cursor<k&&limits[cursor+1]<q)++cursor;float d=float(q-sites[cursor]);out[q]=d*d+line[sites[cursor]];}};
 for(int y=0;y<ph;++y){for(int x=0;x<pw;++x)line[x]=exterior[y*pw+x]?0.f:1e20f;transform(pw);for(int x=0;x<pw;++x)work[y*pw+x]=out[x];}
 for(int x=0;x<pw;++x){for(int y=0;y<ph;++y)line[y]=work[y*pw+x];transform(ph);for(int y=0;y<ph;++y)work[y*pw+x]=out[y];}
 std::vector<float> result(w*h);for(int y=0;y<h;++y)for(int x=0;x<w;++x)result[y*w+x]=std::max(0.f,std::sqrt(work[(y+1)*pw+x+1])-.5f);return result;
}
inline void MaxApplySpriteEdges(unsigned char* rgba,int iw,int ih,int ox,int oy,int w,int h,const std::vector<float>& distance,float feather,float trim){
 if(!rgba||distance.size()!=std::size_t(w)*h||ox<0||oy<0||ox+w>iw||oy+h>ih)return;
 feather=std::isfinite(feather)?std::clamp(feather,0.f,256.f):0;trim=std::isfinite(trim)?std::clamp(trim,0.f,256.f):0;if(feather==0&&trim==0)return;
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){auto p=rgba+((oy+y)*iw+ox+x)*4;if(!p[3]){p[0]=p[1]=p[2]=0;continue;}float d=distance[y*w+x]-trim,t=std::clamp(d/std::max(1.f,feather),0.f,1.f),fade=feather>0?t*t*(3-2*t):t;for(int c=0;c<4;++c)p[c]=static_cast<unsigned char>(p[c]*fade+.5f);}
}
