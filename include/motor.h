#ifndef MOTOR_H
#define MOTOR_H
#include "driver/gpio.h"

class Motor{

    public:
        Motor(gpio_num_t, gpio_num_t);
        enum class Direction{
            forward, 
            reverse, 
            stop
        };
        Direction direction;
            void forward(int);
            void reverse(int);
            void setSpeed(int);
            bool isRunning();
            Direction getDirection();
            int getSpeed(); // last commanded speed, 0-100 (0 once stopped/braked)
            void stop(); // it naturally stops
            void brake(); //actively brake and oppose the direction it is moving (faster stop)
            uint32_t speedToPWM(int speed);

    private:
        gpio_num_t _in1;
        gpio_num_t _in2;
        int _speed;
};

#endif