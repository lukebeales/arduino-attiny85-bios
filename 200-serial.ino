#include <SoftwareSerial.h>

// i make these global because c++ is annoying with pointers :(
char serial_bookend[8];  // what we should discard first on each read
SoftwareSerial serial(4, 3);  // RX, TX


/////////////////////////////////

// flush the buffer. this removes any pesky leftover bits
void serial_flush(Stream &handle) {
  while ( handle.available() > 0 ) {
    char t = handle.read();
  }
  global_buffer[0] = '\0';
}


void serial_transmitting(boolean am_i) {
  if ( am_i == true ) {
    pinMode(3, OUTPUT);
    // make sure things have time.
    serial.print((char)0);    // null character, just something bogus so the output isn't jumbled
    serial.flush();
  } else {
    // block everything out for a bit
    serial.flush();

    // *** THIS IS ABSOLUTELY KEY
      delay(50);  // this just needs to be long enough to block the error message from the ESP12F
    // ***

    pinMode(3, INPUT);
  }
}



void serial_start() {
  pinMode(4, INPUT);
  pinMode(3, OUTPUT);

  // from now on we require serial inputs to be formatted as BIOS+COMMAND=value
  serial_bookend[0] = 'B';
  serial_bookend[1] = 'I';
  serial_bookend[2] = 'O';
  serial_bookend[3] = 'S';
  serial_bookend[4] = '+';
  serial_bookend[5] = '\0';

  serial.begin(9600);
  delay(200);

  serial.listen();

  serial_flush(serial);
}


void serial_end() {
  // close the serial connection so we can hand it back to arduino
  serial.end();
}


// this allows us to discard anything in the buffer until we find a match
boolean serial_read(Stream &handle) {

  // clear the buffer
  global_buffer[0] = '\0';
  byte global_buffer_read = 0;

  // if there's data to see (this is the initial kick to initiate the loop)
  if ( handle.available() > 0 ) {

    unsigned long limit = 10000;
    unsigned long i = 0;
    boolean received = false;

    // whether we need to discard the bits prior to the bookend...
    byte bookend_size = strlen(serial_bookend);
    byte bookend_used = 0;
    boolean bookend_done = true;
    if ( bookend_size > 0 ) {
      bookend_done = false;
    }

    while ( i < limit ) {

      // this should prevent the odd flash stoppage while we're waiting for serial commands
      at_led_flash();
      
      // if there's a byte waiting in the buffer for us
      if ( handle.available() > 0 ) {

        // discard everything up until a match
        if ( bookend_done == false ) {

          bookend_used = strlen(global_buffer);

          // this just stacks up the string until we reach the size we're looking for
          if ( bookend_used < bookend_size ) {
            global_buffer[bookend_used] = handle.read();
            global_buffer[bookend_used + 1] = '\0';

          // the string must be at the size, we need to shift it all to the left
          } else {
            for ( int j = 1; j < bookend_size; j++ ) {
              global_buffer[j-1] = global_buffer[j];
            }
            global_buffer[bookend_size-1] = handle.read();
            global_buffer[bookend_size] = '\0';
          }

          // now lets see if the serial buffer is equal to the serial bookend that we're looking for...
          if ( strcmp(global_buffer, serial_bookend) == 0 ) {
            // reset the buffer ready for the data
            global_buffer[0] = '\0';

            // break out of the bookend discardy bit
            bookend_done = true;
          }
          
          // reset the loop
          i = 0;
        
        } else {
        
          // let us know something has been received
          received = true;
  
          // read the data...
          byte global_buffer_byte = handle.read();

          // if it's not a newline
          if ( global_buffer_byte != 10 ) {
    
            // if it's not a carriage return - this makes it cr/lf friendly
            if (
              ( global_buffer_byte != 0 ) &&   // null character
              ( global_buffer_byte != 13 ) &&
              ( global_buffer_byte != 2 ) &&
              ( global_buffer_byte != 3 ) &&
              ( global_buffer_read < 100 )
            ) {
  
              // add it to the buffer
              global_buffer[global_buffer_read] = global_buffer_byte;
              global_buffer_read++;
  
            }
    
            // reset the loop
            i = 0;
            
          } else {
            global_buffer[global_buffer_read] = '\0';  // this 'terminates' the string
    
            // cancel the loop
            i = limit;
  
          }
  
        }

      }
            
      i++;

    }
    
    // if we received anything...even if it's just a newline/carriage return
    if ( received == true ) {

      return true;

    }

  }

  // empty the buffer as it could be full of the bookend data
  global_buffer[0] = '\0';
  return false;

}
