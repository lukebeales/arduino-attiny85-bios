/*

              +-----+
 reset  5 ----|     |---- VCC
    TX  3 ----|     |---- 2/A1  interrupt
    RX  4 ----|     |---- 1     switch
   GND    ----|     |---- 0     status led
              +-----+

  Prefix with BIOS+:
  GET=[A-z0-9]
  SET=[A-z0-9],value
  ON
  OFF
  RND
  CYCLE
  UPTIME
  TIME[=12345]
  HELP
  RESET
  ZZZ
  SHH
  PING
  
  ~ = charge delay
  + = time on
  - = time off
  ! = enable interrupt (disabling this saves power)
  

*/




char global_buffer[100];
