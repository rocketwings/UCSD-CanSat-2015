#include <Wire.h>
#include <SoftwareSerial.h>
#include <math.h>
#include <SPI.h> //needed for SD card reader
#include <SD.h> // SD library

#include "defStructs.h"

#define BAUD 9600
#define DATA_LENGTH 20  //length of data array
#define AVG_LENGTH 20  //length of avg array

#define CHIP_SELECT 10 //CS pin for SD card reader MUST be set as OUTPUT

#define RELEASE_ALTITUDE 400 // altitude set for release of cansat for container in meters

#define TEAM_ID "123456" // Team ID REMEMBER TO CHANGE THIS TOO LAZY TO LOOK UP

#define RELEASE 'r'
#define TAKE_PIC 'c'
//--------------
// State Params
//--------------
//these will need to be preserved in case of processor reset.(save to sd card.)
boolean launched = false; //boolean for if system has launched
boolean released = false; //boolean for if cansat is released from container
boolean reachAlt = false; //boolean for if cansat has reached realease altitude on ascent
boolean GPSlock = false; //boolean for if cansat has a gps lock

//--------------

int pos = 0;
int camCmdCount = 0;
unsigned long camCmdTime = 0;
unsigned long timeSync = 0;
unsigned long timeCheck = 0;

SoftwareSerial Bridge(8,15); //Rx, Tx this will be the serial bridge between the two microcontrollers
SoftwareSerial Xbee(3,4); // Rx,Tx subject to change.


//------------------------------------------------
// setup runs once
//------------------------------------------------

void setup() {
	// Serial for debug
	Serial.begin(BAUD);
	//bridge setup
	Bridge.begin(BAUD);
	//xbee setup
	Xbee.begin(BAUD);
	//SD setup
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
	//camera setup
	
	
	
}

//----------------------------------------------------
// the loop routine runs over and over again forever:
//----------------------------------------------------

void loop() {        
  
}



//----------------------------------------------------
// functions
//----------------------------------------------------

void parseSend(){
	if(Bridge.available()){
		char buffer[100];
		int commas[15]={0};
		int j = 0;
		Bridge.readBytesUntil('\n',buffer,100);
		for(int i=0;i<100;i++){
			if(buffer[i] == ','){
				commas[j] = i;
				j++;
			}
		}
		timecheck = millis();
		timeSync = strtoul(buffer[commas[11]+1],buffer[commas[12]],10);
				
		char launchedChar = buffer[commas[12]+1];
		if(launchedChar == '0'){
			launched = false;
		}
		else{
			launched = true;
		}
		char launchedChar = buffer[commas[12]+2];
		if(launchedChar == '0'){
			released = false;
		}
		else{
			released = true;
		}
		char launchedChar = buffer[commas[12]+3];
		if(launchedChar == '0'){
			reachAlt = false;
		}
		else{
			reachAlt = true;
		}
		char launchedChar = buffer[commas[12]+4];
		if(launchedChar == '0'){
			GPSlock = false;
		}
		else{
			GPSlock = true;
		}
		saveParams();
	}
}

unsigned long time(){
	return millis()-timeCheck+timeSync;
}


void checkCmd(){
	if(Xbee.available()){
		char cmd = Xbee.read();
		if(cmd == TAKE_PIC){
			snapshot();
			camCmdTime = time();
		}
		if(cmd == RELEASE){
			Bridge.println(RELEASE)
		}
		//add additional cmd checks here if needed.
	}
}

void snapshot(){
	//camera snapshot and send code here
}

void saveParams(){
	if(SD.exists('param'){
		SD.remove('param');
	}
	File params = SD.open('param',FILE_WRITE);
	if(params){
			params.print(launched);
			params.print(released);
			params.print(reachAlt);
			params.println(GPSlock);
			params.close();
		}
	else{
		Serial.println("ERROR file could not be opened!");
	}
}

