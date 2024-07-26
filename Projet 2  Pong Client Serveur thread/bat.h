#pragma once
#include <SFML/Graphics.hpp>

using namespace sf;

class Bat{
private:
	Vector2f position;

	RectangleShape batShape;
	float batSpeed = 5.3f;

public:
	Bat(float startX, float startY);
	FloatRect getPosition();
	void setYPosition(float y);
	void setXPosition(float x);
	void setPosition(float x, float y);
	RectangleShape getShape();
	
	void moveUp();
	void moveDown();

	void update();
};
