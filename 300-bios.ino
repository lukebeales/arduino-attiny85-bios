// format is: Sacrificial byte for brownouts, Map size, then [Instruction, length (ascii code - 48)] repeated as necessary, then the raw data all squished together
// 0 6 ATB8C8 311fa1f7-6710-4123-b870-053a0add3a47HRm97WpEazjr93kv
// A = uuid (AT 311fa1f7-6710-4123-b870-053a0add3a47)
// B = wifi ssid (B8 HRm97WpE)
// C = wifi passcode (C8 azjr93kv)


#include <EEPROM.h>

char bios_code;
char bios_map_buffer[3];
// char bios_buffer[40];  we're sharing the serial buffer now to try and save precious memory

// this is an attempt to 'park' the eeprom pointer to the first bit incase of brownouts
void bios_park() {
  EEPROM.read(0);
}


// i know this wastes clock cycles, but they are free whereas my brain and the chips memory isn't!
void bios_replace(int offset, int length, bool write_map) {

    int difference = 0;
    
    int old_map_size = (int)EEPROM.read(1) - 48;  // size of the map partition

    // if the map is technically empty...
    int old_data_size = 0;  // size of the data partition

    if ( old_map_size > 0 ) {
        // go through the whole map, counting how large the data is
        // this is so we know how much to shift left by, incase the new value is less than the old value
        for ( int i = 1; i < old_map_size; i += 2 ) {
            old_data_size += (int)EEPROM.read(2 + i) - 48;
        }
    }

    if ( write_map ) {
      difference = strlen(bios_map_buffer) - length;
    } else {
      difference = strlen(global_buffer) - length;
    }
       
    // if the replacement data is BIGGER than the original data
    if ( difference > 0 ) {

        // shift all data to the right to make room
        for ( int i = 2 + old_map_size + old_data_size; i > offset + (length - difference); i-- ) {
            EEPROM.update(i + difference, (int)EEPROM.read(i));
        }

    // else if the replacement data is SMALLER than the original data
    } else if ( difference < 0 ) {

        // shift all data to the left
        for ( int i = offset + (length + difference); i < 2 + old_map_size + old_data_size; i++ ) {
            EEPROM.update(i, (int)EEPROM.read(i - difference));
        }

    }
   

    // now write the buffer
    if ( write_map ) {
      for ( int i = 0; i < strlen(bios_map_buffer); i++ ) {
        EEPROM.update(offset + i, bios_map_buffer[i]);
      }
    } else {
      for ( int i = 0; i < strlen(global_buffer); i++ ) {
        EEPROM.update(offset + i, global_buffer[i]);
      }
    }


  bios_park();

}



// this writes the specific setting to the eeprom, rearranging bios as it goes
bool bios_write() {
   
  int old_map_size = (int)EEPROM.read(1) - 48;    // the 48 shift allows us to count starting at the ascii code of 0, so up to 10 bios will seem normal at least
  int old_map_offset = 2;                // where in the eeprom the setting will sit
  int old_map_length = 0;               // how big the map entry is currently

  int old_data_offset = 2 + old_map_size; // where the setting data will sit
  int old_data_length = 0;

  int new_map_size = 0 + old_map_size;
 
  // go through the map counting up the map_offset & data_offset as we go
  for ( int i = 1; i < old_map_size; i += 2 ) {

    // if the current setting 'character' is the one we want, great!
    if ( (char)EEPROM.read(1 + i) == bios_code ) {
 
        old_map_offset = 1 + i;                          // record where we are in the map
        old_map_length = 2;                         // this is how big the current map entry is
        old_data_length = ((int)EEPROM.read(2 + i) - 48);     // this is how much data is currently used by the existing setting

        // if we're looking to replace it with nothing
        if ( strlen(global_buffer) == 0 ) {

            // move the data over two, to allow for the new map entry
            old_data_offset -= 2;

            new_map_size -= 2;  // reduce the map size by 2.
        }

        break;

    // otherwise if the current setting 'character' is after the one we want, then we didn't find it
    } else if ( (int)EEPROM.read(1 + i) > (int)bios_code ) {

        old_map_offset = 1 + i;

        // if there's data to insert...
        if ( strlen(global_buffer) > 0 ) {

            // move the data over two, to allow for the new map entry
            old_data_offset += 2;
            
            new_map_size += 2;  // increase the map size to accommodate the new data
        }

        break;

    } else {

        old_data_offset += ((int)EEPROM.read(2 + i) - 48);  // allows us to use some ascii numbers to start with.

    }

    // if we're the last run, it means we're a new instruction on the end.
    if ( i == 1 + (old_map_size - 2) ) {
        old_map_offset = 2 + old_map_size;
    }

  }

  // if we ended up with a new instruction on the end, and it's not empty
  if (
    ( old_map_offset == (2 + old_map_size) ) &&
    ( strlen(global_buffer) > 0 )
  ) {
      // move the data over two, to allow for the new map entry
      old_data_offset += 2;

      new_map_size += 2;
  }

    // if there's a map that needs to happen
    if ( strlen(global_buffer) > 0 ) {
      bios_map_buffer[1] = strlen(global_buffer) + 48;    // this has to happen first.
      bios_map_buffer[0] = bios_code;
      bios_map_buffer[2] = '\0';
    // otherwise we want to remove the setting right.
    } else {
      bios_map_buffer[0] = '\0';
    }
    bios_replace(old_map_offset, old_map_length, true);


    bios_map_buffer[0] = new_map_size + 48;
    bios_map_buffer[1] = '\0';
    bios_replace(1, 1, true);


    // theory is, we can now make a function like substr_replace and feed these in to it for example:
    bios_replace(old_data_offset, old_data_length, false);

  bios_park();

  return true;
 
}




// reads a specified setting
bool bios_read() {
   
  // reset the buffer just in case
  global_buffer[0] = '\0';

  int map_size = (int)EEPROM.read(1) - 48;    // the 48 shift allows us to count starting at the ascii code of 0, so up to 10 settings will seem normal at least
  int data_offset = 2 + map_size;   // where the setting data sits

  // go through the map counting up the map_offset & data_offset as we go
  for ( int i = 1; i < map_size; i += 2 ) {

      // if the current setting 'character' is the one we want, great!
      if ( (char)EEPROM.read(1 + i) == bios_code ) {
     
        // set the buffer to the value
        for ( int j = 0; j < (int)EEPROM.read(2 + i) - 48; j++ ) {

            global_buffer[j] = (char)EEPROM.read(data_offset + j);
        }

        // close the string
        global_buffer[((int)EEPROM.read(2 + i) - 48)] = '\0';

        break;

      }

      data_offset += (int)EEPROM.read(2 + i) - 48;
  }

  bios_park();

  return true;
 
}


// this sets the first bit as ascii 0.
bool bios_reset() {

  serial.println("");
  serial.print(F("reset..."));


  EEPROM.write(0, '0');
  EEPROM.write(1, '0');
  EEPROM.write(2, '\0');

  // make a uuid here to identify this chip
  bios_code = '*';
  for ( int i = 0; i < 36; i++ ) {
    if ( i == 8 || i == 13 || i == 18 || i == 23 ) {
      global_buffer[i] = '-';
    } else if ( i == 14 ) {
      global_buffer[i] = '4';
    } else {
      if ( random(16) > 9 ) {
        if ( i == 19 ) {
          global_buffer[i] = random(97, 99); // a or b
        } else {
          global_buffer[i] = random(97, 103); // a to f
        }
      } else {
        if ( i == 19 ) {
          global_buffer[i] = random(56, 58);  // 8 or 9
        } else {
          global_buffer[i] = random(48, 58);  // 0 to 9
        }
      }
    }
  }
  global_buffer[36] = '\0';
  
  // add the uuid properly so the map gets updated
  bios_write();
  
  serial.println("");

  bios_park();
  
}


// so what this does is it reads the map size
// goes through the map adding up how much data there is
// then goes through the memory one by one
// if it finds a \0 at any point prior to the total then it's corrupt
// if corrupt, reset it
bool bios_verify() {

  // if the memory looks clean already
  if ( (char)EEPROM.read(0) == '\0' ) {

      bios_reset();

  } else {

      bios_code = '*';
      bios_read();
      if ( strlen(global_buffer) == 0 ) {
        bios_reset();
      }

/* commenting this out for now so I can have enough space in 8k :(

      int map_size = (int)EEPROM.read(1) - 48;    // the 48 shift allows us to count starting at the ascii code of 0, so up to 10 settings will seem normal at least
      int data_size = 0;
    
      // if the map seems too large...
      if ( map_size > 100 ) {

        bios_reset();

      } else {

        // go through the map counting up the map_size & data_size as we go
        for ( int i = 1; i < map_size; i += 2 ) {
          
              // if the eeprom entry is null
              if ( ( (char)EEPROM.read(1 + i) == '\0' ) || ( (char)EEPROM.read(2 + i) == '\0' ) ) {
      
                  i = map_size;
                  data_size = 0;
                  bios_reset();
                  
              } else {
                  data_size += (int)EEPROM.read(2 + i) - 48;  // add to the data size
              }
        }

      }

      if ( data_size > 0 ) {

        // if it seems like the values are wrong...
        if ( ( 2 + map_size + data_size ) > 512 ) {

          bios_reset();

        } else {
  
          // go through the data to see if there's any \0's within it
          for ( int i = 2 + map_size; i < 2 + map_size + data_size; i++ ) {
  
              if ( (char)EEPROM.read(i) == '\0' ) {
      
                  i = 2 + map_size + data_size;
                  bios_reset();
  
              }
              
          }

        }

      }

*/
  }

  bios_park();
  
}


//////////////////////////////////////////////
/*
void bios_chip_diagram_spaces() {
  serial.print(F("   "));
}

void bios_chip_diagram_leg() {
  serial.print(F("--"));
}


void bios_chip_diagram_body() {
  serial.print(F(" "));
  bios_chip_diagram_leg();
  serial.print(F("|"));
  bios_chip_diagram_spaces();
  bios_chip_diagram_spaces();
  serial.print(F("|"));
  bios_chip_diagram_leg();
  serial.print(F(" "));
}

void bios_chip_diagram_end() {
  bios_chip_diagram_spaces();
  bios_chip_diagram_spaces();
  bios_chip_diagram_spaces();
  serial.print(F("+"));
  bios_chip_diagram_leg();
  bios_chip_diagram_leg();
  bios_chip_diagram_leg();
  serial.println(F("+"));
}
*/
//////////////////////////////////////////////

// prints all the settings
void bios_help() {

  serial.println("");

/*
  bios_chip_diagram_end();

  serial.print(F(" reset"));
  bios_chip_diagram_body();
  serial.println(F("VCC"));

  bios_chip_diagram_spaces();
  serial.print(F(" TX"));
  bios_chip_diagram_body();
  serial.println(F("interrupt"));

  bios_chip_diagram_spaces();
  serial.print(F(" RX"));
  bios_chip_diagram_body();
  serial.println(F("switch"));

  bios_chip_diagram_spaces();
  serial.print(F("GND"));
  bios_chip_diagram_body();
  serial.println(F("LED"));

  bios_chip_diagram_end();

  serial.println("");
*/

  int map_size = (int)EEPROM.read(1) - 48;    // the 48 shift allows us to count starting at the ascii code of 0, so up to 10 settings will seem normal at least

  // go through the map counting up the map_offset & data_offset as we go
  for ( int i = 1; i < map_size; i += 2 ) {
    
    bios_code = (char)EEPROM.read(1 + i);
    bios_read();

    serial.print(bios_code);
    serial.print(F(": "));          // single quotes are for a single character, double for a string
    serial.println(global_buffer);
    
  }

  serial.println("");
  serial.println(F("! = int (y)"));
  serial.println(F("~ = charge"));
  serial.println(F("+ = on"));
  serial.println(F("- = off"));

  serial.println("");
  serial.println(F("BIOS+[GET=[?]|SET=[?],val|ON|OFF|RND|CYCLE|UPTIME|TIME[=123]|HELP|RESET|SHH|ZZZ|PING]"));
  serial.println("");

}


/*
  // reads and prints all the settings.
  bool bios_menu() {
  
  
    serial.println("");
    serial.println(F("= Bios ="));
    
    bios_list();
  
    serial.println(F("* = reset"));
    serial.println(F("? = list"));
    serial.print(F("Set: "));
  
  }
*/

/*
bool bios_init() {

  // lets do a sanity check here...
  bios_verify();

  // if the bios jumper is set (pulled down instead of just vcc)
  pinMode(A0, INPUT);
  int biosJumper = analogRead(A0);

  
  // this was 900 when we didn't have the diode in place for 5v
  if ( biosJumper < 1000 ) {

    // let the user know we're in the bios
    pinMode(0, OUTPUT);
    digitalWrite(0, HIGH);

      bios_menu();
    
      // 0 = menu
      // 1 = key received, now waiting for the value
      byte bios_state = 0;

      // keep looping while the analog pin is floating
      while ( true ) {

        // we are looking for uuid, ssid, password
    
        if ( serial_read(serial) ) {
    
          // if we're on the menu
          if ( bios_state == 0 ) {
    
            if ( strlen(global_buffer) > 0 ) {
    
              // record the 'bios_code' as the first character of the serial buffer
              bios_code = global_buffer[0];
            
              if ( bios_code == '*' ) {
    
                bios_reset();
     
                // now show everything again
                bios_menu();
    
              } else if ( bios_code == '?' ) {
    
                bios_menu();
    
              } else {
    
                serial.println("");
                serial.print(F("Set "));
                serial.print(bios_code);
                serial.print(F(" to: "));
      
                // now wait for the value
                bios_state = 1;
    
              }
    
            }
            
          } else if ( bios_state == 1 ) {
    
            // copy global_buffer to bios_buffer
            // *** we're now reusing the global_buffer for this job to save memory! ***
            // strcpy(bios_buffer, global_buffer);
            
            serial.println("");
            serial.print(F("Setting "));
            serial.print(bios_code);
            serial.print(F(" to "));
            serial.print(global_buffer);
            serial.println(F("..."));
            bios_write();
    
            // now show everything again
            bios_menu();
                    
            // go back to waiting for a command
            bios_state = 0;
    
          }
          
        }

      }

  }
}
*/

  /* just incase we want to put multiple buttons on it, this is how to do it.
    if ( timer < 830 ) {    // the lowest possible reading is 839
      timer = 830;
    }
    timer = FREQUENCIES[round((timer - 830) / ((1024 - 830) / 11))];    // dividing by 11 allows us to use the same trick for 0-9 buttons on the analog pins.
  */
