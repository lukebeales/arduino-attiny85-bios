#include <avr/sleep.h>      // Needed for sleep_mode
#include <avr/power.h>
#include <avr/wdt.h>        // Needed to enable/disable watch dog timer
#include <util/atomic.h>
#include <avr/interrupt.h>  // needed to get the ISR function for pin interrupts

unsigned long at_timestamp_at_init = 0;  // this is the timestamp for when the chip was powered on, set by "BIOS+TIME=1234567"

// go through each digit, this allows for normal numbers to be stored
unsigned long at_longify_global_buffer() {
  unsigned long output = 0;
  if ( ( strlen(global_buffer) > 0 ) && ( strlen(global_buffer) <= 10 ) ) {  // keeping this below 10 means we won't risk a buffer overflow
    for ( byte i = 0, ilen = strlen(global_buffer); i < ilen; i++ ) {

      // *** MAKE SURE GLOBAL_BUFFER[i] IS A NUMBER
      if ( ( global_buffer[i] >= 48 ) && ( global_buffer[i] <= 57 ) ) {

        // i know this seems crazy, but the pow() function when referencing a variable uses 2k of space, whereas this uses a matter of bytes
        output = (10 * output) + (global_buffer[i] - 48);
      }
      
    }
  }
  return output;
}


void at_shift_global_buffer(byte remove_this_many) {
  // the <= means we'll pick up the \0 at the end also
  for ( byte i = 0, ilen = strlen(global_buffer) - remove_this_many; i <= ilen; i++ ) {
    global_buffer[i] = global_buffer[remove_this_many+i];
  }
}



// this is moved to here so that we can call it during serial_read
unsigned long at_last_led_flash = 0;
boolean at_charging = 0;
boolean at_switched = false;  // records whether the mosfet is switched high or not.
void at_led_flash() {

  if (
    ( at_last_led_flash == 0 ) ||   // this allows a flick at the start.
    (
      ( at_charging == true ) &&
      ( millis() - at_last_led_flash > 250 )
    ) || (
      ( at_switched == true ) &&
      ( millis() - at_last_led_flash > 500 )
    ) || (
      ( at_switched == false ) &&
      ( millis() - at_last_led_flash > 5000 )
    )
  ) {

    // flash it
    pinMode(0, OUTPUT);
    digitalWrite(0, HIGH);
    delay(1);
    digitalWrite(0, LOW);
    pinMode(0, INPUT);

    // record the current time so we know when to flash next
    at_last_led_flash = millis();

  }
}


volatile boolean at_interrupt_enable = 0;  // this is set by a ! in the bios
// unsigned long at_time_switched; // this keeps track of when the chip was awake, so we can sleep for different times
// volatile boolean at_prevent_sleep = 0;  // tells the chip to not sleep yet

//////////////////////////////////////////////////

/*
  // https://forum.arduino.cc/t/generating-random-numbers-takes-too-long/240183/13
  unsigned int at_random() {
    unsigned long long t;
    x=314527869x+1234567;
    y^=y<<5;
    y^=y>>7;
    y^=y<<22;
    t =4294584393ULLz+c;
    c= t>>32;
    z= t;
    return x+y+z;
  }
*/


///////////////////////////////////////////////////////
  
#define randomSeed(s) srandom(s)

volatile uint32_t at_seed;  // These two variables can be reused in your program after the
volatile int8_t at_nrot;    // function CreateTrulyRandomSeed()executes in the setup() 
                         // function.
// create a truly random seed
// shamelessly stolen from here: https://sites.google.com/site/astudyofentropy/project-definition/timer-jitter-entropy-sources/entropy-library/arduino-random-seed
// which was found via here: https://arduino.stackexchange.com/questions/50671/getting-a-truly-random-number-in-arduino
void at_create_seed() {
  at_seed = 0;
  at_nrot = 32; // Must be at least 4, but more increased the uniformity of the produced 
             // seeds entropy.
  
  // The following five lines of code turn on the watch dog timer interrupt to create
  // the seed value
  cli();                                             
  MCUSR = 0;                                         
  _WD_CONTROL_REG |= (1<<_WD_CHANGE_BIT) | (1<<WDE); 
  _WD_CONTROL_REG = (1<<WDIE);                       
  sei();                                             
 
  while (at_nrot > 0);  // wait here until seed is created
 
  // The following five lines turn off the watch dog timer interrupt
  cli();                                             
  MCUSR = 0;                                         
  _WD_CONTROL_REG |= (1<<_WD_CHANGE_BIT) | (0<<WDE); 
  _WD_CONTROL_REG = (0<< WDIE);                      
  sei();                                             
}


// gets a truly random seed, and then seeds the generator.
void at_random_init() {
  at_create_seed();
  randomSeed(at_seed);
}

///////////////////////////////////////////////////////

void at_pin_clense() {

  // set all pins as inputs.  this apparently saves power.
  for ( byte i = 0; i <= 4; i++ ) {
    pinMode(i, INPUT);
    // digitalWrite(i, LOW);
  }

}

//////////////////////////////////////////////////
// this triggers whenever a change is detected on pin 2, if we're sleeping.
boolean at_interrupted = 0;
ISR(INT0_vect) {
  at_interrupted = true;
}

// this triggers each time we're sleeping
ISR(WDT_vect) {
  // these are needed for the random seed
  at_nrot--;
  at_seed = at_seed << 8;
  at_seed = at_seed ^ TCNT1;  // this was TCNT1L, this is the timer interrupt
}

//////////////////////////////////////////////////

// Sets the watchdog timer to wake us up, but not reset
// From: http://interface.khm.de/index.php/lab/experiments/sleep_watchdog_battery/
// this is in seconds
void at_sleep() {

  // if ( at_prevent_sleep == false ) {
  
      // make all the pins inputs
      // at_pin_clense();
      
      ADCSRA &= ~(1<<ADEN); //Disable ADC, saves ~230uA

      set_sleep_mode(SLEEP_MODE_IDLE);        // Keeps timer1 going so millis() will be good
      // set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Power down everything, wake up from WDT

      // power_all_disable();                    // turn power off to ADC, TIMER 1 and 2, Serial Interface
      noInterrupts();                         // turn off interrupts as a precaution
    
      //This order of commands is important and cannot be combined
      MCUSR &= ~(1<<WDRF); //Clear the watch dog reset
      WDTCR |= (1<<WDCE) | (1<<WDE); //Set WD_change enable, set WD enable
      //WDP3 - WDP2 - WPD1 - WDP0 - time
      // 0      0      0      0      16 ms
      // 0      0      0      1      32 ms
      // 0      0      1      0      64 ms
      // 0      0      1      1      0.125 s
      // 0      1      0      0      0.25 s
      // 0      1      0      1      0.5 s
      // 0      1      1      0      1.0 s
      // 0      1      1      1      2.0 s
      // 1      0      0      0      4.0 s
      // 1      0      0      1      8.0 s
//      WDTCR = (0<<WDP3) | (1<<WDP2) | (1<<WDP1) | (0<<WDP0); // set it to 1 second
      WDTCR = (0<<WDP3) | (0<<WDP2) | (1<<WDP1) | (1<<WDP0); // set it to 0.125s
//      WDTCR = (0<<WDP3) | (0<<WDP2) | (1<<WDP1) | (0<<WDP0); // set it to 64ms
      WDTCR |= _BV(WDIE); //Set the interrupt enable, this will keep unit from resetting after each int
  
   
      wdt_reset ();
  
      // *** this needs to be wrapped in an if so we can save power without it if need be
      if ( at_interrupt_enable == true ) {
          // https://forum.arduino.cc/t/i-want-to-change-from-pcint-to-int0-external-int-on-attiny85/562555/12
          GIMSK |= (1 << INT0);
          // set it to trigger on any logical change on the pin (from 1 to 0, or 0 to 1
          MCUCR |= (0 << ISC01);
          MCUCR |= (1 << ISC00);
          // clear any queued external interrupts
          GIFR = (1<<INTF0);
      }
      // ***
            
      // unsigned long counter = 0;
  
      // enable interrupts for our sleep
      interrupts();
  
/*      
      while (
        ( counter < sheep ) &&
        ( at_force_wakeup == 0 )
      ) {
*/
        sleep_enable();
    
        // go to sleep
        // sleep_mode();
        sleep_cpu();
    
        //==== here we are asleep ====//
        
        // we have woken back up, disable sleeping
        sleep_disable();
/*        
        // count one more sheep
        counter++;
    
      }
*/
      // disable interrupts for this bit
      noInterrupts();
    
      // this needs to be wrapped in an if to save power
      if ( at_interrupt_enable == true ) {
        GIMSK &= ~(1<<INT0);     // disable int0
      }
      //
  
      // switch things back on
      // power_all_enable();
      ADCSRA |= (1<<ADEN); //Enable ADC
  
      interrupts();
  
//    }
  // }
}


/*
    // read the vcc against the internal voltage, found here.
    // https://github.com/cano64/ArduinoSystemStatus/blob/master/SystemStatus.cpp
    int at_vcc() {
      // Read 1.1V reference against AVcc
      // set the reference to Vcc and the measurement to the internal 1.1V reference
      ADMUX = _BV(MUX3) | _BV(MUX2);
      delay(2); // Wait for Vref to settle
      ADCSRA |= _BV(ADSC); // Start conversion
      while (bit_is_set(ADCSRA,ADSC)); // measuring
      uint8_t low = ADCL;
      unsigned int val = (ADCH << 8) | low;
      //discard previous result
      ADCSRA |= _BV(ADSC); // Convert
      while (bit_is_set(ADCSRA, ADSC));
      low = ADCL;
      val = (ADCH << 8) | low;
      
      return ((long)1024 * 1100) / val;
    }
    
    
    int8_t at_temp() {
      // from the data sheet
      //  Temperature / °C -45°C +25°C +85°C
      //  Voltage     / mV 242 mV 314 mV 380 mV
      ADMUX = (1<<REFS0) | (1<<REFS1) | (1<<MUX3); //turn 1.1V reference and select ADC8
      delay(2); //wait for internal reference to settle
      // start the conversion
      ADCSRA |= bit(ADSC);
      //sbi(ADCSRA, ADSC);
      // ADSC is cleared when the conversion finishes
      while (ADCSRA & bit(ADSC));
      //while (bit_is_set(ADCSRA, ADSC));
      uint8_t low  = ADCL;
      uint8_t high = ADCH;
      //discard first reading
      ADCSRA |= bit(ADSC);
      while (ADCSRA & bit(ADSC));
      low  = ADCL;
      high = ADCH;
      int a = (high << 8) | low;
      return a - 272; //return temperature in C
    }

*/
