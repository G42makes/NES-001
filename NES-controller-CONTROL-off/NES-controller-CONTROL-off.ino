//This sketch will allow your arduino to output whatever inputs you give your NES
//  Serial ouput at 9600 is a time stamped list of input changes
//  Connect all inputs as taps to the existing data, the arduino just listens.
//  It could probably be modified to create output that is then fed back in, but this script does not do that.

//Some ref(but not the initial idea):
//    https://github.com/mattpbooth/ArduinoNESController 

//Pins
// I'm using a Pro Trinket 5V for this dev work
#define LATCH     3
#define CLOCK     5
#define DATA      6
#define P_SWITCH  4   //power switch sense(it detects the state of the power switch
#define P_CONT    9   //control the power state(via TIP102)

#define LED       8

//because I want to define stuff clearer:
#define C_NONE      0b00000000
#define C_RIGHT     0b00000001
#define C_LEFT      0b00000010
#define C_DOWN      0b00000100
#define C_UP        0b00001000
#define C_START     0b00010000
#define C_SELECT    0b00100000
#define C_B         0b01000000
#define C_A         0b10000000

//I don't like magic numbers, but for now I need this, at least until I come up with a better read process.
#define MAGIC   3   //made this up as I went... it's to give time for the clock to go low

//aparently this needs to be volatile due to being changed in an interrupt:
//    https://www.arduino.cc/en/Reference/Volatile
volatile byte controller = 0;
byte last = 0;

//store the state of the power to the NES - we assume off to start, but any latch signal will reset this
boolean power_state = false;
boolean switch_state = false;
unsigned long power_last_latch = 0; //used to sense power loss if the system has not latched in a while

//store the LED state
boolean led = false;
const byte flag = C_START | C_SELECT | C_UP;

void setup() {
  //First we setup serial for debug/input as needed.
  Serial.begin(9600);
  Serial.println("NES Online:");

  //allow the LED on pin 13 to be lit if needed
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  //Setup the CLOCK, DATA and P_SWITCH pins as input
  pinMode(CLOCK, INPUT);
  pinMode(DATA, INPUT);
  pinMode(P_SWITCH, INPUT);
  //Setup P_CONT as output
  pinMode(P_CONT, OUTPUT);

  //Link pin LATCH as an interupt input
  pinMode(LATCH, INPUT);
  //Trigger only when the pin falls low: FALLING
  attachInterrupt(digitalPinToInterrupt(LATCH), latchIN, FALLING);
  //see: https://www.arduino.cc/en/Reference/AttachInterrupt 
}

void loop() {
  if (last != controller) {
    Serial.print(millis());
    Serial.print(": ");
    if (controller & C_LEFT)    Serial.print("LEFT   : "); else Serial.print("       : ");
    if (controller & C_RIGHT)   Serial.print("RIGHT  : "); else Serial.print("       : ");
    if (controller & C_UP)      Serial.print("UP     : "); else Serial.print("       : ");
    if (controller & C_DOWN)    Serial.print("DOWN   : "); else Serial.print("       : ");
    if (controller & C_A)       Serial.print("A      : "); else Serial.print("       : ");
    if (controller & C_B)       Serial.print("B      : "); else Serial.print("       : ");
    if (controller & C_START)   Serial.print("START  : "); else Serial.print("       : ");
    if (controller & C_SELECT)  Serial.print("SELECT : "); else Serial.print("       : ");
    if (controller == 0 )       Serial.print("NONE   : "); else Serial.print("       : ");
    
    Serial.print(": 0x");
    Serial.print(controller, HEX);
    Serial.print(": D: ");
    Serial.print(controller);
    
    Serial.println("");
    
    last = controller;
  } else {
    //if the controller does two in a row, check if it's our special combo
    if ((controller & flag) == flag) {
      Serial.print(millis());
      Serial.println(": Toggle Power Requested");
      led = !led; //invert the current value
      digitalWrite(LED, led);
      power_state = !power_state;
      digitalWrite(P_CONT, power_state);
      controller = 0; //we need to clear the state of the 'controller' as there will be no interrupt to read differently
      delay(2000); // so that we don't toggle it, probaby should be longer
    }
  }
  if (switch_state != digitalRead(P_SWITCH)) {
    switch_state = !switch_state;
    Serial.print(millis());
    Serial.print(": Power Switch State: ");
    if (switch_state) Serial.println("ON"); else Serial.println("OFF");
    digitalWrite(LED, switch_state);
    if (switch_state != power_state) {
      Serial.println("Change in power state due to power switch.");
      power_state = !power_state;
      digitalWrite(P_CONT, power_state);
    }
    delay(100);   //debounce
  }
  //delay(500);
}

//Read the state of the controller and store it.
//    http://www.mit.edu/~tarvizo/nes-controller.html
void latchIN() {
  //store the controller state internally first
  byte input = 0;

  // we waited for the latch to fall before calling the interrupt so that we can read the first bit now
  //    we would have to wait for LATCH to go low otherwise, here
  
  //read 8 bits on the clock
  for (int i = 7; i >= 0; i--) {
    //pull the status of the DATA pin and shift it over to match position of the bit
    //the value is active low, so we should get all 1's when nothing is happening
    input += digitalRead(DATA) << i;

    //delay a bit after this, in theory we should do a digital read until low,
    //    but it's too slow and causes big issues
    //This magic number was found experimentally, and technically could cause problems if the game
    //    uses a non-standard timing to read the state, which is allowed. Most games should get the
    //    controller state as fast as possible, so I suspect this will be fine for now
    //TODO: resolve this somehow - I do get spurious data sometimes due to this flaw
    delayMicroseconds(MAGIC); 
    //wait for the clock to go back low, so we don't read the next bit instantly
    while(digitalRead(CLOCK) != HIGH) {}
  }

  // we assign the value to the global, but inverted so that 1 is pressed, and 0 is not
  controller = ~input;
}

