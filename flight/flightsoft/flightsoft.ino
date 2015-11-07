#include <Wire.h>
#include <LSM303.h>
#include <LPS.h>
#include <L3G.h>
#include <SoftwareSerial.h>

#include <math.h>

#define BAUD 9600
#define TIME_TOL  10.  //time tolerance in percent
#define SEND_PER  1000  //send period in milliseconds

int last_send = 0;
int this_send = 0;
int pos = 0;

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

Packet_t data[10];
Packet_t avg[10];

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
  pos++;
  if(pos > 9)  {
    pos = 0;
  }
  //send data
  this_send = millis() % 1000;
//  Serial.println(this_send);
  if (this_send < TIME_TOL*SEND_PER/100. || this_send > (100-TIME_TOL)*SEND_PER/100)  {
    //send data
    Serial.println("==========DATA==============");
    Serial.print(data[pos].time);
    Serial.print(",\t");
    Serial.print(data[pos].gyro_x);
    Serial.print(",\t");
    Serial.print(data[pos].gyro_y);
    Serial.print(",\t");
    Serial.print(data[pos].gyro_z);
    Serial.println();
  }
    
  delay(1);        // delay in between reads for stability
}
