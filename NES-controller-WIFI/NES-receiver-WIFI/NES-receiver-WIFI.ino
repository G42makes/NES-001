#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "";
const char* password = "";
unsigned int localPort = 7869;

WiFiUDP Udp;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

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

volatile byte controller = 0x00;
volatile byte current = controller;
volatile byte currentBit = 0;

void setup() {
  //First we setup serial for debug/input as needed.
  Serial.begin(115200);
  Serial.println("");
  Serial.println("ESP8266 Receiver: Online:");

  //Setup the pins as required:
  pinMode(CLOCK, INPUT);
  pinMode(LATCH, INPUT);
  pinMode(DATA, OUTPUT);

  //set to high, not pressed
  digitalWrite(DATA, HIGH);

  //connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Attaching ISR");
  attachInterrupt(digitalPinToInterrupt(LATCH), latchISR, RISING);
  attachInterrupt(digitalPinToInterrupt(CLOCK), clockISR, RISING);

  //Start listening on UDP
  Udp.begin(localPort);
}

void loop() {

  //here we just read packets a lot
  //TODO: we will make this more discerning going forward, for now accept anything anytime
  int packetSize = Udp.parsePacket();
  if(packetSize) {
    //we have data
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    controller = packetBuffer[0];
  }
}

//need an ISR for the latch pin
void latchISR() {
  //grab a copy of the current controller status
  //We invert it(~) because NES buttons are active LOW(ie 0 is active, 1 is inactive)
  // but we prefer to transmit with the more logical active HIGH(1 is active, 0 is inactive)
  current = ~controller;

  //Since we will be clocking out our data using an interrupt, we need to keep track of our bits
  currentBit = 7;

  //Set DATA state.
  // Shift right by 'currentBit', use the '& 1' to filter down to just one bit
  digitalWrite(DATA, current >> currentBit & 1);
}

//ISR for clock hits
void clockISR() {
  //We should be setup by a latch already, so just work from here
  //To avoid trying to go past the 8 bits we have, we skip if already 0
  //The '--currentBit' decrements the value, puts it back and then uses it,
  //  if you do the -- after, it uses the number and then reduces it
  //Otherwise, the same as in the latch ISR
  if (currentBit) digitalWrite(DATA, current >> --currentBit & 1);
}

