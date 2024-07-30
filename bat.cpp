#include "bat.h"

Bat::Bat(float startX, float startY){

	position.x = startX;
	position.y = startY;

	batShape.setSize(sf::Vector2f(50,100));
	batShape.setPosition(position);

}

FloatRect Bat::getPosition()
{
    return batShape.getGlobalBounds();
}
 
RectangleShape Bat::getShape()
{
    return batShape;
}
 
void Bat::moveDown()
{
    position.y += batSpeed;
}
 
void Bat::moveUp()
{
    position.y -= batSpeed;
}
 
void Bat::update()
{
    batShape.setPosition(position);
}
void Bat::setYPosition(float y){
	position.y = y;
}
void Bat::setXPosition(float x){
    position.x = x;
}
void Bat::setPosition(float x, float y){
    position.x = x;
    position.y = y;
    batShape.setPosition(position);
}

