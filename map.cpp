// https://www.onlinegdb.com/online_c++_compiler
#include <iostream>
#include <cstring>

// so the format is: map_size, map, data
// with map being two characters for each data block: position, and length.
char eeprom[] = "6ATB8D8....................................--------++++++++";

char settings_code;
char settings_buffer[64];


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
     
            std::cout << "Found it to replace!\n";

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

            std::cout << "New entry in the middle somewhere!\n";

            old_map_offset = i;
            old_map_length = 2;

            // if there's data to insert...
            if ( std::char_traits<char>::length(new_map_data) > 0 ) {
                new_map_size += 2;  // increase the map size to accommodate the new data
            }

            break;

        } else {

            std::cout << "Nothing matched, add up some numbers\n";
            old_data_offset += (eeprom[i + 1] - 48);  // allows us to use some ascii numbers to start with.

        }

    // if we're the last run, it means we're a new instruction on the end.
    if ( i == 1 + (old_map_size - 2) ) {
        old_map_offset = old_map_size;
    }

  }

  // if we ended up with a new instruction on the end
  if ( old_map_offset == (1 + old_map_size) ) {
      new_map_size += 2;
  }

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

    // theory is, we can now make a function like substr_replace and feed these in to it for example:
    // settings_replace(data_start, data_length);

    // std::strcpy(settings_buffer, map_data);
    // settings_replace(map_start, map_length);

    // std::strcpy(settings_buffer, map_size);
    // settings_replace(0, 1, map_size)

  return true;
 
}


// i know this wastes clock cycles, but they are free whereas my brain and the chips memory isn't!
void settings_replace(int offset, int length) {

    int data_size = 0;
    int map_size = eeprom[0] - 48;
    int difference = std::char_traits<char>::length(settings_buffer) - length;

    // go through the whole map, counting how large the data is
    for ( int i = 1; i < map_size; i += 2 ) {
        data_size = (eeprom[i] + 1) - 48;
    }
   
    // if difference > 0
        // shift all data to the right
        for ( int i = 1 + map_size + data_size; i > offset + (length - difference); i-- ) {
            eeprom[i + difference] = eeprom[i];
        }
    // elseif ( difference < 0 )
        // shift all data to the left
        for ( int i = offset + (length + difference); i < 1 + map_size + data_size; i++ ) {
            eeprom[i + difference] = eeprom[i];
        }
    //
   

    // now write the buffer
    for ( int i = 0; i < sizeof(settings_buffer); i++ ) {
        eeprom[offset + i] = settings_buffer[i];
    }

}



int main() {
  // std::string name;
  // std::cout << "What is your name? ";
  // getline (std::cin, name);
  // std::cout << "Hello, " << name << "!\n";

  settings_code = 'A';

  std::strcpy(settings_buffer, "hello");

  settings_write();

}
