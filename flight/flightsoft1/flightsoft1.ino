/*
 * Goes on the BLUE pro-micro
 *
 */


//#include <Wire.h>
#include <SoftwareSerial.h>
//#include <math.h>
#include <SPI.h> //needed for SD card reader
#include <SD.h> // SD library
#include <Adafruit_VC0706.h>

//#include "defStructs.h"

#define BAUD 9600
#define DATA_LENGTH 20  //length of data array
#define AVG_LENGTH 20  //length of avg array
#define RESETMICRO 6 //pin to reset promicro (RED)

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
unsigned long int timeSync = 0;
unsigned long int timeCheck = 0;

SoftwareSerial Bridge(11,9); //Rx, Tx this will be the serial bridge between the two microcontrollers
SoftwareSerial Xbee(8,4); // Rx,Tx subject to change.

//cam
Adafruit_VC0706 cam = Adafruit_VC0706(&Serial1);



//------------------------------------------------
// setup runs once
//------------------------------------------------

void setup() {
  pinMode(RESETMICRO,OUTPUT);
  digitalWrite(RESETMICRO, HIGH);
  delay(2000);
	// Serial for debug
	Serial.begin(BAUD);
  Serial1.begin(115200);
  //while (!Serial) {
    // wait for serial port to connect. Needed for Leonardo only
  //}
	//bridge setup
	Bridge.begin(BAUD);
	//xbee setup
	Xbee.begin(BAUD);
  Serial.println("...Xbee initialized!");
	//SD setup
	Serial.print("SD card setup...");
  pinMode(CHIP_SELECT, OUTPUT); // set CP as output
  //digitalWrite(CHIP_SELECT, HIGH);
  while(!SD.begin(CHIP_SELECT))
  {
    Serial.println("Card Initialization Failure!");
    
  }
  
  Serial.println("Card Ready!");
  
	//camera setup
	Serial.println("VC0706 Camera snapshot Initialization...");
  delay(1000);
  if (cam.begin()) Serial.println("Camera Found!");
  else {
    Serial.println("Camera not found");
    while(1);
  }
  Serial.println("Cam");
  cam.setImageSize(VC0706_640x480);
  
  //-----------
  
  //digitalWrite(13,HIGH);
  //delay(250);
  //digitalWrite(13,LOW);
  //delay(250);
  //digitalWrite(13,HIGH);
  //delay(250);
  //digitalWrite(13,LOW);

  Bridge.flush();
  Serial.println("...Setup Complete!");

  delay(1000);
  digitalWrite(RESETMICRO, LOW);
  delay(1);
  digitalWrite(RESETMICRO, HIGH);



    
}

//----------------------------------------------------
// the loop routine runs over and over again forever:
//----------------------------------------------------

void loop() { 
  //Serial.println("sadfasdf");       
  //parseSend();
  checkCmd();
  //delay(5000);
  //Serial.println("Attempting Taking Pic");
  //snapshot();
  //while(1);
  delay(10);
}



//----------------------------------------------------
// functions
//----------------------------------------------------

void parseSend(){
  //Parses Bridge and sends it out to Xbee
  Bridge.listen();
	if(Bridge.available()){
    Serial.println("Lin is a hero");
		char buff[100]={'\0'};
		int commas[15]={0};
		int j = 0;
		Bridge.readBytesUntil('\n',buff,100);
   
    Xbee.println(buff);
    
    Bridge.flush();
		for(int i=0;i<100;i++){
			if(buff[i] == ','){
				commas[j] = i;
				j++;
			}
      if(buff[0] == 'p'){
        getSetSendParams();
        break;
      }
		}
    
		timeCheck = millis();
		timeSync = strtoul(buff+commas[11]+1,NULL,10);
				
		char launchedChar = buff[commas[12]+1];
		if(launchedChar == '0'){
			launched = false;
		}
		else{
			launched = true;
		}
		char releasedChar = buff[commas[12]+2];
		if(releasedChar == '0'){
			released = false;
		}
		else{
			released = true;
		}
		char reachedChar = buff[commas[12]+3];
		if(reachedChar == '0'){
			reachAlt = false;
		}
		else{
			reachAlt = true;
		}
		char GPSlockChar = buff[commas[12]+4];
		if(GPSlockChar == '0'){
			GPSlock = false;
		}
		else{
			GPSlock = true;
		}
		saveParams();
    // send telemetry over xbee
	}
}

unsigned long time(){
	return millis()-timeCheck+timeSync;
}


void checkCmd(){
  Xbee.listen();
	if(Xbee.available()){
		char cmd = Xbee.read();
    Serial.println(cmd);
   
		if(cmd == TAKE_PIC){
      Serial.println("IMAGE CMD");
			snapshot();
			
      camCmdCount ++;
      Bridge.print("c");
      Bridge.print(camCmdCount);
      Bridge.print(",");
      Bridge.println(time());
		}
		
		if(cmd == RELEASE){
      Serial.println("RELEASE CMD");
			Bridge.println(RELEASE);
      releaseSat();
		}
		//add additional cmd checks here if needed.
	}
}

void snapshot(){
	//camera snapshot and send code here
  cam.setImageSize(VC0706_640x480);
  if (! cam.takePicture()) 
    Serial.println("Failed to snap!");
  else 
    Serial.println("Picture taken!");
  uint16_t len = LogPic();
  char fileName[] = "IMAGE00.JPG";
  sendPic(fileName, len);
  
}

uint16_t LogPic () {
    // Create an image with the name IMAGExx.JPG
  char filename[13];
  strcpy(filename, "IMAGE00.JPG");
  for (int i = 0; i < 100; i++) {
    filename[5] = '0' + i/10;
    filename[6] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }
  
  // Open the file for writing
  File imgFile = SD.open(filename, FILE_WRITE);

  // Get the size of the image (frame) taken  
  uint16_t jpglen = cam.frameLength();
  uint16_t len = jpglen;
  Serial.print("Storing ");
  Serial.print(jpglen, DEC);
  Serial.print(" byte image.");

  int32_t time = millis();
  //pinMode(8, OUTPUT);
  // Read all the data up to # bytes!
  byte wCount = 0; // For counting # of writes
  while (jpglen > 0) {
    // read 32 bytes at a time;
    uint8_t *buffer;
    uint8_t bytesToRead = min(64, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
    buffer = cam.readPicture(bytesToRead);
    imgFile.write(buffer, bytesToRead);
    
    if(++wCount >= 64) { // Every 2K, give a little feedback so it doesn't appear locked up
      Serial.print('.');
      wCount = 0;
    }
    //Serial.print("Read ");  Serial.print(bytesToRead, DEC); Serial.println(" bytes");
    jpglen -= bytesToRead;
  }
  imgFile.close();

  time = millis() - time;
  Serial.println("done!");
  Serial.print(time); Serial.println(" ms elapsed");
  return len;
}

int sendPic(char *fileName,uint16_t jpglen) {
  File readFile;
  if(SD.exists(fileName)) {
    readFile = SD.open(fileName, FILE_READ);
  }
  else  {
    Serial.print("That File doesnt exist!");
    return(1); //can't find file, quit now;
  }
  Serial.println("sending picture");
  Serial.println(jpglen,DEC);
  Xbee.println("sending picture");
  Xbee.println(jpglen,DEC);
  delay(1000);
  byte buffer[256];
  while(readFile.available())  {
    for(int i=0; i<256; i++) {
      buffer[i] = readFile.read();
    }
    Xbee.write(buffer,256);
    Serial.print(".");
  }
  Serial.print("\nDone.\n");
  return(0);
}

void saveParams(){
	if(SD.exists("param")){
		SD.remove("param");
	}
	File params = SD.open("param",FILE_WRITE);
	if(params){
			params.print(launched);
			params.print(released);
			params.print(reachAlt);
			params.print(GPSlock);
      params.print(",");
      params.print(camCmdCount);
      params.print(",");
      //------------------------asdfasdfasdf--------------
			params.close();
		}
	else{
		Serial.println("ERROR file could not be opened!");
	}
}

void getSetSendParams(){
  if(SD.exists("param")){
    File params = SD.open("param",FILE_READ);
    if(params){
      char buff[5] = {'\0'};
      params.readBytesUntil('\n',buff,5);
      params.close();
      if(buff[0]=='1'){
        launched = true;          
      }
      else launched = false;
      if(buff[1]=='1'){
        released = true;          
      }
      else released = false;
      if(buff[2]=='1'){
        reachAlt = true;          
      }
      else reachAlt = false;
      if(buff[3]=='1'){
        GPSlock = true;          
      }
      else GPSlock = false;
      Bridge.print("p");
      Bridge.print(launched);
      Bridge.print(released);
      Bridge.print(reachAlt);
      Bridge.println(GPSlock);     
    }
  }
} 
  
void releaseSat(){
//put release code here
  
}

