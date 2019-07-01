#include <dmx.h>
#include <power_mgt.h>
#include <FastLED.h>
#include <LiquidCrystal_I2C.h>
#include <dht.h>
#include <elapsedMillis.h>

dht DHT;

#define DHT11_PIN 13
#define LED_PIN     9
#define NUM_LEDS    120

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
int soilMoistureLevel = 400;
int moistureVal = 0;
int soilPin = A3;
int soilPower = 6;
int RELAY1 = A4;
int RELAY2 = A5;
int buttonPin = 7;
int motorState = 0;
int buttonState = 0;         // variable for reading the pushbutton status

unsigned long startTime;

void setup() {

  Serial.begin(9600);
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
  FastLED.setBrightness(15);
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

  int chk = DHT.read11(DHT11_PIN);

  Serial.print("Temperature = ");
  Serial.println(DHT.temperature);
  Serial.print("Humidity = ");
  Serial.println(DHT.humidity);

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

    if ( sensorValue > 200 )
    {
      //digitalWrite(RELAY1, HIGH);
    } else {
      //digitalWrite(RELAY1, LOW);
    }

  } else {

    digitalWrite(RELAY1, LOW);

  }

  if (sensorValue < 200) {

    light_status = "Darkness  ";

    leds(0, 30).fill_solid(CRGB::Green);
    leds(31, 60).fill_solid(CRGB::Red);
    leds(61, 90).fill_solid(CRGB::Green);
    leds(91, 120).fill_solid(CRGB::Red);

    FastLED.show();
    Serial.println("Darkness detected");

  } else {

    light_status = "Daylight";
    fill_solid( leds, NUM_LEDS, CRGB::Black);
    Serial.println("Daylight detected");

  }

  FastLED.show();
  lcd.setCursor(0, 1);
  lcd.print(light_status);
  lcd.setCursor(10, 1);
  lcd.print(DHT.temperature);
  lcd.print("c");

  lcd.setCursor(10, 0);
  lcd.print((millis() - startTime) / 60000);
  lcd.print("/");
  lcd.print("60");

  if (millis() - startTime >= (60 * 60000)) {

   waterPlants();

    startTime = millis();
  }

  delay(5000);
}

void waterPlants(){

 moistureSum = readSoil();

    Serial.print("Soil Moisture = ");
    Serial.println(moistureSum);

    if (moistureSum < soilMoistureLevel)
    {
      lcd.setCursor(0, 0);
      lcd.print("Dry");

      runMotor();

    } else {
      lcd.setCursor(0, 0);
      lcd.print("Wet");

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

  delay(10);

  moistureVal = analogRead(soilPin);

  digitalWrite(soilPower, LOW);

  return moistureVal;
}
