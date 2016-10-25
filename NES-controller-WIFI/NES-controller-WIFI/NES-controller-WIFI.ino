#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

WiFiUDP Udp;
char packet[255];

const char* ssid = "";
const char* password = "";
IPAddress remoteUdpServer(10, 51, 11, 116);
unsigned int remoteUdpPort = 7869;

//Pins
#define CLOCK   14
#define LATCH   12
#define DATA    13

//because I want to define stuff clearer:
#define NONE      0b00000000
#define RIGHT     0b00000001
#define LEFT      0b00000010
#define DOWN      0b00000100
#define UP        0b00001000
#define START     0b00010000
#define SELECT    0b00100000
#define BUTTONB   0b01000000
#define BUTTONA   0b10000000


//store the last state of the controller to detect only changes in state
byte last = 0xff;

void setup() {
  //First we setup serial for debug/input as needed.
  Serial.begin(115200);
  Serial.println("");
  Serial.println("ESP8266: Online:");

  //Setup the pins as required:
  pinMode(CLOCK, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(DATA, INPUT);

  //set all low
  digitalWrite(CLOCK, LOW);
  digitalWrite(LATCH, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

}

void loop() {
  byte controller = readControllerNES();

  if (last != controller) {
    Serial.print(millis()); //timestamp from boot
    Serial.print(": ");
    if (controller & LEFT)    Serial.print("LEFT   : "); else Serial.print("       : ");
    if (controller & RIGHT)   Serial.print("RIGHT  : "); else Serial.print("       : ");
    if (controller & UP)      Serial.print("UP     : "); else Serial.print("       : ");
    if (controller & DOWN)    Serial.print("DOWN   : "); else Serial.print("       : ");
    if (controller & BUTTONA) Serial.print("A      : "); else Serial.print("       : ");
    if (controller & BUTTONB) Serial.print("B      : "); else Serial.print("       : ");
    if (controller & START)   Serial.print("START  : "); else Serial.print("       : ");
    if (controller & SELECT)  Serial.print("SELECT : "); else Serial.print("       : ");
    if (controller == 0 )     Serial.print("NONE   : "); else Serial.print("       : ");
    
    Serial.print("0x");
    Serial.print(controller, HEX);
    Serial.print(": D: ");
    Serial.print(controller);
    
    Serial.println("");

    last = controller;
    packet[0] = controller;

    Udp.beginPacket(remoteUdpServer, remoteUdpPort);
    Udp.write(packet, 1);
    Udp.endPacket();
  }
  
  //delay(500);
}

byte readControllerNES() {
  //store the button states in a byte(8 buttons)
  byte input = 0;
  
  //Raise the latch high for 12ms
  digitalWrite(LATCH, HIGH);
  delayMicroseconds(12);

  //Lower Latch and read in the first button state
  digitalWrite(LATCH, LOW);
  input = digitalRead(DATA) << 7;

  //Use the clock to pump the rest of the bits/button states
  for ( int i = 6; i >= 0; i-- ) {
    delayMicroseconds(6);
    digitalWrite(CLOCK, HIGH);
    delayMicroseconds(6);
    digitalWrite(CLOCK, LOW);
    input += digitalRead(DATA) << i;
  }

  return ~input;
}

