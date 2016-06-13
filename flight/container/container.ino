#include <Servo.h>

Servo servo;
float val;
float buff[100]={0};
void setup() {
  pinMode(0,INPUT);
  Serial.begin(57600);
  servo.attach(6);
}

void loop() {
  float sum = 0;
  for(int i=0;i<100;i++){
    buff[i] = analogRead(0)*0.0048828125;
    sum += buff[i];
  }
  val = sum/100;

  if(val>1.6){
    servo.write(13);
  }
  else servo.write(175);
  // put your main code here, to run repeatedly:
  Serial.println(val);

}
