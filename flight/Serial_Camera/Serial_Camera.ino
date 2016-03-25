#include <Adafruit_VC0706.h>
#include <SPI.h>
#include <SD.h> // SD library
#define CHIPSELECT 10
//SoftwareSerial Camera(7,5); // RX,TX
Adafruit_VC0706 cam = Adafruit_VC0706(&Serial1);
void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  if(!SD.begin(CHIPSELECT)){
    Serial.println("Card failed, or not present");
  }
  Serial.println("VC0706 Camera snapshot test");
  delay(5000);
  //locating the camera
  {
  if (cam.begin()) Serial.println("Camera Found!");
  else {
    Serial.println("Camera not found");
    return;
  }}
  
  cam.setImageSize(VC0706_640x480);   //setting picture size: large

  //Taking picture
  {
  Serial.println("Snap in 3 secs...");
  delay(3000);

  if (! cam.takePicture()) 
    Serial.println("Failed to snap!");
  else 
    Serial.println("Picture taken!");
  }
 LogPic();
} 



void loop() {
  // put your main code here, to run repeatedly:
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
  Serial.print("Storing ");
  Serial.print(jpglen, DEC);
  Serial.print(" byte image.");

  int32_t time = millis();
  pinMode(8, OUTPUT);
  // Read all the data up to # bytes!
  byte wCount = 0; // For counting # of writes
  while (jpglen > 0) {
    // read 32 bytes at a time;
    uint8_t *buffer;
    uint8_t bytesToRead = min(32, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
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
  return jpglen;
}

