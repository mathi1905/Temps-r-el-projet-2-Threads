#pragma once
#include <SFML/Graphics.hpp>
#include <stdlib.h>
#include <time.h>
using namespace sf;
 
class Ball
{
    private:
        Vector2f position;
    
        // A RectangleShape object called ref
        RectangleShape ballShape;
    
        float xVelocity = .2f;
        float yVelocity = .2f;
    
    public:
        Ball(float startX, float startY);
        
        FloatRect getPosition();
        void start();
        RectangleShape getShape();
    
        float getXVelocity();
        Vector2f getVectorPosition();
        float getYVelocity();
    
        void reboundBat();

        void reboundTopOrBot();
    
        void hitSide(float startX, float startY);
    
        void update();

        void setPosition(float x, float y);
 
};
