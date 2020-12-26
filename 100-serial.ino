#include <SoftwareSerial.h>

char serial_buffer[64];
SoftwareSerial serial(0, 1); // RX, TX


/////////////////////////////////

void serial_start() {
  pinMode(0, INPUT);
  pinMode(1, OUTPUT);

  serial.begin(9600);
  delay(200);
}


void serial_end() {
  // close the serial connection so we can hand it back to arduino
  serial.end();
}


boolean serial_read() {

  // clear the buffer
  serial_buffer[0] = '\0';
  byte serial_buffer_read = 0;

  // if there's data to see (this is the initial kick to initiate the loop)
  if ( serial.available() > 0 ) {

    byte limit = 100;
    byte i = 0;

    while (
      ( i < limit )
    ){

      // if there's a byte waiting in the buffer for us
      if ( serial.available() > 0 ) {

        // read the data...
        byte serial_buffer_byte = serial.read();
  
        // if it's not a newline
        if ( serial_buffer_byte != 10 ) {
  
          // add it to the buffer
          serial_buffer[serial_buffer_read] = serial_buffer_byte;
          serial_buffer_read++;
  
/* i don't think this matters as it resets the chip anyway
          // just to make sure the buffer doesn't overflow
          if ( serial_buffer_read == 63 ) {
            // end it.
            serial_buffer[serial_buffer_read] = '\0';
            i = limit;
          }
*/

          // reset the loop
          i = 0;
          
        } else {
          serial_buffer[serial_buffer_read] = '\0';  // this 'terminates' the string
  
          // cancel the loop
          i = limit;

        }

      }
      
      i++;

    }
    
    // if the length of the buffer is > 0 then return true
    if ( serial_buffer_read > 0 ) {
      return true;
    }

  }

  return false;

}
