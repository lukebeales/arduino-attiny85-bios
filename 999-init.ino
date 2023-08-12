unsigned long enable_serial_at = 0;   // this tells the loop whether to switch on the serial again


////////////////////////////////////////

void loop() {

  // work out if we should flash the led
  at_led_flash();

  // read the serial port
  serial_read(serial);
  
  // if it receives something that starts with BIOS+...
  if ( strlen(global_buffer) > 0 ) {
  
    // grab a hold of the TX pin
    serial_transmitting(true);

    // match the command...
    // i know there's a better way to do this but stick with me for now...
    if (
      ( global_buffer[0] == 'G' ) &&
      ( global_buffer[1] == 'E' ) &&
      ( global_buffer[2] == 'T' )
    ) {

      // fetch the thing from the bios
      bios_code = global_buffer[4];
      bios_read();
  
        // print it to the serial port.
        serial.print(global_buffer);
        serial.println("");
 
    } else if (
      ( global_buffer[0] == 'S' ) &&
      ( global_buffer[1] == 'E' ) &&
      ( global_buffer[2] == 'T' )
    ) {

      if (
        ( global_buffer[4] != '*' ) &&
        ( global_buffer[4] != '?' )
      ) {
        
        bios_code = global_buffer[4];
        // shift everything down so we end up with just the value to be stored, even if it's empty.
        at_shift_global_buffer(6);
        bios_write();
  
        // refresh these just incase
        setup_get_on_off_times();

        serial.println(F("OK"));
  
      }
      
    } else if ( strcmp(global_buffer, "HELP") == 0 ) {

      bios_help();      
    
    } else if ( strcmp(global_buffer, "PING") == 0 ) {

        serial.println(F("OK"));
    
    } else if ( strcmp(global_buffer, "RESET") == 0 ) {

      bios_reset();      
    
      // refresh these just incase
      setup_get_on_off_times();

      serial.println(F("OK"));
  
    } else if ( strcmp(global_buffer, "RND") == 0 ) {

      // return a random number between 0 and the maximum value for a long variable type
      serial.println(random(0, 2147483647));

    } else if ( strcmp(global_buffer, "CYCLE") == 0 ) {

      // says how many times we have switched
      serial.println(setup_switched_count);

    } else if ( strcmp(global_buffer, "UPTIME") == 0 ) {

      // returns how long the bios has been running
      serial.println(millis());

    } else if (
      ( global_buffer[0] == 'T' ) &&
      ( global_buffer[1] == 'I' ) &&
      ( global_buffer[2] == 'M' ) &&
      ( global_buffer[3] == 'E' )
    ) {

      if ( global_buffer[4] == '=' ) {

        // reduce the global_buffer down so we're left with the last piece
        at_shift_global_buffer(5);

        if ( strlen(global_buffer) >= 10 ) {

          // we'll store the timestamp value as the actual time the chip powered on, by taking away (millis() / 1000) from it
          at_timestamp_at_init = at_longify_global_buffer() - round(millis() / 1000);
  
          serial.println(F("OK"));
        }

      } else {

        // return the current timestamp
        serial.println(at_timestamp_at_init + round(millis() / 1000));

      }
    
    } else if ( strcmp(global_buffer, "ON") == 0 ) {
  
      setup_switch(true);
      serial.println(F("OK"));

    // stop listening to the serial input for a while to save on unnecessary processing    
    } else if ( strcmp(global_buffer, "SHH") == 0 ) {
      
      // this needs to ignore all serial input, maybe by disabling then re-enabling serial?
      serial.println(F("OK"));

      serial_end();

      enable_serial_at = millis() + 5000;

    // turn off the switch and add the unused time to 'off'
    } else if ( strcmp(global_buffer, "ZZZ") == 0 ) {
  
      // work out the unused 'on' portion, if we were on
      if (
        ( at_switched == true ) &&
        ( setup_switched_on_when > 0 ) &&
        ( setup_on_time > 0 ) &&
        ( setup_off_time > 0 )
      ) {
        setup_zzz_time = (setup_on_time * 1000) - (millis() - setup_switched_on_when);
      }
      
      setup_switch(false);

      serial.println(F("OK"));

    } else if ( strcmp(global_buffer, "OFF") == 0 ) {

      setup_switch(false);

      serial.println(F("OK"));

    }

    // release the TX pin
    serial_transmitting(false);

  }


  // here we are charging the capacitor
  if (
   ( at_charging == true ) &&
   ( millis() < (charge_for * 1000) )
  ) {

    // do nothing here.  read a book.  have a cup of tea.
  
  // now we can go to normal operation.
  } else {

    // disable charging, this is silly as it sets it each cycle but it'll do for now.
    at_charging = false;
    

    // if any of the times are set
    if (
      ( setup_switched_count == 0 ) &&
      (
        ( setup_on_time > 0 ) ||
        ( setup_off_time > 0 )
      )
    ) {
  
      // start switched
      setup_switch(true);
  
    }


    // check to see if we should switch off
    if (
      ( at_switched == true ) &&
      ( setup_switched_on_when > 0 ) &&
      ( setup_on_time > 0 ) &&
      ( millis() > ( setup_switched_on_when + ( setup_on_time * 1000 ) ) )
    ) {
    
      setup_switch(false);
  
    // if we're off but should be on
    } else if (
      ( at_switched == false ) &&
      ( setup_switched_off_when > 0 ) &&
      ( setup_off_time > 0 ) &&
      ( millis() > ( setup_zzz_time + ( setup_off_time * 1000 ) + setup_switched_off_when ) )
    ) {
  
      setup_switch(true);
  
    }

    // if the interrupt has triggered
    if ( at_interrupted ) {
      setup_switch(true);
    }

  }
  
  if (
    ( enable_serial_at > 0 ) &&
    ( millis() > enable_serial_at )
  ) {
    enable_serial_at = 0;
    serial_start();
  }

  
  // sleep for a bit so we can save some power.
  // we'll still wake up if serial data comes in.
  at_sleep();

}
