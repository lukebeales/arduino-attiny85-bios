// https://www.onlinegdb.com/online_c++_compiler
#include <iostream>
#include <cstring>

// so the format is: map_size, map, data
// with map being two characters for each data block: position, and length.
char eeprom[128] = "6BTC6E8....................................------++++++++";
// char eeprom[128] = "0";

char settings_code;
char settings_buffer[64];



// i know this wastes clock cycles, but they are free whereas my brain and the chips memory isn't!
void settings_replace(int offset, int length) {

    int old_map_size = eeprom[0] - 48;  // size of the map partition
    int difference = std::char_traits<char>::length(settings_buffer) - length;

    // std::cout << std::char_traits<char>::length(settings_buffer);
    // std::cout << '\n';

    // if the map is technically empty...
    int old_data_size = 0;  // size of the data partition
    if ( old_map_size == 0 ) {
        // make the old_data_size whatever is currently stored in there, minus the map size
        old_data_size = std::char_traits<char>::length(eeprom) - 1;
    } else {
        // go through the whole map, counting how large the data is
        // this is so we know how much to shift left by, incase the new value is less than the old value
        for ( int i = 1; i < old_map_size; i += 2 ) {
            old_data_size += (eeprom[i] + 1) - 48;
        }
    }
   
    // if the replacement data is BIGGER than the original data
    if ( difference > 0 ) {

        // shift all data to the right to make room
        for ( int i = 1 + old_map_size + old_data_size; i > offset + (length - difference); i-- ) {
            eeprom[i + difference] = eeprom[i];
        }

    // else if the replacement data is SMALLER than the original data
    } else if ( difference < 0 ) {

        // shift all data to the left
        for ( int i = offset + (length + difference); i < 1 + old_map_size + old_data_size; i++ ) {
            eeprom[i] = eeprom[i - difference];
        }

    }
   

    // now write the buffer
    for ( int i = 0; i < std::char_traits<char>::length(settings_buffer); i++ ) {
        eeprom[offset + i] = settings_buffer[i];
    }

}



// this writes the specific setting to the eeprom, rearranging settings as it goes
bool settings_write() {
   
  int old_map_size = eeprom[0] - 48;    // the 48 shift allows us to count starting at the ascii code of 0, so up to 10 settings will seem normal at least
  int old_map_offset = 1;                // where in the map the setting will sit
  int old_map_length = 0;               // how big the map entry is currently

  char new_map_data[2];
  // if there's something to set
  if ( std::char_traits<char>::length(settings_buffer) > 0 ) {
      new_map_data[0] = settings_code;
      new_map_data[1] = std::char_traits<char>::length(settings_buffer) + 48;       // this seems to count correctly now

//////////////////////////////////////////////////////////////////////////
      new_map_data[2] = '\0';   // THERES SOMETHING WRONG WITH THIS!!!
////////////////////////////////////////////////////////////////////////////

  // otherwise we want to remove the setting right.
  } else {
      new_map_data[0] = '\0';
  }

  int old_data_offset = 1 + old_map_size; // where the settings data will sit
  int old_data_length = 0;

  int new_map_size = 0 + old_map_size;
  
  // go through the map counting up the map_offset & data_offset as we go
  for ( int i = 1; i < old_map_size; i += 2 ) {

        // if the current setting 'character' is the one we want, great!
        if ( eeprom[i] == settings_code ) {
     
            // std::cout << "Found it to replace!\n";

            old_map_offset = i;                          // record where we are in the map
            old_map_length = 2;                         // this is how big the current map entry is
            old_data_length = (eeprom[i + 1] - 48);     // this is how much data is currently used by the existing setting

            // if we're looking to replace it with nothing
            if ( std::char_traits<char>::length(new_map_data) == 0 ) {
                new_map_size -= 2;  // reduce the map size by 2.
            }

            break;

        // otherwise if the current setting 'character' is after the one we want, then we didn't find it
        } else if ( eeprom[i] > settings_code ) {

            // std::cout << "New entry in the middle somewhere!\n";

            old_map_offset = i;

            // if there's data to insert...
            if ( std::char_traits<char>::length(new_map_data) > 0 ) {
                new_map_size += 2;  // increase the map size to accommodate the new data
            }

            break;

        } else {

            // std::cout << "Nothing matched, add up some numbers\n";
            old_data_offset += (eeprom[i + 1] - 48);  // allows us to use some ascii numbers to start with.

        }

    // if we're the last run, it means we're a new instruction on the end.
    if ( i == 1 + (old_map_size - 2) ) {
        old_map_offset = 1 + old_map_size;
    }

  }

  // if we ended up with a new instruction on the end, and it's not empty
  if (
    ( old_map_offset == (1 + old_map_size) ) &&
    ( std::char_traits<char>::length(settings_buffer) > 0 )
  ) {
      new_map_size += 2;
  }

    /*
      std::cout << new_map_size;
      std::cout << "\n";
      std::cout << old_map_offset;
      std::cout << "\n";
      std::cout << old_map_length;
      std::cout << "\n";
      std::cout << new_map_data;
      std::cout << "\n";
      std::cout << old_data_offset;
      std::cout << "\n";
      std::cout << old_data_length;
      std::cout << "\n";
      std::cout << settings_buffer;
      std::cout << "\n";
    */

    // theory is, we can now make a function like substr_replace and feed these in to it for example:
    settings_replace(old_data_offset, old_data_length);

    std::strcpy(settings_buffer, new_map_data);
    settings_replace(old_map_offset, old_map_length);

    settings_buffer[0] = new_map_size + 48;
    settings_buffer[1] = '\0';
    settings_replace(0, 1);

  return true;
 
}




// reads a specified setting
bool settings_read() {
   
  // reset the buffer just in case
  settings_buffer[0] = '\0';

  int map_size = eeprom[0] - 48;    // the 48 shift allows us to count starting at the ascii code of 0, so up to 10 settings will seem normal at least
  int data_offset = 1 + map_size;   // where the settings data sits

  // go through the map counting up the map_offset & data_offset as we go
  for ( int i = 1; i < map_size; i += 2 ) {

      // if the current setting 'character' is the one we want, great!
      if ( eeprom[i] == settings_code ) {
     
        // std::cout << "Found it!\n";

        // set the buffer to the value
        for ( int j = 0; j < eeprom[i + 1] - 48; j++ ) {

            settings_buffer[j] = eeprom[data_offset + j];
        }

        // close the string
        settings_buffer[(eeprom[i + 1] - 48) + 1] = '\0';

        break;

      }

      data_offset += eeprom[i + 1] - 48;
  }

  return true;
 
}




int main() {

  std::cout << eeprom;
  std::cout << '\n';


  settings_code = 'A';
  std::strcpy(settings_buffer, "howdy!");
  settings_write();

  std::cout << eeprom;
  std::cout << '\n';

//////////////////////////////////////////////////
// this is to debug the weird space at the end of the map that keeps appearing
for ( int j = 0; j < 10; j++ ) {

    std::cout << eeprom[j];
    std::cout << '\n';
    
}
//////////////////////////////////////////////////

/*
  settings_code = 'A';
  settings_read();

  std::cout << settings_buffer;
  std::cout << '\n';
*/
}
