//c++ https://www.onlinegdb.com/online_c++_compiler
//c++ #include <iostream>
//c++ #include <cstring>

// format is: Map size, then [Instruction, length (ascii code - 48)] repeated as necessary, then the raw data all squished together
// 6 ATB8C8 311fa1f7-6710-4123-b870-053a0add3a47HRm97WpEazjr93kv
// A = uuid (AT 311fa1f7-6710-4123-b870-053a0add3a47)
// B = wifi ssid (B8 HRm97WpE)
// C = wifi passcode (C8 azjr93kv)


#include <EEPROM.h>

// 0 = menu
// 1 = key received, now waiting for the value
// 2 = value received
byte bios_state = 0;

char bios_code;
char bios_buffer[64];


// i know this wastes clock cycles, but they are free whereas my brain and the chips memory isn't!
void bios_replace(int offset, int length) {

    int old_map_size = (int)EEPROM.read(0) - 48;  // size of the map partition
    //c++ int difference = std::char_traits<char>::length(bios_buffer) - length;
    int difference = strlen(bios_buffer) - length;

    // std::cout << std::char_traits<char>::length(bios_buffer);
    // std::cout << '\n';

    // if the map is technically empty...
    int old_data_size = 0;  // size of the data partition

    if ( old_map_size > 0 ) {
        // go through the whole map, counting how large the data is
        // this is so we know how much to shift left by, incase the new value is less than the old value
        for ( int i = 1; i < old_map_size; i += 2 ) {
            old_data_size += ((int)EEPROM.read(i) + 1) - 48;
        }
    }
   
    // if the replacement data is BIGGER than the original data
    if ( difference > 0 ) {

        // shift all data to the right to make room
        for ( int i = 1 + old_map_size + old_data_size; i > offset + (length - difference); i-- ) {
            EEPROM.write(i + difference, (int)EEPROM.read(i));
        }

    // else if the replacement data is SMALLER than the original data
    } else if ( difference < 0 ) {

        // shift all data to the left
        for ( int i = offset + (length + difference); i < 1 + old_map_size + old_data_size; i++ ) {
            EEPROM.write(i, (int)EEPROM.read(i - difference));
        }

    }
   

    // now write the buffer
    //c++ for ( int i = 0; i < std::char_traits<char>::length(bios_buffer); i++ ) {
    for ( int i = 0; i < strlen(bios_buffer); i++ ) {
        EEPROM.write(offset + i, bios_buffer[i]);
    }

}



// this writes the specific setting to the eeprom, rearranging bios as it goes
bool bios_write() {
   
  int old_map_size = (int)EEPROM.read(0) - 48;    // the 48 shift allows us to count starting at the ascii code of 0, so up to 10 bios will seem normal at least
  int old_map_offset = 1;                // where in the map the setting will sit
  int old_map_length = 0;               // how big the map entry is currently

  int old_data_offset = 1 + old_map_size; // where the setting data will sit
  int old_data_length = 0;

  int new_map_size = 0 + old_map_size;
 
  // go through the map counting up the map_offset & data_offset as we go
  for ( int i = 1; i < old_map_size; i += 2 ) {

        // if the current setting 'character' is the one we want, great!
        if ( (char)EEPROM.read(i) == bios_code ) {
     
            // std::cout << "Found it to replace!\n";

            old_map_offset = i;                          // record where we are in the map
            old_map_length = 2;                         // this is how big the current map entry is
            old_data_length = ((int)EEPROM.read(i + 1) - 48);     // this is how much data is currently used by the existing setting

            // if we're looking to replace it with nothing
            //c++ if ( std::char_traits<char>::length(bios_buffer) == 0 ) {
            if ( strlen(bios_buffer) == 0 ) {
                new_map_size -= 2;  // reduce the map size by 2.
            }

            break;

        // otherwise if the current setting 'character' is after the one we want, then we didn't find it
        } else if ( (int)EEPROM.read(i) > (int)bios_code ) {

            // std::cout << "New entry in the middle somewhere!\n";

            old_map_offset = i;

            // if there's data to insert...
            //c++ if ( std::char_traits<char>::length(bios_buffer) > 0 ) {
            if ( strlen(bios_buffer) > 0 ) {
                new_map_size += 2;  // increase the map size to accommodate the new data
            }

            break;

        } else {

            // std::cout << "Nothing matched, add up some numbers\n";
            old_data_offset += ((int)EEPROM.read(i + 1) - 48);  // allows us to use some ascii numbers to start with.

        }

    // if we're the last run, it means we're a new instruction on the end.
    if ( i == 1 + (old_map_size - 2) ) {
        old_map_offset = 1 + old_map_size;
    }

  }

  // if we ended up with a new instruction on the end, and it's not empty
  if (
    ( old_map_offset == (1 + old_map_size) ) &&
    //c++ ( std::char_traits<char>::length(bios_buffer) > 0 )
    ( strlen(bios_buffer) > 0 )
  ) {
      new_map_size += 2;
  }

    // theory is, we can now make a function like substr_replace and feed these in to it for example:
    bios_replace(old_data_offset, old_data_length);


    // if there's a map that needs to happen
    //c++ if ( std::char_traits<char>::length(bios_buffer) > 0 ) {
    if ( strlen(bios_buffer) > 0 ) {
      //c++ bios_buffer[1] = std::char_traits<char>::length(bios_buffer) + 48;    // this has to happen first.
      bios_buffer[1] = strlen(bios_buffer) + 48;    // this has to happen first.
      bios_buffer[0] = bios_code;
      bios_buffer[2] = '\0';
    // otherwise we want to remove the setting right.
    } else {
      bios_buffer[0] = '\0';
    }
    bios_replace(old_map_offset, old_map_length);


    bios_buffer[0] = new_map_size + 48;
    bios_buffer[1] = '\0';
    bios_replace(0, 1);

  return true;
 
}




// reads a specified setting
bool bios_read() {
   
  // reset the buffer just in case
  bios_buffer[0] = '\0';

  int map_size = (int)EEPROM.read(0) - 48;    // the 48 shift allows us to count starting at the ascii code of 0, so up to 10 settings will seem normal at least
  int data_offset = 1 + map_size;   // where the setting data sits

  // go through the map counting up the map_offset & data_offset as we go
  for ( int i = 1; i < map_size; i += 2 ) {

      // if the current setting 'character' is the one we want, great!
      if ( (char)EEPROM.read(i) == bios_code ) {
     
        // std::cout << "Found it!\n";

        // set the buffer to the value
        for ( int j = 0; j < (int)EEPROM.read(i + 1) - 48; j++ ) {

            bios_buffer[j] = (char)EEPROM.read(data_offset + j);
        }

        // close the string
        bios_buffer[((int)EEPROM.read(i + 1) - 48)] = '\0';

        break;

      }

      data_offset += (int)EEPROM.read(i + 1) - 48;
  }

  return true;
 
}


// this sets the first bit as ascii 0.
bool bios_reset() {

  serial.println("");
  serial.print(F("!!! bios data failed check, formatting..."));

  EEPROM.write(0, 0 + 48);
  EEPROM.write(1, '\0');

  serial.println(F("done !!!"));
  serial.println("");

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

      int map_size = (int)EEPROM.read(0) - 48;    // the 48 shift allows us to count starting at the ascii code of 0, so up to 10 settings will seem normal at least
      int data_size = 0;
    
      // if the map seems too large...
      if ( map_size > 100 ) {

        bios_reset();

      } else {

        // go through the map counting up the map_offset & data_offset as we go
        for ( int i = 1; i < map_size; i += 2 ) {
          
              // if the eeprom entry is null
              if ( ( (char)EEPROM.read(i) == '\0' ) || ( (char)EEPROM.read(i + 1) == '\0' ) ) {
      
                  i = map_size;
                  data_size = 0;
                  bios_reset();
  
              } else {
                  data_size += (int)EEPROM.read(i + 1) - 48;  // add to the data size
              }
        }

      }

      if ( data_size > 0 ) {

        // if it seems like the values are wrong...
        if ( ( 1 + data_size + data_size ) > 512 ) {

          bios_reset();

        } else {
  
          // go through the data to see if there's any \0's within it
          for ( int i = 1 + map_size; i < 1 + map_size + data_size; i ++ ) {
  
              if ( (char)EEPROM.read(i) == '\0' ) {
      
                  i = 1 + map_size + data_size;
                  bios_reset();
  
              }
              
          }

        }

      }

  }  
    
}


// reads and prints all the settings.
bool bios_menu() {

  serial.println("");
  serial.println(F("= Bios ========================"));
  serial.println("");
  
  
  int map_size = (int)EEPROM.read(0) - 48;    // the 48 shift allows us to count starting at the ascii code of 0, so up to 10 settings will seem normal at least

  // go through the map counting up the map_offset & data_offset as we go
  for ( int i = 1; i < map_size; i += 2 ) {
    
    bios_code = (char)EEPROM.read(i);
    bios_read();

    serial.print(bios_code);
    serial.print(F(": "));          // single quotes are for a single character, double for a string
    serial.println(bios_buffer);
    
  }

  serial.println(F(""));
  serial.print(F("Set: "));

}


bool bios_init() {

  serial_start();

  // lets do a sanity check here...
  bios_verify();

  bios_menu();

  // give the user a rolling 5 seconds to change it
  unsigned int expires = round(millis() / 1000) + 10;
  while (
    ( expires == 0 ) ||
    ( round(millis() / 1000) < expires )
  ) {

    // we are looking for uuid, ssid, password

    if ( serial_read() ) {

      // if we're on the menu
      if ( bios_state == 0 ) {

        if ( strlen(serial_buffer) > 0 ) {

          // cancel the timeout
          expires = 0;

          // record the 'bios_code' as the first character of the serial buffer
          bios_code = serial_buffer[0];
        
          serial.println("");
          serial.print(F("Set "));
          serial.print(bios_code);
          serial.print(F(" to: "));
  
          // now wait for the value
          bios_state = 1;

        }
        
      } else if ( bios_state == 1 ) {

        // copy serial_buffer to bios_buffer
        strcpy(bios_buffer, serial_buffer);
        
        serial.println("");
        serial.print(F("setting "));
        serial.print(bios_code);
        serial.print(F(" to "));
        serial.print(bios_buffer);
        serial.println(F("..."));
      
        bios_write();

        // now show everything again
        bios_menu();
                
        // go back to waiting for a command
        bios_state = 0;

        // if something is received on the serial, recalculate the expires value
        // expires = 0;
        expires = round(millis() / 1000) + 10;
  
      }
      
    }
   
  }
  
  /*
      serial.println("");
      serial.println("");
    
      bios_code = 'C';
      bios_read();
    
      serial.println(bios_buffer);
    
      bios_code = 'A';
      strcpy(bios_buffer, "howdy!");
      bios_write();
    
      bios_code = 'B';
      bios_read();
    
      serial.println(bios_buffer);
  */

  serial.println("");
  serial.println(F("Exiting bios..."));

  serial_end();
}
