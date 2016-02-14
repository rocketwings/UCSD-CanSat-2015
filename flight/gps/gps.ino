#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX

char GPSbuffer[82];
int i;
int commas[10];
int j;

void setup() {
  i = 0;//index
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  mySerial.begin(9600);
}  
  

void loop() {

}


