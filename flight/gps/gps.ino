#include <SoftwareSerial.h>

SoftwareSerial GPSSerial(8, 7); // RX, TX

char GPSbuffer[100] = {'\0'};
int i;
int commas[10];
int j;
float GPSvals[6];

void setup() {
  i = 0;//index
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  GPSSerial.begin(9600);
}  
  

void loop() {
  getGPS();
  
}

void getGPS()
{
  if (GPSSerial.available())
  {
    
    GPSSerial.readBytesUntil('\n', GPSbuffer, 100);
    for(int k = 0; GPSbuffer[k]<82;k++){
      Serial.print(GPSbuffer[k]);
      if(GPSbuffer[k+1]=='\0'){
        Serial.print("apsdofasdfo\n");
        break;
      }
    }
    
  
  
    if( GPSbuffer[0]=='$' && GPSbuffer[4]=='M' && GPSbuffer[5]=='C')
    {
      GPSvals[0] = atof(GPSbuffer + commas[0]+1); //Time
      GPSvals[1] = atof(GPSbuffer + commas[2]+1); //Latitude
      GPSvals[2] = atof(GPSbuffer + commas[4]+1); //Longitude
      GPSvals[3] = atof(GPSbuffer + commas[6]+1); //Knots
      //Serial.println('poop');
    }

    else if(GPSbuffer[0]=='$' && GPSbuffer[4]=='G' && GPSbuffer[5]=='A')
    {
      GPSvals[4] = atof(GPSbuffer + commas[6]+1); //SatNum
      GPSvals[5] = atof(GPSbuffer + commas[8]+1); //Altitude
    }
    /*
    Serial.print(GPSvals[0]);
    Serial.print(",");
    Serial.print(GPSvals[1]);
    Serial.print(",");
    Serial.print(GPSvals[2]);
    Serial.print(",");
    Serial.print(GPSvals[3]);
    Serial.print(",");
    Serial.print(GPSvals[4]);
    Serial.print(",");
    Serial.println(GPSvals[5]);    
    */
  }
  
}

