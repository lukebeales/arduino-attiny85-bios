// https://www.onlinegdb.com/online_c++_compiler
#include <iostream>
#include <cstring>

// so the format is: map_size, map, data
// with map being two characters for each data block: position, and length.
char eeprom[] = "6ATB8D8....................................--------++++++++";

char settings_code;
char settings_buffer[64];   // this is wrong


// this writes the specific setting to the eeprom, rearranging settings as it goes
bool settings_write() {
   
  int map_size = eeprom[0] - 48;    // this allows us to count starting at the ascii code of 0, so up to 10 settings will seem normal at least
  int map_start = 1;  // where in the map the setting will sit
  int map_length = 0;

  char map_data[2];
  if ( sizeof(settings_buffer) > 0 ) {
      map_data[0] = settings_code;
      map_data[1] = std::char_traits<char>::length(settings_buffer) + 48;
  } else {
      map_data[0] = '\0';
  }

  int data_start = 1 + map_size; // where the settings data will sit
  int data_length = 0;

  // go through the map counting up the map_offset & data_offset as we go
  for ( int i = 1; i < map_size; i += 2 ) {

        // if the current setting 'character' is the one we want, great!
        if ( eeprom[i] == settings_code ) {
     
            std::cout << "Found it to replace!\n";

            map_start = i;
            map_length = 2;
            data_length = (eeprom[i + 1] - 48);

            if ( std::char_traits<char>::length(map_data) == 0 ) {
                map_size -= 2;
            }

            break;

        // otherwise if the current setting 'character' is after the one we want, then we didn't find it
        } else if ( eeprom[i] > settings_code ) {

            std::cout << "New entry in the middle somewhere!\n";

            map_start = i;
            map_length = 2;

            if ( sizeof(map_data) > 0 ) {
                map_size += 2;
            }

            break;

        } else {

            std::cout << "Nothing matched, add up some numbers\n";
            data_start += (eeprom[i + 1] - 48);  // allows us to use some ascii numbers to start with.

        }

    // if we're the last run, it means we're a new instruction on the end.
    if ( i == 1 + (map_size - 2) ) {
        map_start = map_size;
    }

  }

  // if we ended up with a new instruction on the end
  if ( map_start == (1 + map_size) ) {
      map_size += 2;
  }

  std::cout << map_size;
  std::cout << "\n";
  std::cout << map_start;
  std::cout << "\n";
  std::cout << map_length;
  std::cout << "\n";
  std::cout << map_data;
  std::cout << "\n";
  std::cout << data_start;
  std::cout << "\n";
  std::cout << data_length;
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
