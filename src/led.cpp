#include "Arduino.h"
#include "driver/ledc.h"
#include "config.h"

#define PWM_CHANNEL 2
#define PWM_FREQ 500
#define PWM_RESOLUTION 8
#define LED_MAX_DUTY 30

static int current_led_duty;

void status_led_fade(void)
{
    static int current_led_duty;

    if (current_led_duty < LED_MAX_DUTY)
        current_led_duty++;
    else
        current_led_duty= -LED_MAX_DUTY;
    ledcWrite(PWM_CHANNEL, abs(current_led_duty));
}

void status_led_on(void)
{
    ledcWrite(PWM_CHANNEL, LED_MAX_DUTY);
}

void status_led_off(void)
{
    ledcWrite(PWM_CHANNEL, 0);
}


void init_status_led(void)
{
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PUMP_LED, PWM_CHANNEL);
    current_led_duty = 0;
}