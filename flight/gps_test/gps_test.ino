/*
  Software serial multple serial test
 
 Receives from the hardware serial, sends to software serial.
 Receives from software serial, sends to hardware serial.
 
 The circuit: 
 * RX is digital pin 10 (connect to TX of other device)
 * TX is digital pin 11 (connect to RX of other device)
 
 Note:
 Not all pins on the Mega and Mega 2560 support change interrupts, 
 so only the following can be used for RX: 
 10, 11, 12, 13, 50, 51, 52, 53, 62, 63, 64, 65, 66, 67, 68, 69
 
 Not all pins on the Leonardo support change interrupts, 
 so only the following can be used for RX: 
 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI).
 
 created back in the mists of time
 modified 25 May 2012
 by Tom Igoe
 based on Mikal Hart's example
 
 This example code is in the public domain.
 
 */
#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX
char GPSbuffer[82];
int i;
int commas[10];
int j;

void setup()  
{
  i = 0;//index
  // Open serial communications and wait for port to open:
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }


  Serial.println("Goodnight moon!");

  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);
}

void loop() // run over and over
{
  if (mySerial.available())
  {
    mySerial.readBytesUntil('\n', GPSbuffer, 82);
  }
  if( GPSbuffer[0]=='$' && GPSbuffer[4]=='M' && GPSbuffer[5]=='C')
  {
    Serial.println(GPSbuffer);
    j = 0;
    for(int i=0; i<82 && j<10; i++)  {
      if(GPSbuffer[i] == ',')
      {
        commas[j] = i;
        j++;
      }
    }
    Serial.print("time:");
    for(int i=(commas[0]+1); i<commas[1]; i++)
    {
      Serial.print(GPSbuffer[i]);
    }
    Serial.print("\tlat:");
    for(int i=commas[2]+1; i<commas[3]; i++)
    {
      Serial.print(GPSbuffer[i]);
    }
    Serial.print("\tlong:");
    for(int i=commas[4]+1; i<commas[5]; i++)
    {
      Serial.print(GPSbuffer[i]);
    }
    Serial.print("\tknots:");
    for(int i=commas[6]+1; i<commas[7]; i++)
    {
      Serial.print(GPSbuffer[i]);
    }
    Serial.print("\n");
  }
  else if( GPSbuffer[0]=='$' && GPSbuffer[4]=='G' && GPSbuffer[5]=='A')  {
    Serial.println(GPSbuffer);
    j = 0;
    for(int i=0; i<82 && j<10; i++)  {
      if(GPSbuffer[i] == ',')
      {
        commas[j] = i;
        j++;
      }
    }
    Serial.print("numSat:");
    Serial.print(atoi(GPSbuffer+(commas[6]+1)));
    Serial.print("\n");
    Serial.print("numSat:");
    for(int i=commas[6]+1; i<commas[7]; i++)  {
      Serial.print(GPSbuffer[i]);
    }
    Serial.print("\taltitude:");
    for(int i=commas[8]+1; i<commas[9]; i++)  {
      Serial.print(GPSbuffer[i]);
    }
    Serial.print("\n");
  }
}

