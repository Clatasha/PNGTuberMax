#include "SpriteSheet.h"
#include "MaxPlushRaster.h"

// Duplicate saved sprite state; each copy owns fresh render targets and surface caches.
SpriteSheet::SpriteSheet(const SpriteSheet& o):_frameSizeSetting(o._frameSizeSetting),_visible(o._visible),_loop(o._loop),_offsetFromIdle(o._offsetFromIdle),_preCropSize(o._preCropSize),_cropOffset(o._cropOffset),_optimised(o._optimised),_sprite(o._sprite),_spriteSize(o._spriteSize),_gridSize(o._gridSize),_maxFrame(o._maxFrame),_currentFrame(o._currentFrame),_fps(o._fps),_frameRects(o._frameRects),_playing(o._playing),_timer(o._timer),_tex(o._tex),_texSmooth(o._texSmooth),_texMan(o._texMan),_texPath(o._texPath),_loadTimer(o._loadTimer),_loadTimeout(o._loadTimeout),_spriteUnloaded(o._spriteUnloaded),_spriteLoadFinished(o._spriteLoadFinished){if(_texMan&&!_texPath.empty()){_tex=_texMan->GetTexture(_texPath,this);if(_tex)_sprite.setTexture(*_tex,false);}}

bool SpriteSheet::PrepareDraw(bool forceVisible)
{
	sf::Time dt = _timer.getElapsedTime();

	const float frametime = 1.0f / _fps;

	if (_playing || _synced)
	{
		if (!_synced && (frametime < dt.asSeconds()))
		{
			_currentFrame++;
			_timer.restart();
			for (SpriteSheet* spr : _syncChildren)
				spr->AdvanceFrame();
		}

		if (!_frameRects.empty())
		{
			if (_loop == false && _currentFrame > _maxFrame)
			{
				_currentFrame = _maxFrame;
			}
			else
			{
				_currentFrame = _currentFrame % _frameRects.size();
				_sprite.setTextureRect(_frameRects[_currentFrame]);
			}
		}
	}
	
	if (_visible || forceVisible)
	{
		if (_spriteUnloaded)
			ReloadTexture();

		_loadTimer.restart();
        return _spriteLoadFinished && _tex;
	}
	else if (_loadTimeout > 0)
	{
		if (_loadTimeout < _loadTimer.getElapsedTime().asSeconds())
		{
			UnloadTexture();
		}
	}
    return false;
}
bool SpriteSheet::ComposeFace(){
 if(faceOverlays.empty())return false;auto rect=_sprite.getTextureRect();if(rect.width<=0||rect.height<=0)return false;
 if(_faceComposite.getSize()!=sf::Vector2u(rect.width,rect.height)&&!_faceComposite.create(rect.width,rect.height))return false;
 _faceComposite.clear(sf::Color::Transparent);sf::Sprite body(*_tex,rect);sf::RenderStates state;state.blendMode=sf::BlendMode(sf::BlendMode::One,sf::BlendMode::OneMinusSrcAlpha);_faceComposite.draw(body,state);
 for(auto& layer:faceOverlays)if(layer.texture)MaxFaceDraw(_faceComposite,*layer.texture,layer.settings,{layer.x*rect.width,layer.y*rect.height,layer.width*rect.width,layer.settings.height*rect.height},true,layer.contour);
 _faceComposite.display();_faceComposite.setSmooth(_texSmooth);return true;
}
void SpriteSheet::Draw(sf::RenderTarget* target,const sf::RenderStates& states){if(!PrepareDraw())return;if(!ComposeFace()){DrawSurface(target,_sprite,states);return;}sf::Sprite composed(_faceComposite.getTexture());composed.setColor(_sprite.getColor());auto face=states;face.transform*=_sprite.getTransform();DrawSurface(target,composed,face);}
void SpriteSheet::DrawSurface(sf::RenderTarget* target,const sf::Sprite& sprite,const sf::RenderStates& states){
 if(!rigBones||rigBones->empty()){target->draw(sprite,states);return;}
 sf::RenderStates meshState=states; meshState.transform*=sprite.getTransform();meshState.texture=sprite.getTexture();
 auto rect=sprite.getTextureRect();const int grid=24;sf::VertexArray mesh(sf::Triangles,grid*grid*6);
 // Flat sprites already carry the layer transform; their mesh coordinates must remain local.
 if(&sprite==&_sprite){meshState=states;meshState.transform*=_sprite.getTransform();meshState.texture=_sprite.getTexture();}
 for(int y=0;y<grid;++y)for(int x=0;x<grid;++x){auto make=[&](int a,int b){float u=float(a)/grid,v=float(b)/grid;
 auto raw=(&sprite==&_sprite)?sf::Vector2f(u*rect.width,v*rect.height):sprite.getTransform().transformPoint(u*rect.width,v*rect.height);
 auto p=MaxRigWarp({raw.x/std::max(1.f,_spriteSize.x),raw.y/std::max(1.f,_spriteSize.y)},*rigBones,rigPose);
 auto q=sf::Vector2f(p.x*_spriteSize.x,p.y*_spriteSize.y);if(&sprite!=&_sprite)q=sprite.getTransform().getInverse().transformPoint(q);
 return sf::Vertex(q,sprite.getColor(),{rect.left+u*rect.width,rect.top+v*rect.height});};
 int i=(y*grid+x)*6;mesh[i]=make(x,y);mesh[i+1]=make(x+1,y);mesh[i+2]=make(x+1,y+1);mesh[i+3]=mesh[i];mesh[i+4]=mesh[i+2];mesh[i+5]=make(x,y+1);}
 target->draw(mesh,meshState);
}
const std::vector<MaxPlushSpan>& SpriteSheet::PlushContour(){
 auto rect=_sprite.getTextureRect();auto handle=_tex?_tex->getNativeHandle():0;
 if(handle!=_plushTextureHandle){_plushImage=_tex->copyToImage();_plushTextureHandle=handle;_plushRows.clear();}
 if(_plushRows.empty()||rect!=_plushFrame){_plushFrame=rect;auto size=_plushImage.getSize();_plushRows=MaxPlushContour(_plushImage.getPixelsPtr(),size.x,size.y,rect.left,rect.top,rect.width,rect.height);++_plushRevision;_surfaceDepth.clear();}
 return _plushRows;
}
void SpriteSheet::DrawPlush(sf::RenderTarget* target,SpriteSheet& back,float roundness,float yaw,const sf::RenderStates& states){
 if(!PrepareDraw())return;if(!back.PrepareDraw(true)){DrawSurface(target,_sprite,states);return;}
 const auto& contour=PlushContour();const auto& rear=back.PlushContour();bool composed=ComposeFace();if(composed){++_plushRevision;}auto rect=_sprite.getTextureRect(),rearRect=back._sprite.getTextureRect();
 sf::RenderStates face=states;face.transform*=_sprite.getTransform();float planeWidth=rect.width*std::max(1.f,roundness);
 auto origin=face.transform.transformPoint(0,0),x=face.transform.transformPoint(1,0)-origin,y=face.transform.transformPoint(0,1)-origin;
 auto view=target->getView().getSize();auto size=target->getSize();float sx=view.x>0?size.x/view.x:1,sy=view.y>0?size.y/view.y:1;
 float dx=std::sqrt(x.x*x.x*sx*sx+x.y*x.y*sy*sy),dy=std::sqrt(y.x*y.x*sx*sx+y.y*y.y*sy*sy);
 int width=std::clamp(int(std::ceil(planeWidth*dx/8))*8,32,1536),height=std::clamp(int(std::ceil(rect.height*dy/8))*8,32,1536);
 auto originalSize=_plushImage.getSize();MaxPlushPixels original{_plushImage.getPixelsPtr(),int(originalSize.x),int(originalSize.y),rect.left,rect.top,rect.width,rect.height,&contour};
 if(_surfaceDepth.empty()||surfaceFlatness!=_surfaceDepthFlatness){_surfaceDepth=MaxPlushDepth(original,surfaceFlatness);_surfaceDepthFlatness=surfaceFlatness;++_surfaceShapeRevision;}
 auto& frontTexture=composed?_faceComposite.getTexture():*_tex;auto frontRect=composed?sf::IntRect(0,0,rect.width,rect.height):rect;
 if(_surface.Draw(frontTexture,frontRect,*back._tex,rearRect,_surfaceDepth,_surfaceShapeRevision,roundness,yaw,width,height,*_tex,rect)){
 sf::Sprite projected(_surface.Texture());auto projectedSize=_surface.Texture().getSize();projected.setPosition((rect.width-planeWidth)*.5f,0);projected.setScale(planeWidth/projectedSize.x,float(rect.height)/projectedSize.y);projected.setColor(_sprite.getColor());DrawSurface(target,projected,face);return;}
 if(composed)_faceImage=_faceComposite.getTexture().copyToImage();
 if(width!=_projectedWidth||height!=_projectedHeight||yaw!=_projectedYaw||roundness!=_projectedRoundness||_plushRevision!=_projectedFrontRevision||back._plushRevision!=_projectedBackRevision){
  auto fs=_plushImage.getSize(),bs=back._plushImage.getSize();
  MaxPlushPixels front{_plushImage.getPixelsPtr(),int(fs.x),int(fs.y),rect.left,rect.top,rect.width,rect.height,&contour};
  if(_surfaceDepth.empty()||surfaceFlatness!=_surfaceDepthFlatness){_surfaceDepth=MaxPlushDepth(front,surfaceFlatness);_surfaceDepthFlatness=surfaceFlatness;}front.depth=&_surfaceDepth;
  if(composed){front.pixels=_faceImage.getPixelsPtr();front.textureW=rect.width;front.textureH=rect.height;front.x=front.y=0;}
  MaxPlushPixels rearImage{back._plushImage.getPixelsPtr(),int(bs.x),int(bs.y),rearRect.left,rearRect.top,rearRect.width,rearRect.height,&rear};
  auto pixels=MaxPlushRaster(front,rearImage,roundness,yaw,width,height);if(pixels.empty()){DrawSurface(target,_sprite,states);return;}
  sf::Image projected;projected.create(width,height,pixels.data());
  if(_projectedTexture.getSize()!=sf::Vector2u(width,height)){if(!_projectedTexture.loadFromImage(projected)){DrawSurface(target,_sprite,states);return;}}else _projectedTexture.update(projected);
  _projectedTexture.setSmooth(_texSmooth);_projectedWidth=width;_projectedHeight=height;_projectedYaw=yaw;_projectedRoundness=roundness;_projectedFrontRevision=_plushRevision;_projectedBackRevision=back._plushRevision;
 }
 sf::Sprite projected(_projectedTexture);projected.setPosition((rect.width-planeWidth)*.5f,0);projected.setScale(planeWidth/width,float(rect.height)/height);projected.setColor(_sprite.getColor());DrawSurface(target,projected,face);
}

void SpriteSheet::Tick()
{
	if (_loadTimeout > 0 && _spriteUnloaded == false)
	{
		if (_loadTimeout < _loadTimer.getElapsedTime().asSeconds())
		{
			UnloadTexture();
		}
	}
}

void SpriteSheet::LoadFromTexture(TextureManager* texMan, const std::string& texPath, int frameCount, int gridX, int gridY, float fps, const sf::Vector2f& size, std::string* errorMsg)
{
	bool autoSize = size == sf::Vector2f(-1, -1);

	if (texMan == nullptr)
		return;

	sf::Texture* tex = texMan->GetTexture(texPath, (void*)this, errorMsg);
	if (tex == nullptr)
		return;

	_tex = tex;
    _plushTextureHandle=0;_plushRows.clear();
	_texMan = texMan;
	_texPath = texPath;

	if(autoSize)
		_sprite.setTexture(*tex, true);

	SetAttributes(frameCount, gridX, gridY, fps, size);

	if(!autoSize)
		_sprite.setTexture(*tex, false);

	_spriteLoadFinished = true;
}

void SpriteSheet::SetAttributes(int frameCount, int gridX, int gridY, float fps, const sf::Vector2f& size)
{
	_fps = fps;

	gridX = std::max(1, gridX);
	gridY = std::max(1, gridY);
	frameCount = std::max(1, frameCount);

	_gridSize = { gridX, gridY };
	sf::Vector2f frameSize(size);

	if (frameSize == sf::Vector2f(-1, -1))
	{
		if (_sprite.getTexture() == nullptr)
			return;

		sf::Vector2u texSize = _sprite.getTexture()->getSize();
		frameSize = sf::Vector2f((float)texSize.x / gridX, (float)texSize.y / gridY);
	}

	_frameRects.clear();

	int fCount = 0;
	for (int y = 0; y < gridY && fCount < frameCount; y++)
	{
		for (int x = 0; x < gridX && fCount < frameCount; x++)
		{
			_frameRects.push_back(sf::IntRect(x * frameSize.x, y * frameSize.y, frameSize.x, frameSize.y));
			fCount++;
		}
	}

	_sprite.setTextureRect(_frameRects[0]);
	_currentFrame = 0;
	_timer.restart();
	_maxFrame = _frameRects.size() - 1;
	_spriteSize = frameSize;

	_playing = true;
}

void SpriteSheet::UpdateSize()
{
	if (FrameCount() > 1)
		return;
	if (_sprite.getTexture() == nullptr)
		return;

	SetAttributes(1, 1, 1, 1);
}

void SpriteSheet::UnloadTexture()
{
	if (_texMan == nullptr || _spriteUnloaded)
		return;

	_spriteUnloaded = true;
	_spriteLoadFinished = false;

	_visible = false;
	_tex = nullptr;
	_sprite.setTexture(*_texMan->GetIcon(TextureManager::ICON_EMPTY));
	_texMan->UnloadTexture(_texPath, (void*)this);
	

}

void SpriteSheet::ReloadTexture(bool force, const std::string& updatePath)
{
    _plushTextureHandle=0;_plushRows.clear();
	if (!force && (_texMan == nullptr || !_spriteUnloaded))
		return;

	_spriteUnloaded = false;
	_loadingThread = new std::thread([&]
		{
			bool texChanged = false;
			if (updatePath != _texPath)
			{
				_texPath = updatePath;
				texChanged = true;
			}

			auto tex = _texMan->GetTexture(_texPath, (void*)this);
			_tex = tex;
			_tex->setSmooth(_texSmooth);
			_sprite.setTexture(*tex, texChanged);

			if (texChanged)
			{
				SetAttributes(_maxFrame, _gridSize.x, _gridSize.y, _fps, {-1,-1});
			}

			_spriteLoadFinished = true;

		});
}

bool SpriteSheet::HasTexture()
{
	if (_tex != nullptr)
		return true;

	if (_spriteUnloaded && _texPath != "")
		return true;

	return false;
}

void SpriteSheet::Clear()
{
	_texPath = "";
	_tex = nullptr;
	_spriteSize = { 0,0 };
	_gridSize = { 1,1 };
	
	_sprite = sf::Sprite();
}
