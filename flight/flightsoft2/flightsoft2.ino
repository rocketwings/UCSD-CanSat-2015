#include <Wire.h>
#include <LSM303.h>
#include <LPS.h>
#include <L3G.h>
#include <SoftwareSerial.h>
#include <math.h>
#include <SPI.h> //needed for SD card reader
#include <SD.h> // SD library

#include "defStructs.h"

#define BAUD 9600
#define TIME_TOL  1  //time tolerance in percent
#define SEND_PER  1000  //send period in milliseconds
#define DATA_LENGTH 20  //length of data array
#define AVG_LENGTH 20  //length of avg array
#define L_CUSHION 9 // left cushion for averaging. if 9, takes average of 10 values to left including pos.
#define R_CUSHION 0 //disregard this for now. leave it 0.

#define CHIP_SELECT 10 //CS pin for SD card reader MUST be set as OUTPUT
#define PITOT_PIN 20//analog pin for pitot tube
#define PITOT_CAL 21  //calibration for the zero point of the differential pressure

#define RELEASE_ALTITUDE 400 // altitude set for release of cansat for container in meters

#define TEAM_ID "123456" // Team ID REMEMBER TO CHANGE THIS TOO LAZY TO LOOK UP

//--------------
// State Params
//--------------
//these will need to be preserved in case of processor reset.(save to sd card.)
boolean launched = false; //boolean for if system has launched
boolean released = false; //boolean for if cansat is released from container
boolean reachAlt = false; //boolean for if cansat has reached realease altitude on ascent
boolean GPSlock = false; //boolean for if cansat has a gps lock

//--------------

unsigned long last_send = 0;
unsigned long this_send = 0;
int pos = 0;
int packet_count = 0;
int pitotRead = 0;

L3G gyro;
LPS ps;
LSM303 compass;
//SoftwareSerial Xbee(8,15);  // RX, TX
SoftwareSerial Bridge(8,15); //Rx, Tx this will be the serial bridge between the two microcontrollers

float FakeGPS[5] = {1, 1, 1, 1, 1};
float gpsData[5] = {-1, -1, -1, -1, -1};

Packet_t data[DATA_LENGTH];
Avg_t avg[DATA_LENGTH] = {0};

//------------------------------------------------
// setup runs once
//------------------------------------------------

void setup() {
  // initialize serial communication at BAUD bits per second:
  //delay(3000);
  Serial.begin(BAUD);
  while (!Serial) {
    // wait for serial port to connect. Needed for Leonardo only
  }
	
  Serial.print("SD card setup...");
  pinMode(CHIP_SELECT, OUTPUT); // set CP as output
  if(!SD.begin(CHIP_SELECT))
  {
    Serial.println("Card Initialization Failure!");
    while(1);
  }
  else
  {
    Serial.println("Card Ready!");
  }
	
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
  
  
  //while(1);
}

//----------------------------------------------------
// the loop routine runs over and over again forever:
//----------------------------------------------------

void loop() {        
	
  getData(pos);
	
  freqLimiter(pos);

  pos++;
  if(pos >= DATA_LENGTH){
    pos = DATA_LENGTH - 1;
		shiftDataLeft();
		avgGenerator(pos);
		shiftAvgLeft(); 
		launchIndicator();
  }
  
}

//----------------------------------------------------
// functions
//----------------------------------------------------

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

void getData(int pos){
	// read data
  data[pos].time = millis();
  // pololu
	data[pos].pressure = ps.readPressureMillibars();
  data[pos].altitude = ps.pressureToAltitudeMeters(data[pos].pressure);
  data[pos].temp = ps.readTemperatureC();
  gyro.read();
  data[pos].gyro_x = gyro.g.x;
  data[pos].gyro_y = gyro.g.y;
  data[pos].gyro_z = gyro.g.z;
  compass.read();
  data[pos].compass_ax = compass.a.x;
  data[pos].compass_ay = compass.a.y;
  data[pos].compass_az = compass.a.z;
  data[pos].compass_mx = compass.m.x;
  data[pos].compass_my = compass.m.y;
  data[pos].compass_mz = compass.m.z;
  //pitot
	pitotRead = analogRead(PITOT_PIN);
	// Serial.println(pitotRead);
  data[pos].airspeed = sqrt(2000.*(((pitotRead-PITOT_CAL)/(0.2*1024.0))-2.5)/1.225);
	//GPS
	data[pos].latitude = gpsData[];
	data[pos].longitude = gpsData[];
	data[pos].gpsalt = gpsData[];
	data[pos].satnum = gpsData[];
	data[pos].gpsspeed = gpsData[];
	//put rest of gps code here
}

void serialMonitor(int pos){
	Serial.print(TEAM_ID); //team ID
	Serial.print(",");
	Serial.print(packet_count); 
	Serial.print(",");
	Serial.print(avg[pos].altitude);
	Serial.print(",");
	Serial.print(avg[pos].pressure);
	Serial.print(",");
	Serial.print(avg[pos].airspeed);
	Serial.print(",");
	Serial.print(avg[pos].temp);
	Serial.print(",");
	Serial.print(data[pos].voltage);
	Serial.print(",");
	Serial.print(data[pos].latitude); //latitude
	Serial.print(",");
	Serial.print(data[pos].longitude);//longitude
	Serial.print(",");
	Serial.print(data[pos].gpsalt);//altitude
	Serial.print(",");
	Serial.print(data[pos].satnum);// sat num
	Serial.print(",");
	Serial.print(data[pos].gpsspeed);// gps speed
	Serial.print(",");
	Serial.print(data[pos].time);// time
	Serial.print(",");
	Serial.print(data[pos].imgcmdTime);// time of last imaging command
	Serial.print(",");
	Serial.print(data[pos].imgcmdCount);// number of imaging commands
	Serial.print(",");
	Serial.println(data[pos].bonus);
}

void bridgeSend(int pos){
	Bridge.print(TEAM_ID); //team ID
	Bridge.print(",");
	Bridge.print(packet_count); 
	Bridge.print(",");
	Bridge.print(avg[pos].altitude);
	Bridge.print(",");
	Bridge.print(avg[pos].pressure);
	Bridge.print(",");
	Bridge.print(avg[pos].airspeed);
	Bridge.print(",");
	Bridge.print(avg[pos].temp);
	Bridge.print(",");
	Bridge.print(data[pos].voltage);
	Bridge.print(",");
	Bridge.print(data[pos].latitude); //latitude
	Bridge.print(",");
	Bridge.print(data[pos].longitude);//longitude
	Bridge.print(",");
	Bridge.print(data[pos].altitude);//altitude
	Bridge.print(",");
	Bridge.print(data[pos].satnum);// sat num
	Bridge.print(",");
	Bridge.print(data[pos].gpsspeed);// gps speed
	Bridge.print(",");
	Bridge.print(data[pos].time);// time
//	Bridge.print(",");
//	Bridge.print(data[pos].imgcmdTime);// time of last imaging command
//	Bridge.print(",");
//	Bridge.print(data[pos].imgcmdCount);// number of imaging commands
//	Bridge.print(",");
//	Bridge.println(data[pos].bonus);
}

void freqLimiter(int pos){
	this_send = millis();
  if (this_send - last_send > .5*SEND_PER)  {
    if (this_send%1000 < TIME_TOL*SEND_PER/100. || this_send%1000 > (100-TIME_TOL)*SEND_PER/100)  {
			last_send = this_send;
      packet_count++;
			
			//logData(pos);		
			//xbeeSend(pos);
			serialMonitor(pos);
		}
	}
	else  {
    delay(1);        // delay in between reads for stability
  }
}

avgGenerator(pos){
	int leftIdx;
	int rightIdx;
	if(pos - L_CUSHION >= 0){
		leftIdx = pos - L_CUSHION;
	}
	else{
		leftIdx = 0;
	}
	if(pos + R_CUSHION <= DATA_LENGTH - 1){
		rightIdx = pos - R_CUSHION;
	}
	else{
		rightIdx = DATA_LENGTH - 1;
	}
	
	for(int i = leftIdx; i < rightIdx; i++){
		avg[pos].pressure += data[i].pressure;
		avg[pos].temp += data[i].temp;
		avg[pos].altitude += data[i].altitude;
		avg[pos].airspeed += data[i].airspeed;
	}
	avg[pos].pressure = avg[pos].pressure / (rightIdx - leftIdx + 1);
	avg[pos].temp = avg[pos].temp / (rightIdx - leftIdx + 1);
	avg[pos].altitude = avg[pos].altitude / (rightIdx - leftIdx + 1);
	avg[pos].airspeed = avg[pos].airspeed / (rightIdx - leftIdx + 1);	
}

releaseTrigger(int pos){
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
		sumTime += float(data[n].time);
		sumTtime2 += float(data[n].time * data[n].time);
		crossSum += float(data[n].time) * avg[n].altitude;
	}
	
	slope = (AVG_LENGTH * crossSum - sumAlt*sumTime)/(AVG_LENGTH * sumTtime2 - sumTime * sumTime);
	
	if (slope >= LAUNCH_VELOCITY){
		launched = True;
	}
	
	return slope;
}

stateIni(){
	// will set state variables from other arduino. (SD card to serial bridge)
}

shiftAvgLeft(){
	for(int i=0; i<AVG_LENGTH - 2; i++){
		avg[i] = avg[i+1];
	}
}

shiftDataLeft(){
	for(int i=0; i<DATA_LENGTH - 2; i++){
		data[i] = data[i+1];
	}
}