// SimpleTx - the master or the transmitter

// wirings ===================================================

// NRL240L modules
// VCC and Ground to 3.3V and GND
// (must have a capacitor between V and G, 100uf works)
// CSN  10
// CE   9
// MOSI 11
// SCK  13
// IRQ  Not connected
// MISO 12

// Nunchuk (Wire -> pins)
// Red    GND
// Green  3.3V
// White  SCL (second from top) /A5
// Yellow SDA (first from top)  /A4
// Black  Not connected

// Dependencies ================================================

// all RF24 files
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// nunchuck stuff
#include <Wire.h>
#include <Nunchuk.h>

// Defines ======================================================

// the 2 optional pins
#define CE_PIN   9
#define CSN_PIN 10

// slave addresses, can have multiple, see robin's page
const byte slaveAddress[5] = {'R','x','A','A','A'};

RF24 radio(CE_PIN, CSN_PIN); // Create a Radio

char dataToSend[10] = "Message 0";
char txNum = '0';

// setting up sending interval structures
unsigned long currentMillis;
unsigned long prevMillis;
unsigned long txIntervalMillis = 10; // send once per second

// other functioanlity / display
const int buttonPin = 7;     // the number of the pushbutton pin
int buttonState = 0;         // variable for reading the pushbutton status

struct Data_Package{
  byte x;
  byte y;
  byte c;
  byte z;
};

// Variable of above struct
Data_Package data;

// code ================================================
void send();

void setup() {

    Serial.begin(9600);

    Serial.println("MyTx Starting");

    radio.begin();
    radio.setDataRate( RF24_250KBPS );

    // what the fuck is this
    radio.setRetries(3,5); // delay, count

    // writing pipes do not need to be closed
    radio.openWritingPipe(slaveAddress);

    // initialize the pushbutton pin as an input:
    pinMode(buttonPin, INPUT);

    // start nunchuck stuff
    Wire.begin();
    nunchuk_init();
}

void loop() {
    currentMillis = millis();
    // read the state of the pushbutton value:
    buttonState = digitalRead(buttonPin);
    //Serial.println(buttonState);
    
    if (currentMillis - prevMillis >= txIntervalMillis) {
        send();
        prevMillis = millis();
    }
}

// functions ==========================================

void send() {
    if (nunchuk_read()) {
            // Work with nunchuk_data
            //nunchuk_print();
            data.x = nunchuk_joystickX_raw();
            data.y = nunchuk_joystickY_raw(); 
            Serial.print(data.x);
            Serial.print("\t ");
            Serial.print(data.y);
            
     }

    bool rslt;
    rslt = radio.write (&data, sizeof(data));
    //rslt = radio.write( &buttonState, sizeof(buttonState) );
    // rslt = radio.write( &dataToSend, sizeof(dataToSend) );
        // Always use sizeof() as it gives the size as the number of bytes.
        // For example if dataToSend was an int sizeof() would correctly return 2

    Serial.print("\t B state: ");
    Serial.print(buttonState);
    // rslt indicates if message was recieved by anyone
    // no clue how this works
    
    if (rslt) {
        Serial.println("  Aok");
        
        // updateMessage();
    }
    else {
        Serial.println("  Not Aok");
    }
}

void updateMessage() {
        // so you can see that new data is being sent
    txNum += 1;
    if (txNum > '9') {
        txNum = '0';
    }
    dataToSend[8] = txNum;
}