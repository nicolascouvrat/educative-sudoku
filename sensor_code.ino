// Arduinoのピン定義 http://garretlab.web.fc2.com/arduino/lab/digital_color_sensor/index.html
//ピンの配置　http://kousaku-kousaku.blogspot.jp/2008/11/arduinos9706.html
//繋げ方　https://fabble.cc/arduinotutorial/colorsensor

/*
 * Code version 2017/1/23
 * This is the sensor + color decision code 
 * GENERAL CODE
 */
#include <Wire.h>
const int doutPin = 12;   // Dout
const int rangePin = 9;  // Range
const int ckPin = 10;     // CK
const int gatePin =11;   // Gate
int pinRed=3; //pin values for LED coloring
int pinBlue=5; //TODO: Change these pin values
int pinGreen=4;
int address=14; // TO CHANGE DEPENDING ON POSITION

int addressHigh=0; //updated according to address at setup
int addressLow=0;

const int trigPin = A0;
const int echoPin = A1;

int currentColor=0; //COLOR CODE: 1=pink, 2=green, 3=blue 4=grey 0=NONE , determined by sensor OR set up by master
bool initialSetUp=false; // if the color has been set up by the master, we prevent it from changing later


void setup() {
  addressHigh=(address/10);
  addressLow=(address%10);
  Wire.begin(address);
  Serial.begin(9600);
  Serial.println(addressLow);
  Serial.println(addressHigh);
  // ピンモードを設定する。doutPinは入力、それ以外は出力。
  pinMode(doutPin, INPUT);
  pinMode(rangePin, OUTPUT);
  pinMode(ckPin, OUTPUT);
  pinMode(gatePin, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(pinRed,OUTPUT);
  pinMode(pinBlue,OUTPUT);
  pinMode(pinGreen,OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestedEvent);
}

void loop() {
  if(!initialSetUp){ //if it has been set up by master then when don't bother running through the color decision code
    if(distanceCheck()){
        digitalWrite(13, HIGH);   // set the LED on
        // put your main code here, to run repeatedly: 
        int red, green, blue;  // 測定した値を格納するための変数
        int integration = 50;  // 測定時間(ミリ秒)を格納する変数
        char s[64];            // シリアルコンソールに出力する文字列を保持する変数
        
        digitalWrite(gatePin, LOW);        // GateとCKをLOWにする。
        digitalWrite(ckPin, LOW);
        digitalWrite(rangePin, HIGH);      // RangeをHIGHにする。
        
        digitalWrite(gatePin, HIGH);       // GateをHIGHにして測定開始。
        delay(integration);                // 測定時間だけ待つ。
        digitalWrite(gatePin, LOW);        // GateをLOWにして測定終了。
        delayMicroseconds(4);              // 4ミリ秒待つ。
        red = shiftIn12(doutPin, ckPin);   // 赤色の値を読む。
        delayMicroseconds(3);              // 3ミリ秒待つ。
        green = shiftIn12(doutPin, ckPin); // 緑色の値を読む。
        delayMicroseconds(3);              // 3ミリ秒待つ。
        blue = shiftIn12(doutPin, ckPin);  // 青色の値を読む。
        
        sprintf(s, "%4d, %4d, %4d", red, green, blue);
        Serial.println(s);
        if(6.700+-210.400*red+32.700*green+38.400*blue>0){
          Serial.println("blue");
          currentColor=3;
        }
        else if(0.000+20.300*red+-74.700*green+26.800*blue>0){
          Serial.println("pink");
          currentColor=2;
        }
        else if(8.900+-1.000*red+62.000*green+-88.100*blue>0){
          Serial.println("green");
          currentColor=1;
        }
        else if(12.200+40.100*red+-8.100*green+-35.800*blue>0){
          Serial.println("grey");
          currentColor=4;
        }
        else{
          Serial.println("etc...");
          currentColor=0;
        }  
    }else{
      currentColor=0;
    }    
  }
  else{
    if(distanceCheck()) setUpColor(0);
    else setUpColor(currentColor);
  }
  delay(500);
}

//12ビットの値を読み込む関数(LSBから送信されるデータを想定)
int shiftIn12(int dataPin, int clockPin) {
  int value = 0;
  
  for (int i = 0; i < 12; i++) {
    digitalWrite(clockPin, HIGH);           // クロックをHIGHにする
    value |= digitalRead(dataPin) << i;     // データピンの値を読み取り、所定のビットを設定する。
    digitalWrite(clockPin, LOW);            // クロックピンをLOWにする。
  }
 
  return value;
}

/*
 * FROM HERE, NON COLOR-SENSOR RELATED CODE
 */
void setUpColor(int receivedValue){
  /*
   * COLOR CODE: 1=green, 2=pink, 3=blue 4=brown/orange 0=NONE
   */
  switch(receivedValue){
    case 2: 
    setColor(200, 100, 40);  // pink
    break;
    case 1: 
    setColor(0, 255, 0);  // green
    break;
    case 3:
    setColor(0, 0, 255);  // blue
    break;
    case 4:
    setColor(255, 255, 255);  // orange (lqst color)
    break;
    default:
    setColor(0,0,0); //clear
    break;
  }
}

void receiveEvent(int bytes){
  initialSetUp=false;
  currentColor=0;
  //receive color info for initial setup
  currentColor=Wire.read();
  //Serial.println(currentColor); //to test received value
  setUpColor(currentColor); //turn on the led according to the received value
  if(currentColor!=0) initialSetUp=true; //lock the color because it has been decided by master IF ITS NOT NILL
}

void requestedEvent(){
  /*
   * REQUESTED DATA FORMAT : 3 characters in [color,adress] where address takes 2 characters from 01 to 16 
   */
  char Buf[3];
  Buf[0]='0'+currentColor; //risk of bug here?
  Buf[1]='0'+addressHigh;
  Buf[2]='0'+addressLow;
  Wire.write(Buf); //respond with msg
}

void setColor(int red, int green, int blue) {
  analogWrite(pinRed, red);
  analogWrite(pinGreen, green);
  analogWrite(pinBlue, blue);  
}

bool distanceCheck(){
    // establish variables for duration of the ping, 
  // and the distance result in inches and centimeters:
  long duration, inches, cm;

  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  
  duration = pulseIn(echoPin, HIGH);

  // convert the time into a distance
  cm = microsecondsToCentimeters(duration);
  Serial.println(cm);
  if(cm<=4) return true;
  else digitalWrite(13,LOW);
  return false;
  //return true; //FOR TEST PURPOSE, REMOVE THIS LINE
}

long microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}

