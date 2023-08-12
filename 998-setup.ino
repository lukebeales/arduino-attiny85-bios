
// these allow for capacitor charging before we switch anything
unsigned long charge_for = 0;
unsigned long setup_switched_count = 0;


volatile unsigned long setup_zzz_time = 0; // this is the unused 'on' time if we are told to sleep while we're on
volatile unsigned long setup_switched_on_when = 0;
volatile unsigned long setup_switched_off_when = 0;
void setup_switch(boolean should_i) {

  at_interrupted = false;
  pinMode(1, OUTPUT);

  // just incase...
  if ( should_i == true ) {

    digitalWrite(1, HIGH);

    // we can't go to sleep while this is high
    at_switched = true;

    // record when we have switched on
    setup_switched_on_when = millis();

    setup_switched_off_when = 0;
    setup_zzz_time = 0;

    setup_switched_count++;

  } else {

    digitalWrite(1, LOW);
    pinMode(1, INPUT);  // this is to save power

    at_switched = false;

    // record when we have switched off
    setup_switched_off_when = millis();

    setup_switched_on_when = 0;

  }

}

volatile unsigned long setup_on_time = 0;
volatile unsigned long setup_off_time = 0;
void setup_get_on_off_times() {

  setup_zzz_time = 0;

  bios_code = '+';
  bios_read();
  setup_on_time = at_longify_global_buffer();
  setup_switched_on_when = millis();
  
  bios_code = '-';
  bios_read();
  setup_off_time = at_longify_global_buffer();
  setup_switched_off_when = millis();

  bios_code = '!';
  bios_read();
  if ( strlen(global_buffer) > 0 ) {
    at_interrupt_enable = true;
  }

}


void setup() {

  at_pin_clense();
  
  // seed the random function for good measure...
  at_random_init();

  // everything needs serial
  serial_start();

  serial_transmitting(true);

      // verify the bios is all good.
      bios_verify();
    
      bios_help();
    
      // read the times in to start with
      setup_get_on_off_times();

  serial_transmitting(false);

  // allow for a capacitor to charge if necessary
  bios_code = '~';
  bios_read();
  if ( strlen(global_buffer) > 0 ) {
    at_charging = true;
    charge_for = at_longify_global_buffer();
  }
}
