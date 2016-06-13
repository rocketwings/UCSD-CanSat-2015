/*
 * Goes on RED pro-micro
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

#define GPSECHO true
#define BAUD 9600
#define TIME_TOL  1  //time tolerance in percent
#define SEND_PER  1000  //send period in milliseconds
#define DATA_LENGTH 10  //length of data array
#define AVG_LENGTH 10  //length of avg array

#define PITOT_PIN 20//analog pin for pitot tube
#define PITOT_CAL 32  //calibration for the zero point of the differential pressure

#define RELEASE_ALTITUDE 400 // altitude set for release of cansat for container in meters
#define RELEASE_TIMEOUT 5000 //timeout for release led after it starts
#define TEAM_ID "3640" // Team ID REMEMBER TO CHANGE THIS TOO LAZY TO LOOK UP

#define LAUNCH_VELOCITY 10
#define ACCEL_THRESH 18
#define LAUNCHED_ALT_THRESHOLD 20

#define LED_PIN 5
#define BUZZER_PIN 6
#define VOLTAGE_PIN 18

#define HOUR_OFFSET -5

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
SoftwareSerial Bridge(9, 8); //Rx, Tx this will be the serial bridge between the two microcontrollers

Adafruit_GPS GPS(&Serial1);
HardwareSerial mySerial = Serial1;
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
  Serial.begin(57600);
  //while (!Serial)
  // wait for serial port to connect. Needed for Leonardo only

  //Ini Bridge
  Bridge.begin(57600);
  Serial.println("Bridge initialized");
  //GPS

  GPS.begin(9600);
  delay(1);
  Serial.println("GPS initialized");
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  delay(1);
  Serial.println("GPS initialized");
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  delay(1);
  Serial.println("GPS initialized");

  delay(1);
  Serial.println("GPS initialized");
  //-------------
  delay(1);
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

  // State initialization
  getBridge();
  Serial.println("State Initialized.");

  useInterrupt(true);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,OUTPUT);
  digitalWrite(BUZZER_PIN,LOW);
  //delay(100);
  digitalWrite(BUZZER_PIN,HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN,LOW);
  delay(100);
  digitalWrite(BUZZER_PIN,HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN,LOW);
  //while(1);
  delay(1000);
}


void loop() {
  getData(pos);
  //releaseSat();
  //Serial.println(altSim());
  freqLimiter(pos);
  static bool filled = false;

  if (pos == DATA_LENGTH) {
    filled = true;
    pos = 0;
  }
  if (filled == true) {
    //Serial.println("LIN IS AN ASS");
    avgGenerator(pos);
    static bool ran = false;
    
    launchIndicatorAlt(pos);
    stateTrigger(pos);
    //launchIndicator();//try adding accelerometer data as well for detection
    //Serial.print(avg[pos].airspeed);
    //Serial.print(",");
    //Serial.println(data[pos].airspeed);
  }
  //Serial.println(pos);
  //delay(500);
  pos++;

}

void freqLimiter(int pos) {
  this_send = millis();
  if (this_send - last_send > .5 * SEND_PER)  {
    if (this_send % 1000 < TIME_TOL * SEND_PER / 100. || this_send % 1000 > (100 - TIME_TOL)*SEND_PER / 100)  {
      last_send = this_send;
      packet_count++;

      //logData(pos);
      //xbeeSend(pos);
      bridgeSend(pos);
      //serialMonitor(pos);

    }
  }
  else  {
    delay(1);        // delay in between reads for stability
  }
}

void getData(int pos) {
  // read data

  // pololu
  data[pos].pressure = ps.readPressureMillibars();
  static float baseAlt = ps.pressureToAltitudeMeters(data[pos].pressure);
  data[pos].altitude = ps.pressureToAltitudeMeters(data[pos].pressure) - baseAlt;
  //data[pos].altitude = altSim();
  data[pos].temp = ps.readTemperatureF();

  //  compass.read();
  //  data[pos].compass_ax = compass.a.x;
  //  data[pos].compass_ay = compass.a.y;
  //  data[pos].compass_az = compass.a.z;

  //pitot
  pitotRead = analogRead(PITOT_PIN);
  data[pos].airspeed = sqrt(2000.*((((float)pitotRead - (float)PITOT_CAL) / (0.2 * 1024.0)) - 2.5) / 1.225);
  if(data[pos].airspeed != data[pos].airspeed){
    data[pos].airspeed = 0.0;
    //Serial.println("NAN");
  }
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

  
  getTime();
  
  //--------------------------------------------------------------
  //Voltage
  data[pos].voltage = voltage();
  
  //camera and params (params if requested.)
  getBridge();
}

void bridgeSend(int pos) {
  //Bridge.flush();
  Bridge.print(TEAM_ID); //team ID
  Bridge.print(",");
  Bridge.print(data[pos].time);// time
  Bridge.print(",");
  Bridge.print(packet_count);
  Bridge.print(",");
  Bridge.print(avg[pos].altitude, 2);
  Bridge.print(",");
  Bridge.print(avg[pos].pressure, 2);
  Bridge.print(",");
  Bridge.print(data[pos].airspeed, 2);
  Bridge.print(",");
  Bridge.print(avg[pos].temp, 2);
  Bridge.print(",");
  Bridge.print(data[pos].voltage, 3);
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
  Bridge.print(imgCmdTime);
  Bridge.print(",");
  Bridge.print(imgCmdCount);
  Bridge.print(",");
  // State Params
  Bridge.print(launched);// time of last imaging command
  Bridge.print(released);// number of imaging commands
  Bridge.print(reachAlt);
  Bridge.print(GPSlock);
  Bridge.print("\n");

  Serial.print(TEAM_ID); //team ID
  Serial.print(",");
  Serial.print(data[pos].time);// time
  Serial.print(",");
  Serial.print(packet_count);
  Serial.print(",");
  Serial.print(avg[pos].altitude, 2);
  Serial.print(",");
  Serial.print(avg[pos].pressure, 2);
  Serial.print(",");
  Serial.print(data[pos].airspeed, 2);
  Serial.print(",");
  Serial.print(data[pos].temp, 2);
  Serial.print(",");
  Serial.print(data[pos].voltage, 2);
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
  Serial.print(imgCmdTime);// time of last imaging command
  Serial.print(",");
  Serial.print(imgCmdCount);// number of imaging commands
  Serial.print(",");

  // State Params
  Serial.print(launched);
  Serial.print(released);
  Serial.print(reachAlt);
  Serial.print(GPSlock);
  Serial.print("\n");

  Serial.println(GPS.hour);
  Serial.println(GPS.minute);
  Serial.println(GPS.seconds);
  Serial.println(GPS.milliseconds);
  Serial.print((int)round(data[pos].time/(1000*60*60))%24);
  Serial.print(",");
  Serial.print((int)round(data[pos].time/(1000*60))%60);
  Serial.print(",");
  Serial.println((int)round(data[pos].time/(1000))%60);
}

void avgGenerator(int pos) {
  avg[pos] = {0};

  for (int i = 0; i < DATA_LENGTH ; i++) {
    avg[pos].pressure = avg[pos].pressure + data[i].pressure;
    avg[pos].temp = avg[pos].temp + data[i].temp;
    avg[pos].altitude = avg[pos].altitude + data[i].altitude;
    avg[pos].airspeed = avg[pos].airspeed + data[i].airspeed;
  }
  avg[pos].pressure /=  DATA_LENGTH;
  avg[pos].temp /= DATA_LENGTH;
  avg[pos].altitude /= DATA_LENGTH;
  avg[pos].airspeed /= DATA_LENGTH;
}

void stateTrigger(int pos) {
  if (avg[pos].altitude >= RELEASE_ALTITUDE + 15) {
    reachAlt = true;
  }
  if (launched && reachAlt) {
    if (avg[pos].altitude <= RELEASE_ALTITUDE + 10) {
      static unsigned long releaseTime = millis(); 
      static bool ran = false;
      if(!ran){
        releaseSat();
        ran = true;
      }
      if(millis() - releaseTime >= RELEASE_TIMEOUT){
        digitalWrite(LED_PIN,LOW);
      }
    }
  }
  if(released && (avg[pos].altitude < LAUNCHED_ALT_THRESHOLD)){
    buzzer();
  }
}

float launchIndicator() {
  float slope = 0.0;
  float sumAlt = 0.0;
  float sumAlt2 = 0.0;
  float sumTime = 0.0;
  float sumTtime2 = 0.0;
  float crossSum = 0.0;

  for (int n = 0; n < AVG_LENGTH; n++) {
    sumAlt += avg[n].altitude;
    sumAlt2 += avg[n].altitude * avg[n].altitude;
    sumTime += float((data[n].time / 1000));
    sumTtime2 += float((data[n].time / 1000) * (data[n].time / 1000));
    crossSum += float(data[n].time / 1000) * avg[n].altitude;
  }

  slope = (AVG_LENGTH * crossSum - sumAlt * sumTime) / (AVG_LENGTH * sumTtime2 - sumTime * sumTime);

  if (slope >= LAUNCH_VELOCITY || avg[pos].altitude >= LAUNCHED_ALT_THRESHOLD) {
    launched = true;
  }
  //Serial.println(slope);
  return slope;
}
/*
float launchIndicatorAccel() {
  compass.read();
  float mag = sqrt(pow(compass.a.x, 2) + pow(compass.a.y, 2) + pow(compass.a.z, 2));
  //static int hold = millis();
  if (mag > ACCEL_THRESH) {
    launched = true;
  }
  Serial.print(compass.a.x);
  Serial.print(",");
  Serial.print(compass.a.y);
  Serial.print(",");
  Serial.print(compass.a.z);
  Serial.print(",");
  Serial.println(mag);
  return mag;
}
*/

void launchIndicatorAlt(int pos){
  if(data[pos].altitude > LAUNCHED_ALT_THRESHOLD){
    launched = true;
  }
}

void getBridge() {
  // will set state variables and get necessary data from other arduino. (SD card to serial bridge)
  Bridge.listen();
  while (Bridge.available()) {
    char buff[25] = {'\0'};
    Bridge.readBytesUntil('\n', buff, 25);
    if (buff[0] == 'p') {
      //Bridge.print('p');
      Serial.println("State Recieved.");
      Serial.println(buff);
      if (buff[1] == '1') {
        launched = true;
      }
      else launched = false;
      if (buff[2] == '1') {
        released = true;
      }
      else released = false;
      Serial.println(released);
      Serial.println(buff[2]);
      if (buff[3] == '1') {
        reachAlt = true;
      }
      else reachAlt = false;
      if (buff[4] == '1') {
        GPSlock = true;
      }
      else GPSlock = false;
      printParams();
    }
    if (buff[0] == 'c') {
      
      //Bridge.print('c');
      Serial.println("Cam params Recieved.");
      Serial.println(buff);
      int comma = 0;
      for (int i = 0; i < 25; i++) {
        if (buff[i] == ',') {
          comma = i;
        }
      }
      //Serial.println(comma);
      imgCmdCount = atoi(buff + 1);
      imgCmdTime = strtoul(buff+comma+1, NULL, 10);
    }
    if (buff[0] == 'r') {
      //For command through Xbee
      Serial.println("Releasing...");
      releaseSat();
    }
    if(buff[0] == 'l'){
      digitalWrite(LED_PIN,LOW);
    }
    if(buff[0] == 'b'){
      digitalWrite(BUZZER_PIN,LOW);
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

void releaseSat() {
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 127);
  released = true;
  Serial.println("RELEASING!");
}

void buzzer() {
  //commented for now because annoying.
  
  static bool ran = false;
  if(!ran){
    Serial.println("BUZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ!!!!!!!!!");
    ran = true;
    digitalWrite(BUZZER_PIN, HIGH);
  }
}

void getTime(){
  //Serial.println("Getting Time");
  //static unsigned long tempNum = 32400000;
  //static unsigned long tempTimes = ((unsigned long)9)*60*60*1000;
  static bool timeFix = false;
  if(GPS.satellites >=4 || timeFix){
    static unsigned long GPStime = ((unsigned long)(GPS.hour+HOUR_OFFSET)%24) * ((unsigned long)1000 * 60 * 60) 
      + (unsigned long)GPS.minute * ((unsigned long)1000 * 60) 
      + (unsigned long)GPS.seconds * ((unsigned long)1000) 
      + (unsigned long)GPS.milliseconds;
    data[pos].time = millis()+ GPStime;
    timeFix = 1;
    //Serial.println(tempNum);
    //Serial.println(tempTime
  }
  else{
    data[pos].time = millis();
  }
}

float voltage(){
  //0-614.4 assuming dividing to half 6V * Ratio -> ADC
  //R1 = 10k, R2 = 1K + 1K
  float ratio = (float)5/6;
  float raw = (float)analogRead(VOLTAGE_PIN);
  float volt = raw * 0.0048828125 * (1/ratio);
  //Serial.print("Raw: ");
  //Serial.print(raw);
  //Serial.print(" Volts: ");
  //Serial.println(volt);
  return volt;
}

float altSim(){
  static unsigned long timePrevious = millis();
  static float alt = 0;
  static bool hit = false;
  if((millis() - timePrevious) >= 10){
    timePrevious = millis();
    if(alt<610 && hit==false){
      alt+=.8;
    }
    
    else if(alt>=610 || hit){
      hit = true;
      alt-=.1;
      
      if(alt<0){
        alt=0;
      }
    }
  }
  return alt;
}

void printParams(){
  Serial.print(launched);
  Serial.print(released);
  Serial.print(reachAlt);
  Serial.println(GPSlock);
}

