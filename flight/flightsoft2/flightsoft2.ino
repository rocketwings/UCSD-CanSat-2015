/*
 * Goes on the RED sparkfun micro
 * 
 */


#include <Wire.h>
#include <LSM303.h>
#include <LPS.h>
#include <L3G.h>
#include <SoftwareSerial.h>
#include <math.h>
#include <Adafruit_GPS.h>

#include "defStructs.h"

#define GPSECHO  true
#define BAUD 9600
#define TIME_TOL  1  //time tolerance in percent
#define SEND_PER  1000  //send period in milliseconds
#define DATA_LENGTH 15  //length of data array
#define AVG_LENGTH 15  //length of avg array

#define PITOT_PIN 20//analog pin for pitot tube
#define PITOT_CAL 31  //calibration for the zero point of the differential pressure

#define RELEASE_ALTITUDE 400 // altitude set for release of cansat for container in meters

#define TEAM_ID "123456" // Team ID REMEMBER TO CHANGE THIS TOO LAZY TO LOOK UP

#define LAUNCH_VELOCITY 100

//--------------
// State Params
//--------------
//these will need to be preserved in case of processor reset.(save to sd card.)
boolean launched = false; //boolean for if system has launched
boolean released = false; //boolean for if cansat is released from container
boolean reachAlt = false; //boolean for if cansat has reached realease altitude on ascent
boolean GPSlock = false; //boolean for if cansat has a gps lock

//--------------

unsigned long last_send = 0; // time of last sending event
unsigned long this_send = 0; 
unsigned long missionTime = 0;
int imgCmdCount = 0;
unsigned long imgCmdTime = 0;

int pos = 0; // index
int packet_count = 0;
int pitotRead = 0;

L3G gyro;
LPS ps;
LSM303 compass;
SoftwareSerial Bridge(9,8); //Rx, Tx this will be the serial bridge between the two microcontrollers

Adafruit_GPS GPS(&Serial1);

//float FakeGPS[5] = {1, 1, 1, 1, 1};
//float gpsData[5] = {-1, -1, -1, -1, -1};

Packet_t data[DATA_LENGTH];
Avg_t avg[DATA_LENGTH] = {0};


boolean usingInterrupt = false;
void useInterrupt(boolean);//prototype function
//------------------------------------------------
// setup runs once
//------------------------------------------------

void setup() {
  // initialize serial communication at BAUD bits per second:
  //
  Serial.begin(9600);
  //while (!Serial) {
    // wait for serial port to connect. Needed for Leonardo only
  //}
	//Ini Bridge
  Bridge.begin(9600);
  Serial.println("Bridge initialized");
	//GPS
  GPS.begin(9600);
  Serial.println("GPS initialized");
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  Serial.println("GPS initialized");
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  Serial.println("GPS initialized");
  useInterrupt(true);
  Serial.println("GPS initialized");
  //-------------
  	
  Serial.println("lkdsfoijewaflkjfds");
  Wire.begin();

  if (!ps.init()) {
    Serial.println("Failed to autodetect pressure sensor!");
  }
  Serial.println("1");
  if (!gyro.init()) {
    Serial.println("Failed to autodetect gyro!");
  }
  Serial.println("2");
  if (!compass.init()) {
    Serial.println("Failed to autodetect compass!");
  }
  Serial.println("3");
  gyro.enableDefault();
  ps.enableDefault();
  compass.enableDefault();
  Serial.println("lkdsfoijewaflkjfds2");
  pinMode(PITOT_PIN, INPUT);
  
  
  //while(1);
  //delay(1000);
}

//----------------------------------------------------
// the loop routine runs over and over again forever:
//----------------------------------------------------

void loop() {        
  getData(pos);
	
  freqLimiter(pos);
  static bool filled = false;
 
  if(pos == DATA_LENGTH){
    filled = true;
    pos = 0;   
  }
  if(filled == true){
      //Serial.println("LIN IS AN ASS");
      avgGenerator(pos);
      launchIndicator();//try adding accelerometer data as well for detection
      //Serial.print(avg[pos].airspeed);
      //Serial.print(",");
      //Serial.println(data[pos].airspeed);
  }
  //Serial.println(pos);
  //delay(500); 
  pos++;
  
}

//----------------------------------------------------
// functions
//----------------------------------------------------
/*
void logData(int pos)
{
   File logFile = SD.open("telemetry.txt", FILE_WRITE);
   if(logFile)
   {
			logFile.print(TEAM_ID); //team ID
			logFile.print(",");
			logFile.print(packet_count); 
			logFile.print(",");
			logFile.print(avg[pos].altitude);
			logFile.print(",");
			logFile.print(avg[pos].pressure);
			logFile.print(",");
			logFile.print(avg[pos].airspeed);
			logFile.print(",");
			logFile.print(avg[pos].temp);
			logFile.print(",");
			logFile.print(data[pos].voltage);
			logFile.print(",");
			logFile.print(data[pos].latitude); //latitude
			logFile.print(",");
			logFile.print(data[pos].longitude);//longitude
			logFile.print(",");
			logFile.print(data[pos].gpsalt);//altitude
			logFile.print(",");
			logFile.print(data[pos].satnum);// sat num
			logFile.print(",");
			logFile.print(data[pos].gpsspeed);// gps speed
			logFile.print(",");
			logFile.print(data[pos].time);// time
			logFile.print(",");
			logFile.print(data[pos].imgcmdTime);// time of last imaging command
			logFile.print(",");
			logFile.print(data[pos].imgcmdCount);// number of imaging commands
			logFile.print(",");
			logFile.println(data[pos].bonus); //bonus data if applicable
			logFile.close();
   }
		else Serial.println("Error opening file! NOOOOO!!!!!!");
		return;
}
*/
void getData(int pos){
	// read data
  
  // pololu
	data[pos].pressure = ps.readPressureMillibars();
  data[pos].altitude = ps.pressureToAltitudeMeters(data[pos].pressure);
  data[pos].temp = ps.readTemperatureC();

//  compass.read();
//  data[pos].compass_ax = compass.a.x;
//  data[pos].compass_ay = compass.a.y;
//  data[pos].compass_az = compass.a.z;

  //pitot
	pitotRead = analogRead(PITOT_PIN);
  data[pos].airspeed = sqrt(2000.*(((pitotRead-PITOT_CAL)/(0.2*1024.0))-2.5)/1.225);
	//GPS
  if (! usingInterrupt) {
    // read data from the GPS in the 'main loop'
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
    if (GPSECHO)
      if (c) Serial.print(c);
  }
  
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences! 
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false
  
    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
  }
  static unsigned long GPStime = GPS.hour * (1000*60*60) + GPS.minute * (1000*60) + GPS.seconds * (1000) + GPS.milliseconds;
  missionTime = millis() + GPStime;
  //--------------------------------------------------------------
	//GPS.latitude = gpsData[0];
	//GPS.longitude = gpsData[1];
	//GPS.altitude = gpsData[2];
	//GPS.satellites = gpsData[3];
	//GPS.speed = gpsData[4];
	//put rest of gps code here

  //camera and params (params if requested.)
  getBridge();
}

void serialMonitor(int pos){
  /*
	Serial.print(TEAM_ID); //team ID
	Serial.print(",");
	Serial.print(packet_count); 
	Serial.print(",");
	Serial.print(data[pos].altitude);
	Serial.print(",");
	Serial.print(data[pos].pressure);
	Serial.print(",");
	Serial.print(avg[pos].airspeed);
	Serial.print(",");
	Serial.print(data[pos].temp);
	Serial.print(",");
	Serial.print(data[pos].voltage);
	Serial.print(",");
	Serial.print(GPS.latitude); //latitude
  Serial.print(",");
  Serial.print(GPS.longitude);//longitude
  Serial.print(",");
  Serial.print(GPS.altitude);//altitude
  Serial.print(",");
  Serial.print(GPS.satellites);// sat num
  Serial.print(",");
  Serial.print(GPS.speed);// gps speed
  Serial.print(",");
	Serial.print(missionTime);// time
	Serial.print(",");
	Serial.print(imgCmdTime);// time of last imaging command
	Serial.print(",");
	Serial.print(imgCmdCount);// number of imaging commands
	Serial.print(",");
	Serial.print(data[pos].bonus);
  Serial.print("\n");
  //Serial.print((int)&GPS.latitude);
  //Serial.print('\t');
  //Serial.print((int)&data[pos].temp);
  //Serial.print("\n");
  */
}

void bridgeSend(int pos){
	Bridge.print(TEAM_ID); //team ID
	Bridge.print(",");
	Bridge.print(packet_count); 
	Bridge.print(",");
	Bridge.print(avg[pos].altitude,2);
	Bridge.print(",");
	Bridge.print(avg[pos].pressure,2);
	Bridge.print(",");
	Bridge.print(avg[pos].airspeed,2);
	Bridge.print(",");
	Bridge.print(avg[pos].temp,2);
	Bridge.print(",");
	Bridge.print(data[pos].voltage,2);
	Bridge.print(",");
	Bridge.print(GPS.latitude); //latitude
	Bridge.print(",");
	Bridge.print(GPS.longitude);//longitude
	Bridge.print(",");
	Bridge.print(GPS.altitude);//altitude
	Bridge.print(",");
	Bridge.print(GPS.satellites);// sat num
	Bridge.print(",");
	Bridge.print(GPS.speed);// gps speed
	Bridge.print(",");
	Bridge.print(missionTime);// time
	Bridge.print(",");
	// State Params
	Bridge.print(launched);// time of last imaging command
	Bridge.print(released);// number of imaging commands
	Bridge.print(reachAlt);
	Bridge.print(GPSlock);
  Bridge.print("\n");

  Serial.print(TEAM_ID); //team ID
  Serial.print(",");
  Serial.print(packet_count); 
  Serial.print(",");
  Serial.print(avg[pos].altitude,2);
  Serial.print(",");
  Serial.print(avg[pos].pressure,2);
  Serial.print(",");
  Serial.print(avg[pos].airspeed,2);
  Serial.print(",");
  Serial.print(avg[pos].temp,2);
  Serial.print(",");
  Serial.print(data[pos].voltage,2);
  Serial.print(",");
  Serial.print(GPS.latitude); //latitude
  Serial.print(",");
  Serial.print(GPS.longitude);//longitude
  Serial.print(",");
  Serial.print(GPS.altitude);//altitude
  Serial.print(",");
  Serial.print(GPS.satellites);// sat num
  Serial.print(",");
  Serial.print(GPS.speed);// gps speed
  Serial.print(",");
  Serial.print(missionTime);// time
  Serial.print(",");
  // State Params
  Serial.print(launched);// time of last imaging command
  Serial.print(released);// number of imaging commands
  Serial.print(reachAlt);
  Serial.print(GPSlock);
  Serial.print("\n");
}

void freqLimiter(int pos){
	this_send = millis();
  if (this_send - last_send > .5*SEND_PER)  {
    if (this_send%1000 < TIME_TOL*SEND_PER/100. || this_send%1000 > (100-TIME_TOL)*SEND_PER/100)  {
			last_send = this_send;
      packet_count++;
			
			//logData(pos);		
			//xbeeSend(pos);
      bridgeSend(pos);
			serialMonitor(pos);
      
		}
	}
	else  {
    delay(1);        // delay in between reads for stability
  }
}

void avgGenerator(int pos){
  avg[pos] = {0};
	
	for(int i = 0; i < DATA_LENGTH ; i++){
		avg[pos].pressure = avg[pos].pressure+data[i].pressure;
		avg[pos].temp = avg[pos].temp+data[i].temp;
		avg[pos].altitude = avg[pos].altitude+data[i].altitude;
		avg[pos].airspeed = avg[pos].airspeed+data[i].airspeed;
	}
	avg[pos].pressure /=  DATA_LENGTH;
	avg[pos].temp /= DATA_LENGTH;
	avg[pos].altitude /= DATA_LENGTH;
	avg[pos].airspeed /= DATA_LENGTH;	
}

void releaseTrigger(int pos){
	if(avg[pos].altitude >= RELEASE_ALTITUDE){
		reachAlt = true;
	}
	if(launched && reachAlt){
		if(avg[pos].altitude <= RELEASE_ALTITUDE){
			// Release code analogWrite(LED_PIN, 127);
		}
	}
}

float launchIndicator(){
	float slope = 0.0;
	float sumAlt = 0.0;
	float sumAlt2 = 0.0;
	float sumTime = 0.0;
	float sumTtime2 = 0.0;
	float crossSum = 0.0;
	
	for(int n=0; n<AVG_LENGTH; n++){
		sumAlt += avg[n].altitude;
		sumAlt2 += avg[n].altitude * avg[n].altitude;
		sumTime += float((missionTime/1000));
		sumTtime2 += float((missionTime/1000) * (missionTime/1000));
		crossSum += float(missionTime/1000) * avg[n].altitude;
	}
	
	slope = (AVG_LENGTH * crossSum - sumAlt*sumTime)/(AVG_LENGTH * sumTtime2 - sumTime * sumTime);
	
	if (slope >= LAUNCH_VELOCITY){
		launched = true;
	}
	
	return slope;
}

void getBridge(){
	// will set state variables and get necessary data from other arduino. (SD card to serial bridge)
  if(Bridge.available()){
    char buff[25] = {'\0'};
    Bridge.readBytesUntil('\n',buff,25);
    if(buff[0] == 'p'){
      if(buff[1] == '1'){
        launched == true;
      }
      else launched == false;
      if(buff[2] == '1'){
        released == true;
      }
      else released == false;
      if(buff[3] == '1'){
        reachAlt == true;
      }
      else reachAlt == false;
      if(buff[4] == '1'){
        GPSlock == true;
      }
      else GPSlock == false;
    }
    if(buff[0] == 'c'){
      int comma = 0;
      for(int i;i<25;i++){
        if(buff[i]==','){
          comma = i;
        }
      }
    imgCmdCount = atoi(buff+1);
    imgCmdTime = strtoul(buff+comma,NULL,10); 
    }
  }
}

SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
#ifdef UDR0
  if (GPSECHO)
    if (c) UDR0 = c;  
    // writing direct to UDR0 is much much faster than Serial.print 
    // but only one character can be written at a time. 
#endif
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

void releaseSat(){
  //Release code here
  
}

