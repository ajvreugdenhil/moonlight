#include <Arduino.h>

#define LIGHT_PIN_RED D0
#define LIGHT_PIN_ORANGE D1
#define LIGHT_PIN_GREEN D2
#define LIGHT_PIN_BLUE D3
#define LIGHT_PIN_WHITE D4
#define LIGHTCOUNT 5
#define LIGHT_BLINK_SPEED 700

enum light_state
{
  OFF,
  ON,
  BLINKING
};

struct light
{
  int pin;
  light_state state;
  int intensity;
};

light light_red;
light light_orange;
light light_green;
light light_blue;
light light_white;
light lights[LIGHTCOUNT];
unsigned long previous_light_millis;
bool blink_state;

void setup()
{
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_BUILTIN_AUX, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_BUILTIN_AUX, HIGH);

  light_red.pin = LIGHT_PIN_RED;
  light_orange.pin = LIGHT_PIN_ORANGE;
  light_green.pin = LIGHT_PIN_GREEN;
  light_blue.pin = LIGHT_PIN_BLUE;
  light_white.pin = LIGHT_PIN_WHITE;

  light_red.intensity = 128;
  light_orange.intensity = 128;
  light_green.intensity = 255;
  light_blue.intensity = 32;
  light_white.intensity = 32;

  lights[0] = light_red;
  lights[1] = light_orange;
  lights[2] = light_green;
  lights[3] = light_blue;
  lights[4] = light_white;

  for (int i = 0; i < LIGHTCOUNT; i++)
  {
    pinMode(lights[i].pin, OUTPUT);
    digitalWrite(lights[i].pin, HIGH);
    lights[i].state = ON;
  }

  for (int i = 0; i < LIGHTCOUNT; i++)
  {
    delay(500);
    digitalWrite(lights[i].pin, LOW);
    lights[i].state = BLINKING;
  }

  previous_light_millis = millis();
  blink_state = false;
}

void update_lights()
{
  unsigned long current_millis = millis();
  if (current_millis > (previous_light_millis + LIGHT_BLINK_SPEED))
  {
    blink_state = !blink_state;

    for (int i = 0; i < LIGHTCOUNT; i++)
    {
      if (lights[i].state == ON || (lights[i].state == BLINKING && blink_state))
      {
        analogWrite(lights[i].pin, lights[i].intensity);
      }
      else
      {
        digitalWrite(lights[i].pin, LOW);
      }
    }

    previous_light_millis = current_millis;
  }
}

void loop()
{
  update_lights();
}