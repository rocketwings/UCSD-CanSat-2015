#ifndef flightsoft_h
#define flightsoft_h

typedef struct Packet {
  unsigned long time;
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
  float latitude;
  float longitude;
  float gpsalt;
  float satnum;
  float gpsspeed;
  unsigned long imgcmdTime;
  int imgcmdCount;
  int bonus;
} Packet_t;
#endif
