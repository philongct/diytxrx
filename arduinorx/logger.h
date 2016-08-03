#ifndef __LOGGER_h__
#define __LOGGER_h__

uint8_t globalLogLevel = 2;

int serial_putc( char c, FILE * ) {
  Serial.write( c );

  return c;
} 

void printf_begin() {
  Serial.begin(115200);
  fdevopen( &serial_putc, 0 );
}
  
void printlog(uint8_t level, const char* message, ...) {
  if (globalLogLevel >= level) {
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    printf("\n");
  }
}

#endif
