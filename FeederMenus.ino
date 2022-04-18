//BASE CODE FOR READING ROTARY ENCODER HERE: https://www.semesin.com/project/2018/10/10/menu-arduino-dengan-rotary-encoder/
#define encpinCLK     3 //Encoder CLK
#define encpinDT      2 //Encoder DT
#define encpinSW      4 //Encoder SW
 
#include <LiquidCrystal.h>
const int rs = 5, en = 6, d4 = A2, d5 = A3, d6 = A4, d7 = A5; //RESET->D5, ENABLE->D6, D4->A2, D5->A3, D6->A4, D7->A5
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
 
uint8_t maskSensorA;
uint8_t maskSensorB;
uint8_t *pinSensorA;
uint8_t *pinSensorB;
volatile bool encoderAFlag = 0;
volatile bool encoderBFlag = 0;
int prevDir = 0;
int curSetting = 0;
 
int8_t encDir = 0;
int nilaiSetting[4];
byte setMode;
String Settings[] = {"Feed Time:", "Feed Amnt:", "Feed Now:"};
String Settingsvals[] = {"5", "25", "0"};
String SettingsSuffix[] = {" min", " g", " g"};
bool changingVal = false;
int curVal = 0;

template<typename T, size_t N> size_t ArraySize(T(&)[N]){ return N; }
 
void setup() {
  Serial.begin(9600);
 
  pinMode(encpinCLK, INPUT_PULLUP);
  pinMode(encpinDT, INPUT_PULLUP);
  pinMode(encpinSW, INPUT_PULLUP);
 
  lcd.begin(16, 2);
 
  attachInterrupt(digitalPinToInterrupt(encpinCLK), encoderARising, RISING); //Map rising edge to handler functions
  attachInterrupt(digitalPinToInterrupt(encpinDT), encoderBRising, RISING);
 
  maskSensorA  = digitalPinToBitMask(encpinCLK);
  pinSensorA = portInputRegister(digitalPinToPort(encpinCLK));
  maskSensorB  = digitalPinToBitMask(encpinDT);
  pinSensorB = portInputRegister(digitalPinToPort(encpinDT));
 
  lcd.clear();
  displaySetting(0);
}

//function for printing setting and values on second row of display
void displaySetting(int index){
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(Settings[curSetting]);
    lcd.print(Settingsvals[curSetting]);
    lcd.print(SettingsSuffix[curSetting]);
}

//function is dispense food. TODO: add stepper motor code
void feed(int grams){
  Serial.print("dispensed ");
  Serial.print(grams);
  Serial.println(" grams");
}

void loop() {
  if (encDir != 0 && prevDir == encDir)
  {  
    if(changingVal){
       curVal = curVal + encDir;
       Settingsvals[curSetting] = curVal;
       displaySetting(curSetting);
    }
    else{
       curSetting = constrain(curSetting + encDir,0,ArraySize(Settings)-1);  
       displaySetting(curSetting);
    }
    encDir = 0; 
  }
  
  prevDir = encDir;
  
  if (!digitalRead(encpinSW))
  {
    delay(50);
    while (!digitalRead(encpinSW));
    changingVal = !changingVal;
    curVal = Settingsvals[curSetting].toInt();
    if(curSetting == 2 && changingVal == 0){
      feed(curVal);
      Settingsvals[curSetting] = "0";
      displaySetting(curSetting);
    }
  }
  delay(50);
}
void encoderARising() {
  if ((*pinSensorB & maskSensorB) && encoderAFlag)
  {
    encDir = -1;
    encoderAFlag = false;
    encoderBFlag = false;
  }
  else
  {
    encoderBFlag = true;
  }
}
 
void encoderBRising() {
  if ((*pinSensorA & maskSensorA) && encoderBFlag)
  {
    encDir = 1;
    encoderAFlag = false;
    encoderBFlag = false;
  }
  else
  {
    encoderAFlag = true;
  }
}