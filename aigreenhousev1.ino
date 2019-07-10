#include <dmx.h>
#include <power_mgt.h>
#include <FastLED.h>
#include <LiquidCrystal_I2C.h>
#include <dht.h>
#include <elapsedMillis.h>

#include <Bridge.h>
#include <Console.h>
#include <YunClient.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

dht DHT;

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "boss7"
#define AIO_KEY         "3358822107454ccabee8322f8e37b35a"

/************ Global State (you don't need to change this!) ******************/

// Create a BridgeClient instance to communicate using the Yun's bridge & Linux OS.
BridgeClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish iot_temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");

Adafruit_MQTT_Publish iot_humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");

Adafruit_MQTT_Publish iot_soilmoisture = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/soilmoisture");

Adafruit_MQTT_Publish iot_soilmoisturediff = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/soilmoisturediff");

Adafruit_MQTT_Publish iot_ledlights = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/ledlights");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe iot_onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");


// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe iot_waterbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/waterbutton");

#define DHT11_PIN 13
#define LED_PIN     9
#define NUM_LEDS    120
#define FASTLED_ALLOW_INTERRUPTS 0

CRGBArray<NUM_LEDS> leds;
LiquidCrystal_I2C lcd(0x27, 16, 2);
elapsedMillis timeElapsed;

String light_status = "Daylight";
int Contrast = 0.1;
int motorPin = 7;
int sensorValue = 0;
int TempMaxi = -100, TempMini = 100; // to start max/min temperature
int temp_i;
int goalTempAim = 18;
int goalTempIdeal = 20;
int goalTemp = 18;
int soilMoistureLevel = 800;
int moistureVal = 0;
int soilPin = A3;
int soilPower = 6;
int RELAY1 = A4;
int RELAY2 = A5;
int buttonPin = 7;
int motorState = 0;
int buttonState = 0;         // variable for reading the pushbutton status
int times_watered = 0;
int NUMBER_SETS = 4;  
int soilMoistureDiff = 0;
unsigned long startTime;

void setup() {

  Serial.begin(9600);


  Bridge.begin();
  Console.begin();
  Console.println(F("Adafruit MQTT demo"));

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&iot_onoffbutton);

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&iot_waterbutton);
  
  delay(500);

  startTime = millis();

  lcd.init();                   
  lcd.backlight();              

  lcd.clear();                  
  lcd.setCursor(0, 0);          

  analogWrite(6, Contrast);

 lcd.setCursor(0, 0);
 lcd.print("AI Greenhouse 0.1");

 lcd.setCursor(0, 1);
 lcd.print("Initializing...");
 
  pinMode(motorPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(soilPower, OUTPUT);
  digitalWrite(soilPower, LOW);
  
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(40);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 1500);

  for (int dot = 0; dot < NUM_LEDS; dot++) {
    leds[dot] = CRGB::Blue;
    FastLED.show();
    
    // clear this led for the next time around the loop
    leds[dot] = CRGB::Black;
    delay(30);
  }

  int chk = DHT.read11(DHT11_PIN);

  delay(5000);
  lcd.clear();

  waterPlants();
  
}

void loop() {

// Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
 Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    // Check if its the onoff button feed
    if (subscription == &iot_waterbutton) {
      Serial.print(F("On-Off button: "));
      Serial.println((char *)iot_waterbutton.lastread);
      
      if (iot_waterbutton.lastread == 1) {
       runMotor();
      }
      if (iot_waterbutton.lastread == 0) {
        digitalWrite(RELAY2, LOW);
      }
    }
  }

  

  int chk = DHT.read11(DHT11_PIN);

  Serial.print("Temperature = ");
  Serial.println(DHT.temperature);
  Serial.print("Humidity = ");
  Serial.println(DHT.humidity);


 if (! iot_temperature.publish(DHT.temperature)) {
    Console.println(F("Failed"));
  } else {
    Console.println(F("OK!"));
  }

 if (! iot_humidity.publish(DHT.humidity)) {
    Console.println(F("Failed"));
  } else {
    Console.println(F("OK!"));
  }

  if (! iot_soilmoisturediff.publish((int32_t)soilMoistureDiff)) {
    Console.println(F("Failed"));
  } else {
    Console.println(F("OK!"));
  }

  if (! iot_soilmoisture.publish((int32_t)moistureVal)) {
    Console.println(F("Failed"));
  } else {
    Console.println(F("OK!"));
  }
  
  sensorValue = analogRead(A0);
  Serial.print("Light sensor = ");
  Serial.println(sensorValue);

  int moistureSum = 0;

  buttonState = digitalRead(buttonPin);

  

  Serial.print("Button state = ");
  Serial.println(buttonState);

  if (buttonState == LOW) {


    runMotor();

  } else {

    digitalWrite(RELAY2, LOW);

  }

  if (DHT.temperature >= goalTempIdeal) {

    if ( sensorValue > 810 )
    {
      digitalWrite(RELAY1, HIGH);
    } else {
      digitalWrite(RELAY1, LOW);
    }

  } else {

    digitalWrite(RELAY1, LOW);

  }

  if (sensorValue < 200) {
//if(1==1){
    light_status = "Darkness  ";
    

//NUM_LED
for (int x = 0; x < NUM_LEDS; x++)
{
    switch (x % NUMBER_SETS)
    {
        case (0):
        {
            leds[x] = CRGB::Red;
            break;
        }
        case (1):
        {
            leds[x] = CRGB::Blue;
            break;
        }
        case (2):
        {
            leds[x] = CRGB::Red;
            break;
        }
        case (3):
        {
            leds[x] = CRGB::Red;
            break;
        }
    }
    
leds(0, 20).fill_solid(CRGB::Black);
leds(49, 70).fill_solid(CRGB::Black);
leds(100, 120).fill_solid(CRGB::Black);
    
   // Serial.println(x);
    FastLED.show();
    delay(10);
}

//    leds(0, 30).fill_solid(CRGB::Green);
//    leds(31, 60).fill_solid(CRGB::Red);
//    leds(61, 90).fill_solid(CRGB::Green);
//    leds(91, 120).fill_solid(CRGB::Red);

    FastLED.show();
    Serial.println("Darkness detected");

if (! iot_ledlights.publish((int32_t)1)) {
    Console.println(F("Failed"));
  } else {
    Console.println(F("OK!"));
  }

  } else {

    light_status = "Daylight";
    fill_solid( leds, NUM_LEDS, CRGB::Black);
    Serial.println("Daylight detected");

    if (! iot_ledlights.publish((int32_t)0)) {
    Console.println(F("Failed"));
  } else {
    Console.println(F("OK!"));
  }

  }

  FastLED.show();
  lcd.setCursor(0, 1);
  lcd.print(light_status);
  lcd.setCursor(10, 1);
  lcd.print(DHT.temperature);
  lcd.print("c");

  lcd.setCursor(10, 0);
  lcd.print(60 - (millis() - startTime) / 60000);
  lcd.print(" | ");
  lcd.print(times_watered);

  if (millis() - startTime >= (60 * 60000)) {

   lcd.clear(); 
   waterPlants();
  

    startTime = millis();
  }

  delay(5000);
}

void waterPlants(){

 int moistureSum = 0;
 moistureSum = readSoil();

    Serial.print("Soil Moisture = ");
    Serial.println(moistureSum);

int soil_difference = 0;
soil_difference = moistureSum - soilMoistureLevel;

soilMoistureDiff = soil_difference;


Serial.println(soil_difference);
    if (moistureSum < soilMoistureLevel)
    {
      lcd.setCursor(0, 0);
      lcd.print("Dry ");
      lcd.print(soil_difference);
      
      runMotor();
      

    } else {
      lcd.setCursor(0, 0);
      lcd.print("Wet ");
      lcd.print(soil_difference);

      digitalWrite(RELAY2, LOW);
    }
  
}

void runMotor()
{
  buttonState = digitalRead(buttonPin);

  if ( sensorValue > 200 || buttonState == LOW)
  {
    unsigned int interval = 2000; //ftime in ms
    unsigned long startTime = millis();

times_watered++;

    do
    {
      digitalWrite(RELAY1, LOW);
      digitalWrite(RELAY2, HIGH);
      timeElapsed = millis() - startTime;
    }
    while (timeElapsed < interval);
    digitalWrite(RELAY2, LOW);

  }
}

int readSoil()
{
  digitalWrite(soilPower, HIGH);

  delay(15000);

  moistureVal = analogRead(soilPin);

  digitalWrite(soilPower, LOW);

  return moistureVal;
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Console.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Console.println(mqtt.connectErrorString(ret));
       Console.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
  }
  Console.println("MQTT Connected!");
}





