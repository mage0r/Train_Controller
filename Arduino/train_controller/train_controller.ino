#include <Adafruit_NeoPixel.h>

#define NEOPIXEL A2

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(10, NEOPIXEL, NEO_GRB + NEO_KHZ800);

struct Train {
  String name;
  int train_speed;
  unsigned int train_status;
  unsigned int first; // first bar position
  unsigned int last; // last bar position.
  unsigned int bars; // total number of bars.
  unsigned int estop_pin;
  unsigned int forward_pin;
  unsigned int reverse_pin;
  unsigned int speed_pin; // sensor to read speed
  unsigned int enable_pin; // this sets the speed of the train.
  unsigned int input_1;
  unsigned int input_2;
  long update_train;
  int update_cycle;
  int slow_value; // how much it slows down per update cycle.
  
};

void bar_graph(Train change_train, Adafruit_NeoPixel thestrip, int percentage);
void changeSpeed(unsigned int train_number, int train_speed, unsigned int change_status, unsigned int change_estop );

unsigned const int trainCount = 2;
Train trains[trainCount];

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  Serial.begin( 9600 );
  Serial.println("Starting up");
  
  trains[0].name = "Train1";
  trains[0].first = 5;
  trains[0].last = 9;
  trains[0].bars = 15;
  trains[0].estop_pin = 11;
  trains[0].forward_pin = 12;
  trains[0].reverse_pin = 13;
  trains[0].speed_pin = A1;
  trains[0].enable_pin = 6;
  trains[0].input_1 = 4;
  trains[0].input_2 = 7;
  trains[0].train_status = 0;
  trains[0].update_cycle = 500;
  trains[0].slow_value = 20;
  
  trains[1].name = "Train2";
  trains[1].first = 0;
  trains[1].last = 4;
  trains[1].bars = 15;
  trains[1].estop_pin = 8;
  trains[1].forward_pin = 10;
  trains[1].reverse_pin = 9;
  trains[1].speed_pin = A0;
  trains[1].enable_pin = 5;
  trains[1].input_1 = 3;
  trains[1].input_2 = 2;
  trains[1].train_status = 0;
  trains[1].update_cycle = 500;
  trains[1].slow_value = 20;
 
  for(int i = 0; i < trainCount; i++) {
    pinMode( trains[i].enable_pin, OUTPUT );  // Train enable
    pinMode( trains[i].input_1, OUTPUT ); // Train input 1
    pinMode( trains[i].input_2, OUTPUT ); // Train input 2
    analogWrite( trains[i].enable_pin, 0); // Make sure the train is off
    digitalWrite( trains[i].input_1, LOW );
    digitalWrite( trains[i].input_2, LOW );
    pinMode(trains[i].speed_pin, INPUT); // set the speed pin to be input
    pinMode(trains[i].estop_pin, OUTPUT); // set the speed pin to be input
    pinMode(trains[i].forward_pin, OUTPUT); // set the speed pin to be input
    pinMode(trains[i].reverse_pin, OUTPUT); // set the speed pin to be input
    trains[i].update_train = millis();
    trains[i].train_speed = map(analogRead(trains[i].speed_pin),0,1024,255,0);
  }
  
}

void loop() {
   
  for(int i = 0; i < trainCount; i++) {
    
    // get the train speed and convert it into hbridge friendly units.
    int temp_speed = map(analogRead(trains[i].speed_pin),0,1024,255,0);
    
    unsigned int temp_status = 0;
    if(digitalRead(trains[i].forward_pin))
      temp_status = 1;
    else if (digitalRead(trains[i].reverse_pin))
      temp_status = 2;
    else
      temp_speed = 0;
        
        // dodgy, couldn't get pointers to work.
        // saw a few bugs where I got 65500 as a result.  weird.
        // this if statement will clean them out.
    if (temp_speed < 256){
      changeSpeed(i, temp_speed, temp_status, digitalRead(trains[i].estop_pin) );
      
      // update our bar graph.
      bar_graph(trains[i], strip, map(temp_speed,0,255,0,150)); // at least we START with the same number here.
    }
      
    
  }
      
  strip.show();
      //delay(1000);
}

// Change the speed of the train
void changeSpeed(unsigned int train_number, int train_speed, unsigned int change_status, unsigned int change_estop ) {
  
  if(!change_estop) {
    // estop has been triggered.  Shut it down.
    Serial.print(trains[train_number].name);
    Serial.println(": estop trigged");
    
    trains[train_number].train_speed = 0;
    digitalWrite( trains[train_number].input_1, LOW );
    digitalWrite( trains[train_number].input_2, LOW );
    
    if (trains[train_number].update_train < (millis() - trains[train_number].update_cycle))
      trains[train_number].update_train = millis();
  } else  if (trains[train_number].update_train < (millis() - trains[train_number].update_cycle)){
    
    trains[train_number].update_train = millis();
    
    
    // no emergency stop, lets proceed normally.
    if (trains[train_number].train_status != change_status)
      train_speed = 0;
      
    if ((trains[train_number].train_speed == 0 ) && trains[train_number].train_status != change_status) {
      // the train has come to a stop, we can change direction.
      trains[train_number].train_status = change_status;
      if( trains[train_number].train_status == 0 )
      {
        Serial.print(trains[train_number].name);
        Serial.println(": status changed to STOP");
        digitalWrite( trains[train_number].input_1, LOW );
        digitalWrite( trains[train_number].input_2, LOW );
      }
      else if( trains[train_number].train_status == 1 ) {
        Serial.print(trains[train_number].name);
        Serial.println(": status changed to FORWARD");
        digitalWrite( trains[train_number].input_1, HIGH );
        digitalWrite( trains[train_number].input_2, LOW );
      }
      else if( trains[train_number].train_status == 2 ) {
        Serial.print(trains[train_number].name);
        Serial.println(": status changed to REVERSE");
        digitalWrite( trains[train_number].input_1, LOW );
        digitalWrite( trains[train_number].input_2, HIGH );
      }
    }
    
    if((train_speed < trains[train_number].train_speed || trains[train_number].train_status != change_status)) {
      // Train is too fast or trying to change states
      Serial.print(trains[train_number].name);
      Serial.print(": Slowing from ");
      Serial.print(trains[train_number].train_speed);
      Serial.print(" to ");
      Serial.println(train_speed);
      
      if ( trains[train_number].train_speed - trains[train_number].slow_value < train_speed )
        trains[train_number].train_speed = train_speed;
      else
        trains[train_number].train_speed = trains[train_number].train_speed - trains[train_number].slow_value;
      
    } else if (trains[train_number].train_status && train_speed > trains[train_number].train_speed) {
      // Train is too slow.
      Serial.print(trains[train_number].name);
      Serial.print(": Speeding up from ");
      Serial.print(trains[train_number].train_speed);
      Serial.print(" to ");
      Serial.println(train_speed);
      
      if (train_speed - trains[train_number].train_speed > trains[train_number].slow_value)
        trains[train_number].train_speed = trains[train_number].train_speed + trains[train_number].slow_value;
      else
        trains[train_number].train_speed = train_speed;
    }
  }
  
  // put the change in speed into play.
  analogWrite( trains[train_number].enable_pin, trains[train_number].train_speed );
}

// set the bar graph
void bar_graph(Train change_train, Adafruit_NeoPixel thestrip, int percentage){
  // tricky.  based off a percentage, set the right led's to on.
  // first set the leds to off.
  
  unsigned int j[2] = {change_train.first,0}; // j[0] is the pin we're up to, j[1] is what position we're up to.
  unsigned int k[3] = {0,0,0};
  
  int temp_train_speed = map(change_train.train_speed,0,255,0,15); // this is our actual speed
  
  for (unsigned int i = 0; i < change_train.bars; i++) {

    if (i < percentage/10 || i < temp_train_speed)
      k[j[1]] = 50; // LED intensity
    else
      k[j[1]] = 0;
     
   // lets add some flashing.
   if ((i >= temp_train_speed || i >= percentage/10)&& change_train.update_train > millis() - change_train.update_cycle/4 ) {
     k[j[1]] = 0;
   }
      
    if (j[1] < 2)
      j[1]++; // iterate the pixel
    else
    {
      thestrip.setPixelColor(j[0],k[1],k[0],k[2]);
      j[0]++;
      j[1] = 0;
      k[0] = 0;
      k[1] = 0;
      k[2] = 0;
    }
  }
}

