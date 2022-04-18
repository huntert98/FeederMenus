//BASE CODE FOR READING ROTARY ENCODER HERE: https://www.semesin.com/project/2018/10/10/menu-arduino-dengan-rotary-encoder/
#include <LiquidCrystal.h>

#define encpinCLK     3 //Encoder CLK
#define encpinDT      2 //Encoder DT
#define encpinSW      4 //Encoder SW
#define THERMISTOR    A1
#define DIR           9
#define STEP          10
#define ENABLE        8
#define HEATBED       12
#define PUMP          13
#define WTRSENSOR     A0
#define WTERENABLE    7

const int rs = 5, en = 6, d4 = A2, d5 = A3, d6 = A4, d7 = A5; //RESET->D5, ENABLE->D6, D4->A2, D5->A3, D6->A4, D7->A5
const int stepsPerGram = 50;
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
byte setMode;
String Settings[] = {"Feed Time:", "Feed Amnt:", "Feed Now:", "Temp:"};
String Settingsvals[] = {"2", "25", "0", "40"};
String SettingsSuffix[] = {" min", " g", " g","\337F"};
bool changingVal = false;
int curVal = 0;
unsigned long onTime;

template<typename T, size_t N> size_t ArraySize(T(&)[N]){ return N; }
 
void setup() {
  Serial.begin(9600);
 
  pinMode(encpinCLK, INPUT_PULLUP);
  pinMode(encpinDT, INPUT_PULLUP);
  pinMode(encpinSW, INPUT_PULLUP);
  pinMode(DIR,OUTPUT);
  pinMode(STEP,OUTPUT);
  pinMode(ENABLE,OUTPUT);
  pinMode(WTERENABLE,OUTPUT);
  pinMode(PUMP,OUTPUT);  
  pinMode(WTRSENSOR,INPUT);
  lcd.begin(16, 2);
 
  attachInterrupt(digitalPinToInterrupt(encpinCLK), encoderARising, RISING); //Map rising edge to handler functions
  attachInterrupt(digitalPinToInterrupt(encpinDT), encoderBRising, RISING);
 
  maskSensorA  = digitalPinToBitMask(encpinCLK);
  pinSensorA = portInputRegister(digitalPinToPort(encpinCLK));
  maskSensorB  = digitalPinToBitMask(encpinDT);
  pinSensorB = portInputRegister(digitalPinToPort(encpinDT));

  digitalWrite(WTERENABLE,LOW);
  digitalWrite(ENABLE,HIGH);
  lcd.clear();
  displayStats();
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

unsigned long lastFeeding = 0;
//function is dispense food. TODO: add stepper motor code
void feed(int grams){
  lastFeeding = onTime;

  digitalWrite(ENABLE,LOW);
  digitalWrite(DIR,HIGH);
  for(int x=0; x <= grams * stepsPerGram; x++){
    digitalWrite(STEP,HIGH);
    delayMicroseconds(500);
    digitalWrite(STEP,LOW);
    delayMicroseconds(500);
  }  
  digitalWrite(ENABLE,HIGH);
  digitalWrite(HEATBED,LOW);
  
  Serial.print("dispensed ");
  Serial.print(grams);
  Serial.println(" grams");
}

String lastTempStr = "";
unsigned long lastRefresh = 0;
int sectilFeed = 9999;
//function for printing values on first row of display
void displayStats(){
  if(onTime - lastRefresh > 1000){
    lastRefresh = onTime;
    lcd.setCursor(0, 0);
    //lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print("\337F:");
    String tempStr = String(getTemp(),0);
    if(tempStr != lastTempStr){
      lcd.print("   ");
      lcd.setCursor(3, 0);
      lcd.print(tempStr);
      lastTempStr = tempStr;
    }
    lcd.setCursor(7, 0);
    lcd.print("   ");
    lcd.setCursor(7, 0);
    lcd.print(sectilFeed);
    lcd.print("      ");
    lcd.setCursor(12, 0);
    lcd.print(getWaterLevel());
    lcd.print("%");
  }
}


float getTemp(){
  int Vo;
  float R1 = 10000;
  float logR2, R2, T, Tc, Tf;
  float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
  Vo = analogRead(THERMISTOR);
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  Tc = T - 273.15;
  Tf = (Tc * 9.0)/ 5.0 + 32.0; 

  return Tf;
//  Serial.print("Temperature: "); 
//  Serial.print(Tf);
//  Serial.print(" F; ");
//  Serial.print(Tc);
//  Serial.println(" C");   
}

unsigned long lastHeaterON = 0;
void heatBed(){
   if(onTime - lastHeaterON > 5000){
    int tempF = getTemp();
    int setTemp = Settingsvals[3].toInt();
    //Serial.print(setTemp);
    //Serial.print("   ");
    //Serial.println(tempF);
    
    if (tempF < setTemp){
      digitalWrite(HEATBED, HIGH);
      lastHeaterON = onTime;
    }
    else{
      digitalWrite(HEATBED, LOW);
    }
  }
}

int getWaterLevel(){
    digitalWrite(WTERENABLE,HIGH);
    int wtrVal = analogRead(WTRSENSOR);
    wtrVal = map(wtrVal, 0, 180, 0, 100);
    digitalWrite(WTERENABLE,LOW);
    //Serial.println(wtrVal);
    return wtrVal;
}

unsigned long lastPumpReading = 0;
void pump(){
  if(onTime - lastPumpReading > 1000){
    int wtrVal = getWaterLevel();
    lastPumpReading = onTime;
  }
}

void loop() {
  getTemp();
  onTime = millis();
  sectilFeed = ((Settingsvals[0].toInt()*60)+(lastFeeding/1000))-(onTime/1000);
  if (encDir != 0 && prevDir == encDir) //if encoder turned and it turned the same direction as last turn
  {  
    if(changingVal){ //if button was pressed to edit value
       curVal = curVal + encDir; //add 1 to current value
       curVal = constrain(curVal, 1, 999);
       Settingsvals[curSetting] = curVal; //set menu setting to new value
       displaySetting(curSetting); //update display
    }
    else{ //cycling through menu screens
       curSetting = constrain(curSetting + encDir,0,ArraySize(Settings)-1);  //only allow indexes that are valid in settings array
       displaySetting(curSetting); //update display
    }
    encDir = 0; //reset encoder direction
  }
  
  prevDir = encDir; //set previous direction
  
  if (!digitalRead(encpinSW)) //if button was pressed
  {
    delay(50); //debounce
    while (!digitalRead(encpinSW)); //debounce
    changingVal = !changingVal; //flip the value
    curVal = Settingsvals[curSetting].toInt(); //get current setting value String->int
    if(curSetting == 2 && changingVal == 0){ //if FEED NOW
      feed(curVal); //feed
      Settingsvals[curSetting] = "0"; //reset feed to zero
      displaySetting(curSetting); //update display
    }
  }

  if(sectilFeed < 0){
    feed(Settingsvals[1].toInt());
  }
  displayStats();
  heatBed();
  pump();
  delay(50); //helps smooth things out
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
