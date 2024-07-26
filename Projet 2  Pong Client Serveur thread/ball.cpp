#include "ball.h"

// This the constructor and it is called when we create an object
Ball::Ball(float startX, float startY)
{
    position.x = startX;
    position.y = startY;
 
    ballShape.setSize(sf::Vector2f(10, 10));
    ballShape.setPosition(position);
}
void Ball::start(){
/*
	srand(time(NULL));
  xVelocity = 	((double)rand())/((double)RAND_MAX) / 3.0 + 0.1;
  if(((double)rand())/((double)RAND_MAX) >=0.5)
	xVelocity = xVelocity*-1;
  yVelocity = ((double)rand())/((double)RAND_MAX) / 3.0 + 0.1;
  if(((double)rand())/((double)RAND_MAX) >=0.5)
	yVelocity = yVelocity*-1;
*/
	xVelocity = -1.2f;
	yVelocity = 1.2f;
}

FloatRect Ball::getPosition()
{
    return ballShape.getGlobalBounds();
}
 
RectangleShape Ball::getShape()
{
    return ballShape;
}
 
float Ball::getXVelocity()
{
    return xVelocity;
}

float Ball::getYVelocity()
{
    return yVelocity;
}
 
void Ball::reboundTopOrBot()
{
    yVelocity = -yVelocity;
    xVelocity = xVelocity+xVelocity*0.2;
}
 
void Ball::reboundBat()
{
    position.x -= (xVelocity * 30);
    xVelocity = -(xVelocity+xVelocity*0.2);
    // AUgmenter un peu la vélocité
 
}
 
void Ball::hitSide(float startX, float startY)
{
    position.y = startX;
    position.x = startY;
    start();
}
void Ball::setPosition(float x, float y)
{
	position.x = x;
	position.y = y;
	ballShape.setPosition(position);

}
Vector2f Ball::getVectorPosition(){
	return position;
}
void Ball::update()
{

    // Update the ball position variables
    position.y += yVelocity;
    position.x += xVelocity;
 
    // Move the ball and the bat
    ballShape.setPosition(position);
}
