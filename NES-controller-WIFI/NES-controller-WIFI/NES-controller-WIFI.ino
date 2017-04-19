#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

WiFiUDP Udp;
WiFiClient Tcp;
char packet[255];

const char* ssid = "";
const char* password = "";
//IPAddress remoteServer(10, 51, 11, 116);
IPAddress remoteServer(10, 51, 11, 127);
unsigned int remotePort = 7869;

//Static config information to send to the server to allow it to understand the data format we will use
//I used http://www.jsoneditoronline.org/ to help design this, but any JSON editor will work
//Then ran it through http://www.freeformatter.com/json-formatter.html with the javascript escape filter to escape all the quotes quickly.
const char* jsonString = "{\"name\":\"NES-004\",\"controller\":{\"dataSizeBits\":8,\"inputs\":{\"0\":{\"name\":\"dpad0RIGHT\",\"displayName\":\"Right\",\"oppositeDirection\":\"dpad0LEFT\",\"type\":\"dpadBoolean\",\"activeValue\":true,\"dataSizeBits\":1},\"1\":{\"name\":\"dpad0LEFT\",\"displayName\":\"Left\",\"oppositeDirection\":\"dpad0RIGHT\",\"type\":\"dpadBoolean\",\"activeValue\":true,\"dataSizeBits\":1},\"2\":{\"name\":\"dpad0DOWN\",\"displayName\":\"Down\",\"oppositeDirection\":\"dpad0Up\",\"type\":\"dpadBoolean\",\"activeValue\":true,\"dataSizeBits\":1},\"3\":{\"name\":\"dpad0UP\",\"displayName\":\"Up\",\"oppositeDirection\":\"dpad0DOWN\",\"type\":\"dpadBoolean\",\"activeValue\":true,\"dataSizeBits\":1},\"4\":{\"name\":\"buttonA\",\"displayName\":\"A\",\"type\":\"buttonBoolean\",\"activeValue\":true,\"dataSizeBits\":1},\"5\":{\"name\":\"buttonB\",\"displayName\":\"B\",\"type\":\"buttonBoolean\",\"activeValue\":true,\"dataSizeBits\":1},\"6\":{\"name\":\"START\",\"displayName\":\"Start\",\"type\":\"buttonBoolean\",\"activeValue\":true,\"dataSizeBits\":1},\"7\":{\"name\":\"SELECT\",\"displayName\":\"Select\",\"type\":\"buttonBoolean\",\"activeValue\":true,\"dataSizeBits\":1}}}}";

unsigned long mTime;
const uint8_t nullStr[] = {0x00};

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

#define RESEND    2   //each press is resent up to this many times

//store the last state of the controller to detect only changes in state
byte last = 0xff;

byte sent = 0;

void setup() {
  //First we setup serial for debug/input as needed.
  Serial.begin(115200);
  Serial.println("");
  Serial.println("ESP8266 Controller: Online:");

  //Setup the pins as required:
  pinMode(CLOCK, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(DATA, INPUT);

  //set all low
  digitalWrite(CLOCK, LOW);
  digitalWrite(LATCH, LOW);

  //connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  //Contact the server, send/recv configs
  if( Tcp.connect(remoteServer, remotePort) ) {
    tcpStatusUpdate();
  } else {
    Serial.print(millis()); //timestamp from boot
    Serial.println(": Failed to connect to TCP server");
  }

  
  //initialize the time variable with the current millis value
  mTime = millis();
}

void loop() {
  byte controller = readControllerNES();

  if (last != controller) {
    serialPrintStatus(controller);

    last = controller;

    udpControllerUpdate(controller);

    sent = 1;
  } else if (sent < RESEND) {
    //resend the last controller state a few times if no new status came in
    sent++;
    udpControllerUpdate(controller);
  }
  
  /*if (mTime + 30000 < millis()){
    //more then 30 seconds since last tcp send
    //TODO: make that configurable by server
    //TODO: take into account number wrapping over
    tcpStatusUpdate();
    
    mTime = millis();
  }*/
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

void udpControllerUpdate(byte controller) {
  //send the controller status to the server
  packet[0] = controller;

  Udp.beginPacket(remoteServer, remotePort);
  Udp.write(packet, 1);
  Udp.endPacket();
}

void tcpStatusUp() {
  //Print on serial the status
  Serial.print(millis()); //timestamp from boot
  Serial.print(": ");
  Serial.print("Sending TCP Update: ");

  //Send a status message to the TCP side to let it know we are still here
  //TODO: come up with a better status message
  Tcp.print("Time: ");
  Tcp.println(millis());
  Tcp.write(nullStr, sizeof(nullStr));
  
  Serial.println("Done TCP Update");
}

void tcpStatusUpdate() {
  //And send/recieve configs with server
  
  Serial.print(millis()); //timestamp from boot
  Serial.println(": Connected to TCP server, sending config");
  
  Tcp.print(jsonString);
  Tcp.write(nullStr, sizeof(nullStr));

  Serial.print(millis()); //timestamp from boot
  Serial.println(": Done sending TCP data");

  //See if we get a response
  String back = Tcp.readStringUntil('\x00');
  Serial.print(millis());
  Serial.print(": Result: ");
  Serial.println(back);

  //TODO: decode return data/back
}

void serialPrintStatus(byte controller) {
  //Do serial output of the controller status
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
}

