#include "Arduino.h"
#include "config.h"

extern t_Config Config;

void beep(int len)
{
    ESP_LOGD("BEEP", "BEEP");
 
    ledcWrite(0, Config.buzzerintensity);
    delay(len);
    ledcWrite(0, 0);
}

void init_buzzer()
{
    ledcSetup(0, BUZZER_FREQ, 8);
    ledcAttachPin(BUZZER, 0);
}