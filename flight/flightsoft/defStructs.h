#ifndef flightsoft_h
#define flightsoft_h

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
#endif
