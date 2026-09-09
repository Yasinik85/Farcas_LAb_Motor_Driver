#include "motor.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

Motor::Motor(gpio_num_t in1, gpio_num_t in2) {
    _in1 = in1;
    _in2 = in2;
    direction = Direction::stop;
    _speed = 0;

    ledc_timer_config_t timer_config{};
    timer_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    timer_config.duty_resolution = LEDC_TIMER_11_BIT;
    timer_config.timer_num = LEDC_TIMER_0;
    timer_config.freq_hz = 20000;
    timer_config.clk_cfg = LEDC_AUTO_CLK;
    timer_config.deconfigure = false;
    ledc_timer_config(&timer_config);

   ledc_channel_config_t channel_in1{};

    channel_in1.gpio_num = _in1;
    channel_in1.speed_mode = LEDC_HIGH_SPEED_MODE;
    channel_in1.channel = LEDC_CHANNEL_0;
    channel_in1.intr_type = LEDC_INTR_DISABLE;
    channel_in1.timer_sel = LEDC_TIMER_0;
    channel_in1.duty = 0;
    channel_in1.hpoint = 0;

    ledc_channel_config(&channel_in1);

    ledc_channel_config_t channel_in2{};
    channel_in2.gpio_num = _in2;
    channel_in2.speed_mode = LEDC_HIGH_SPEED_MODE;
    channel_in2.channel = LEDC_CHANNEL_1;
    channel_in2.intr_type = LEDC_INTR_DISABLE;
    channel_in2.timer_sel = LEDC_TIMER_0;
    channel_in2.duty = 0;
    channel_in2.hpoint = 0;
    
    ledc_channel_config(&channel_in2);
}
uint32_t Motor::speedToPWM(int speed)
{
    return (speed * 2047) / 100;
}

void Motor::forward(int speed) {
    uint32_t pwm = speedToPWM(speed);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, pwm);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    direction = Direction::forward;
    _speed = speed;

}
void Motor::reverse(int speed){
    uint32_t pwm= speedToPWM(speed);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, pwm);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    direction = Direction::reverse;
    _speed = speed;

};

void Motor::stop(){
   ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    direction = Direction::stop;
    _speed = 0;
}

void Motor::brake(){
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 2047);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 2047);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    direction = Direction::stop;
    _speed = 0;
}

bool Motor::isRunning(){
    if (direction == Direction::forward || direction == Direction::reverse){
        return true;
    }
    return false;
}
 
Motor::Direction Motor::getDirection(){
    return direction;
}

int Motor::getSpeed(){
    return _speed;
}

void Motor::setSpeed(int speed){
    uint32_t pwm = speedToPWM(speed);
    if (direction == Direction::forward) {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, pwm);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
        _speed = speed;
    } else if (direction == Direction::reverse) {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, pwm);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
        _speed = speed;
    } else {
        stop();
    }
}