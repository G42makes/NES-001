//Latch pin
#define LATCH 3

void setup() {
  //First we setup serial for debug/input as needed.
  Serial.begin(9600);
  Serial.println("NES Online");

  //Link pin LATCH as an interupt input
  pinMode(LATCH, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(LATCH), latchIN, CHANGE);
}

void loop() {
  

}

void latchIN() {
  Serial.print(millis());
  Serial.println("LATCH");
}

