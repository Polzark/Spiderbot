// SimpleRx - the slave or the receiver

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>
#include <data.cpp>
// #include <movement.cpp>
#include <anatomy.cpp>

#include <FastLED_NeoPixel.h>

// Which pin on the Arduino is connected to the LEDs?
#define DATA_PIN 3

// How many LEDs are attached to the Arduino?
#define NUM_LEDS 4

// LED brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 18

// Amount of time for each half-blink, in milliseconds
#define BLINK_TIME 300

// LED orders for industry night
// 1: pcb, 2: top eye, 3: left eye, 4: right eye

FastLED_NeoPixel<NUM_LEDS, DATA_PIN, NEO_GRB> strip;      // <- FastLED NeoPixel

#define CE_PIN   48
#define CSN_PIN 49

const byte thisSlaveAddress[5] = {'R','x','A','A','A'};

RF24 radio(CE_PIN, CSN_PIN);

char dataReceived[10]; // this must match dataToSend in the TX
int buttonState = 0;
bool newData = false;

const int ledPin =  4;      // the number of the LED pin

void startuplights(FastLED_NeoPixel<NUM_LEDS, DATA_PIN, NEO_GRB> strip);
void setstrip(FastLED_NeoPixel<NUM_LEDS, DATA_PIN, NEO_GRB> strip, uint32_t color);
//===========

struct Data_Package{
  byte x;
  byte y;
  byte c;
  byte z;
};

uint32_t black = strip.Color(0, 0, 0);
uint32_t red = strip.Color(0, 255, 0);
uint32_t green = strip.Color(255, 0, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t yellow = strip.Color(255, 255, 0);
uint32_t orange = strip.Color(102, 255, 0);
uint32_t purple = strip.Color(0, 128, 128);
uint32_t pink_questionmark = strip.Color(130, 255, 160);

// Variable of above struct
Data_Package data;
Servo head;
int headpin = 22;
Body bot;
void setup() {
    strip.begin();  // initialize strip (required!)
    strip.setBrightness(BRIGHTNESS);
    pinMode(25,OUTPUT);
    digitalWrite(25,HIGH);
    pinMode(ledPin, OUTPUT);
    startuplights(strip);
    for ( int i = 0; i < 4; i++) {
		strip.setPixelColor(i, green);
		delay(BLINK_TIME);
		strip.show();
    }
    head.attach(headpin);
    Serial.begin(9600);
    bot = Body(robotdata);

    Serial.println("MyRx Starting");
    radio.begin();
    radio.setDataRate( RF24_250KBPS );
    radio.openReadingPipe(1, thisSlaveAddress);
    radio.startListening();

    setstrip(strip, black);
    strip.setPixelColor(0, blue);
    strip.show();
    Serial.println("setup done");

    // bot.stance();
    // while(1!=0);
}

//=============

void getData() {
    radio.flush_rx();
    while (!radio.available()) {
        setstrip(strip, red);
    	  strip.show();
    };
        //radio.read( &dataReceived, sizeof(dataReceived) );
        //radio.read( &buttonState, sizeof(buttonState));
    radio.read( &data, sizeof(Data_Package));
    strip.setPixelColor(1, yellow);
    strip.show();

    newData = true;
}

void showData() {
    if (newData == true) {
        digitalWrite(ledPin, buttonState);
        setstrip(strip, black);
        newData = false;
        Serial.print(data.x);
        Serial.print("\t ");
        Serial.print(data.y);
        Serial.print("\t ");
        Serial.print(data.c);
        Serial.print("\t ");
        Serial.println(data.z);
    }
}

void loop() {
    getData();
    showData();
    // bot.stance();
    // bot.servoChecks();

    double heading = 0;
    double x = data.x;
    double y = data.y;
    x -= 130;
    y -= 129;

    // trig bs
    if (x != 0) {
      	heading = atan(y / x) * 180/PI;
		    setstrip(strip, green);
      	strip.show();
    } else if (y < 0) {
      	heading = 270;
		    setstrip(strip, green);
      	strip.show();
    } else if (y == 0) {
        strip.setPixelColor(2, pink_questionmark);
      	strip.setPixelColor(3, pink_questionmark);
      	strip.show();
      	bot.stance();
      	Serial.println("Standstill");
      	heading = -100;
      	// loop();
    } else {
      	heading = 90;
		    setstrip(strip, green);
      	strip.show();
    }

    if (x < 0) {
      	heading += 180;
    }
    if (heading != -100) {

      // Serial.print(x);
      // Serial.print(" ");
      // Serial.print(y);
      // Serial.print(" ");
      // Serial.print(data.c);
      // Serial.print(" ");
      // Serial.print(data.z);
      // Serial.print(" ");

      Serial.print("Heading: ");
      Serial.println(180 - heading);
      if (data.c == 1 && x != 0) {
        Serial.println("Turn in place");
        setstrip(strip, yellow);
        // heading = -100;
        strip.show();
        bot.tripodturn(x < 0);
      } else {
        head.write(max(60, min(120, 180 -heading)));
        bot.tripodgait(90 - heading);
      }
    }
}

void startuplights(FastLED_NeoPixel<NUM_LEDS, DATA_PIN, NEO_GRB> strip) {
  // int blinkdelay = 250;
  for (int i = 0; i <3;i++){
    setstrip(strip, black);
    strip.setPixelColor(i+1,orange);
    strip.show();
    delay(BLINK_TIME);
  }
  setstrip(strip, black);
}

void setstrip(FastLED_NeoPixel<NUM_LEDS, DATA_PIN, NEO_GRB> strip, uint32_t color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
  }
}
//==============
