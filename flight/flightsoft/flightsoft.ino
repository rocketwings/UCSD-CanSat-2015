#include <Wire.h>
#include <LSM303.h>
#include <LPS.h>
#include <L3G.h>
#include <SoftwareSerial.h>

#include <math.h>

#define BAUD 9600
#define TIME_TOL  1.  //time tolerance in percent
#define SEND_PER  100  //send period in milliseconds
#define DATA_LENGTH 10  //length of data array
#define AVG_LENGTH  10  //length of avg array
#define PITOT_PIN 0 //analog pin for pitot tube
#define PITOT_CAL 19  //calibration for the zero point of the differential pressure

unsigned long last_send = 0;
unsigned long this_send = 0;
int pos = 0;
int packet_count = 0;
int pitotRead = 0;

L3G gyro;
LPS ps;
LSM303 compass;
SoftwareSerial Xbee(8,15);  // RX, TX

typedef struct Packet {
  long time;
  float pressure;
  float altitude;
  float temp;
  float voltage;
  float airspeed;
  float gyro_x;
  float gyro_y;
  float gyro_z;
  float compass_ax;
  float compass_ay;
  float compass_az;
  float compass_mx;
  float compass_my;
  float compass_mz;
} Packet_t;

Packet_t data[DATA_LENGTH];
Packet_t avg[AVG_LENGTH];

void setup() {
  // initialize serial communication at BAUD bits per second:
  Serial.begin(BAUD);
  Wire.begin();
  Serial.println("lkdsfoijewaflkjfds");

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

}

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  // int sensorValue = analogRead(A0);
  // print out the value you read:
  // Serial.println(sensorValue);

  // read data
  data[pos].time = millis();
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
  pitotRead = analogRead(PITOT_PIN);
//  Serial.println(pitotRead);
  data[pos].airspeed = sqrt(2000.*(((pitotRead-PITOT_CAL)/(0.2*1024.0))-2.5)/1.225);

  pos++;
  if(pos >= DATA_LENGTH)  {
    pos = 0;
  }
  //send data
  this_send = millis();
  if (this_send - last_send > .5*SEND_PER)  {
    if (this_send%1000 < TIME_TOL*SEND_PER/100. || this_send%1000 > (100-TIME_TOL)*SEND_PER/100)  {
      //send data
      last_send = this_send;
      packet_count++;
      Serial.print("12345");
      Serial.print(",");
      Serial.print(packet_count);
      Serial.print(",");
      Serial.print(data[pos].altitude);
      Serial.print(",");
      Serial.print(data[pos].pressure);
      Serial.print(",");
      Serial.print(data[pos].airspeed);
      Serial.print(",");
      Serial.print(data[pos].temp);
      Serial.print(",");
      Serial.print("5.6");
      Serial.print(",");
      Serial.print("89");
      Serial.print(",");
      Serial.print("98");
      Serial.print(",");
      Serial.print("21");
      Serial.print(",");
      Serial.print("5");
      Serial.print(",");
      Serial.print("6");
      Serial.print(",");
      Serial.print("2");
      Serial.print(",");
      Serial.print(data[pos].time);
      Serial.print(",");
      Serial.println();
    }
  }
  else  {
    delay(1);        // delay in between reads for stability
  }
}
