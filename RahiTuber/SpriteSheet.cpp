#include "SpriteSheet.h"
#include "MaxPlushMesh.h"

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
void SpriteSheet::Draw(sf::RenderTarget* target,const sf::RenderStates& states){if(PrepareDraw())target->draw(_sprite,states);}
const std::vector<MaxPlushSpan>& SpriteSheet::PlushContour(){
 auto rect=_sprite.getTextureRect();auto handle=_tex?_tex->getNativeHandle():0;
 if(handle!=_plushTextureHandle){_plushImage=_tex->copyToImage();if(_texSmooth&&_frameRects.size()<=1)_tex->generateMipmap();_plushTextureHandle=handle;_plushRows.clear();}
 if(_plushRows.empty()||rect!=_plushFrame){_plushFrame=rect;auto size=_plushImage.getSize();_plushRows=MaxPlushContour(_plushImage.getPixelsPtr(),size.x,size.y,rect.left,rect.top,rect.width,rect.height);}
 return _plushRows;
}
void SpriteSheet::DrawPlush(sf::RenderTarget* target,SpriteSheet& back,float roundness,float yaw,const sf::RenderStates& states){
 if(!PrepareDraw())return;if(!back.PrepareDraw(true)){target->draw(_sprite,states);return;}
 const auto& contour=PlushContour();const auto& rear=back.PlushContour();
 sf::RenderStates face=states;face.transform*=_sprite.getTransform();
 // Both halves use one contour, so their left and right seams coincide at every turn angle.
 face.texture=back._tex;auto rearMesh=MaxPlushMesh(contour,rear,back._sprite.getTextureRect(),roundness,yaw,true,_sprite.getColor());target->draw(rearMesh,face);
 face.texture=_tex;auto frontMesh=MaxPlushMesh(contour,contour,_sprite.getTextureRect(),roundness,yaw,false,_sprite.getColor());target->draw(frontMesh,face);
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
