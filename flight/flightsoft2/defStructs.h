#ifndef flightsoft_h
#define flightsoft_h

typedef struct Packet {
  //unsigned long time;
  float pressure;
  float altitude;
  float temp;
  float voltage;
  float airspeed;
  //float latitude;
  //float longitude;
  //float gpsalt;
  //float satnum;
  //float gpsspeed;
  unsigned long  time;
//  int imgcmdCount;
  int bonus;
} Packet_t;

typedef struct Avg {
  //unsigned long time;
  float pressure;
  float altitude;
  float temp;
  float airspeed;
} Avg_t;

#endif
