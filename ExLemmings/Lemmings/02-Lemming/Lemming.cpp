#include <cmath>
#include <iostream>
#include <algorithm>
#include <GL/glew.h>
#include <GL/glut.h>
#include "Lemming.h"
#include "Game.h"

void Lemming::loadSpritesheet(string filename, int NUM_FRAMES,  int NUM_ANIMS, const glm::vec2& position) {
	_spritesheet.loadFromFile(filename, TEXTURE_PIXEL_FORMAT_RGBA);
	_spritesheet.setMinFilter(GL_NEAREST);
	_spritesheet.setMagFilter(GL_NEAREST);
	_sprite = Sprite::createSprite(glm::ivec2(_spritesheet.width() / NUM_FRAMES, _spritesheet.height() / NUM_ANIMS), glm::vec2(1.0f / float(NUM_FRAMES), 1.0f / float(NUM_ANIMS)), &_spritesheet, &_shaderProgram);
	_sprite->setNumberAnimations(NUM_ANIMS);

	for (int i = 0; i < NUM_ANIMS; i++) {
		_sprite->setAnimationSpeed(i, 6);
	}
	float height = 1.0f / float(NUM_ANIMS);
	for (int frame = 0; frame < NUM_FRAMES; frame++) {
		float num_frame = float(frame) / float(NUM_FRAMES);
		for (int anim = 0; anim < NUM_ANIMS; anim++) {
			_sprite->addKeyframe(anim, glm::vec2(num_frame, float(anim)*height));
		}
	}
	_sprite->setPosition(position);
}

void Lemming::init(const glm::vec2 &initialPosition, ShaderProgram &shaderProgram)
{
	_state = FALLING_RIGHT;
	_shaderProgram = shaderProgram;
	loadSpritesheet("images/lemming.png", 8, 4, initialPosition);
	_sprite->changeAnimation(FALLING_RIGHT_ANIM);
}

void Lemming::update(int deltaTime)
{
	int fall;

	if(_dead || _sprite->update(deltaTime) == 0)
		return;
	bool canDescend;
	glm::vec2 ori;
	switch(_state)
	{
	case FALLING_LEFT:
		fall = collisionFloor(2);
		if(fall > 0)
			_sprite->position() += glm::vec2(0, fall);
		else{
			_state = WALKING_LEFT;
			_sprite->changeAnimation(WALKING_LEFT_ANIM);
		}

		break;
	case FALLING_RIGHT:
		fall = collisionFloor(2);
		if(fall > 0)
			_sprite->position() += glm::vec2(0, fall);
		else{
			_state = WALKING_RIGHT;
			_sprite->changeAnimation(WALKING_RIGHT_ANIM);
		}
			
		break;
	case WALKING_LEFT:
		canDescend = true;
		ori = _sprite->position(); //save original position
		_sprite->position() += glm::vec2(-1, -3); //try to put the lemming up top a 3 pixel climb
		while (collision() && canDescend)
		{
			//while the position is a collision, try a position lower
			_sprite->position() += glm::vec2(0, 1);
			//if we can not move more positions in a single movement
			if (_sprite->position().y - ori.y > 3) canDescend = false;
		}
		if (!canDescend) {
			_sprite->position() = ori;
			_sprite->changeAnimation(WALKING_RIGHT_ANIM);
			_state = WALKING_RIGHT;
		}
		else
		{
			fall = collisionFloor(7);

			_sprite->position() += glm::vec2(0, fall);
			if (fall > 6){
				_state = FALLING_LEFT;
				_sprite->changeAnimation(FALLING_LEFT_ANIM);
			}
		}
		break;
	case WALKING_RIGHT:
		canDescend = true;
		ori = _sprite->position(); //save original position
		_sprite->position() += glm::vec2(1, -3); //try to put the lemming up top a 3 pixel climb
		while(collision() && canDescend)
		{
			//while the position is a collision, try a position lower
			_sprite->position() += glm::vec2(0, 1);
			//if we can not move more positions in a single movement
			if (_sprite->position().y - ori.y > 3) canDescend = false;
		}
		if (!canDescend) {
			_sprite->position() = ori;
			_sprite->changeAnimation(WALKING_LEFT_ANIM);
			_state = WALKING_LEFT;
		}
		else
		{
			fall = collisionFloor(7);
			_sprite->position() += glm::vec2(0, fall);
			if (fall > 6) {
				_state = FALLING_RIGHT;
				_sprite->changeAnimation(FALLING_RIGHT_ANIM);
			}
		}
		break;
	case EXPLODING:
		++_framesFromStart;
		if (_framesFromStart >= 16) pop();
		break;
	case BASH_LEFT:
		++_framesFromStart;
		_framesFromStart %= 32;
		if (_framesFromStart == 31) _sprite->position() += glm::vec2(-1, 0);
		else if (_framesFromStart == 17) _sprite->position() += glm::vec2(-5, 0);
		else if (_framesFromStart == 27) _sprite->position() += glm::vec2(-3, 0);
		else if (_framesFromStart == 3 || _framesFromStart == 19) {
			int posX = floor(_sprite->position().x + 120) + 3;
			int posY = floor(_sprite->position().y) + 9;
			hole(posX, posY, 7);
		}
		break;
	case BASH_RIGHT:
		++_framesFromStart;
		_framesFromStart %= 32;
		if( _framesFromStart == 31) _sprite->position() += glm::vec2(1, 0);
		else if (_framesFromStart == 17) _sprite->position() += glm::vec2(5, 0);
		else if (_framesFromStart == 27) _sprite->position() += glm::vec2(3, 0);
		else if (_framesFromStart == 3 || _framesFromStart == 19) {
			int posX = floor(_sprite->position().x + 120) + 10;
			int posY = floor(_sprite->position().y) + 7;
			hole(posX, posY, 7);
		}
		break;
	}
}


void Lemming::render()
{
	if (!_dead) {
		//_shaderProgram.setUniform1i("clicked", b);
		_shaderProgram.setUniform2f("center", _sprite->position().x, _sprite->position().y);

		//	.setUniformMatrix4f("modelview", modelview);
		_sprite->render();
	}
}

void Lemming::makeStopper(bool b) {
	if (b && _state != STOPPED) {
		_state = STOPPED;
		loadSpritesheet("images/stopper.png", 16, 1, _sprite->position());
		_sprite->changeAnimation(STOPPED_ANIM);
	}
	//TEST
	else if (!b && _state == STOPPED) {
		_state = WALKING_RIGHT;
		loadSpritesheet("images/lemming.png", 8, 4, _sprite->position());
		_sprite->changeAnimation(WALKING_RIGHT_ANIM);
	}
}

void Lemming::makeBomber(bool b) {
	if (b && _state != EXPLODING) {
		loadSpritesheet("images/bomber.png", 16, 1, _sprite->position());
		_state = EXPLODING;
		_framesFromStart = 0;
		_sprite->changeAnimation(EXPLODING_ANIM);
	}
	//TEST
	else if (!b && _state == EXPLODING) {
		_dead = false;
		_state = WALKING_RIGHT;
		loadSpritesheet("images/lemming.png", 8, 4, _sprite->position());
		_sprite->changeAnimation(WALKING_RIGHT_ANIM);
	}
}

void Lemming::makeBasher(bool b)
{
	if (b && _state != BASH_RIGHT && _state != BASH_LEFT) {
		loadSpritesheet("images/basher.png", 32, 2, _sprite->position());
		_state = BASH_LEFT;
		_framesFromStart = 0;
		_sprite->changeAnimation(BASH_LEFT_ANIM);
	}
	//TEST
	else if (!b && (_state == BASH_RIGHT || _state == BASH_LEFT)) {
		_state = WALKING_RIGHT;
		loadSpritesheet("images/lemming.png", 8, 4, _sprite->position());
		_sprite->changeAnimation(WALKING_RIGHT_ANIM);
	}
}



void Lemming::setMapMask(VariableTexture *mapMask)
{
	_mask = mapMask;
}

int Lemming::collisionFloor(int maxFall)
{
	bool bContact = false;
	int fall = 0;
	glm::ivec2 posBase = _sprite->position() + glm::vec2(120, 0); // Add the map displacement
	
	posBase += glm::ivec2(7, 16);
	while((fall < maxFall) && !bContact)
	{
		if((_mask->pixel(posBase.x, posBase.y+fall) == 0) && (_mask->pixel(posBase.x+1, posBase.y+fall) == 0))
			fall += 1;
		else
			bContact = true;
	}

	return fall;
}

bool Lemming::collision()
{
	glm::ivec2 posBase = _sprite->position() + glm::vec2(120, 0); // Add the map displacement
	
	posBase += glm::ivec2(7, 15);
	if((_mask->pixel(posBase.x, posBase.y) == 0) && (_mask->pixel(posBase.x+1, posBase.y) == 0))
		return false;

	return true;
}

void Lemming::hole(int posX, int posY, int radius) {
	for (int y = max(0, posY - radius); y <= min(_mask->height() - 1, posY + radius); y++)
		for (int x = max(0, posX - radius); x <= min(_mask->width() - 1, posX + radius); x++) {
			if (Utils::instance().pit_distance(posX, posY, x, y) <= radius) _mask->setPixel(x, y, 0);
		}
}

void Lemming::pop() {
	_dead = true;
	// Transform from mouse coordinates to map coordinates
	//   The map is enlarged 3 times and displaced 120 pixels
	int posX = floor(_sprite->position().x + 120)+7;
	int posY = floor(_sprite->position().y)+16;
	hole(posX, posY, 5);
}



