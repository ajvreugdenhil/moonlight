#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#define LIGHT_PIN_RED D0
#define LIGHT_PIN_ORANGE D1
#define LIGHT_PIN_GREEN D2
#define LIGHT_PIN_BLUE D3
#define LIGHT_PIN_WHITE D4
#define LIGHTCOUNT 5
#define LIGHT_BLINK_SPEED 900

#define QUERY_SPEED 5000

#define COLDTEMPERATURE 40
#define PIDERROR 3

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

struct printer_state
{
  bool heating;
  bool hot;
  bool busy;
  bool paused;
  bool warning;
  bool error;
  bool query_failure;
};

light light_red;
light light_orange;
light light_green;
light light_blue;
light light_white;
light* lights[LIGHTCOUNT];
unsigned long previous_light_millis;
bool blink_state;

unsigned long previous_query_millis;

WiFiClient client;
HTTPClient http;

void setup()
{
  Serial.begin(115200);
  WiFiManager wifiManager;
  wifiManager.autoConnect("Moonlight");

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

  lights[0] = &light_red;
  lights[1] = &light_orange;
  lights[2] = &light_green;
  lights[3] = &light_blue;
  lights[4] = &light_white;

  for (int i = 0; i < LIGHTCOUNT; i++)
  {
    pinMode(lights[i]->pin, OUTPUT);
    digitalWrite(lights[i]->pin, HIGH);
    lights[i]->state = ON;
  }

  for (int i = 0; i < LIGHTCOUNT; i++)
  {
    delay(500);
    digitalWrite(lights[i]->pin, LOW);
    lights[i]->state = OFF;
  }

  previous_light_millis = millis();
  previous_query_millis = millis();
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
      if (lights[i]->state == ON || (lights[i]->state == BLINKING && blink_state))
      {
        analogWrite(lights[i]->pin, lights[i]->intensity);
      }
      else
      {
        digitalWrite(lights[i]->pin, LOW);
      }
    }

    previous_light_millis = current_millis;
  }
}

void set_lights(printer_state state)
{
  // RED
  if (state.error)
  {
    light_red.state = ON;
  }
  else
  {
    light_red.state = OFF;
  }
  // ORANGE
  if (state.query_failure || state.warning)
  {
    light_orange.state = ON;
  }
  else
  {
    light_orange.state = OFF;
  }

  // GREEN
  if (state.busy)
  {
    light_green.state = ON;
  }
  else if (state.paused)
  {
    light_green.state = BLINKING;
  }
  else
  {
    light_green.state = OFF;
  }

  // BLUE
  if (state.heating)
  {
    light_blue.state = BLINKING;
  }
  else if (state.hot)
  {
    light_blue.state = ON;
  }
  else
  {
    light_blue.state = OFF;
  }
}

String make_request(String url)
{
  http.begin(client, url.c_str());
  http.addHeader("X-Api-Key", "5360513ee58549ada91995eca47d9aeb");
  http.GET();
  String data = http.getString();
  http.end();
  return data;
}

printer_state query_printer()
{
  printer_state result;
  result.warning = false; // not set
  
  String url = "http://192.168.2.8:7125/printer/objects/query?print_stats&heater_bed&extruder";
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, make_request(url));
  if (error)
  {
    Serial.print(F("deserializeJson() failed"));
    Serial.println(error.f_str());
    result.query_failure = true;
  }
  else
  {
    result.query_failure = false;
  }

  String state = doc["result"]["status"]["print_stats"]["state"].as<String>();
  float bed_temp = doc["result"]["status"]["heater_bed"]["temperature"].as<float>();
  float bed_target = doc["result"]["status"]["heater_bed"]["target"].as<float>();
  float tool_temp = doc["result"]["status"]["heater_bed"]["temperature"].as<float>();
  float tool_target = doc["result"]["status"]["heater_bed"]["target"].as<float>();

  if (state == "printing")
  {
    result.busy = true;
    result.paused = false;
  }
  else if (state == "paused")
  {
    result.busy = false;
    result.paused = true;
  }
  else
  {
    result.busy = false;
    result.paused = false;
  }

  result.error = (state == "error");

  if ((bed_temp < bed_target - PIDERROR) || (tool_temp <  tool_target - PIDERROR))
  {
    result.heating = true;
  }
  else
  {
    result.heating = false;
  }

  if (bed_temp > COLDTEMPERATURE || tool_temp > COLDTEMPERATURE)
  {
    result.hot = true;
  }
  else
  {
    result.hot = false;
  }

  return result;
}

void loop()
{
  if (millis() > previous_query_millis + QUERY_SPEED)
  {
    set_lights(query_printer());
    previous_query_millis = millis();
  }
  update_lights();
}
