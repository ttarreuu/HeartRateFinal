#include <Wire.h>
#include <MAX30105.h>
#include <heartRate.h>

#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
MAX30105 sensorHeartRate;

const byte RATE_SIZE = 4; 
byte rates[RATE_SIZE]; 
byte rateSpot = 0;
double finalBPMs[RATE_SIZE];
long lastBeat = 0; 

float BPM;
int beatAvg;

String status;
int bpm;
int age = 20;
String gender = "Male";
const int btn_age = 12;
const int btn_gender = 13;
const int ages[] = {18, 25, 35, 45, 55, 65, 100};
const int bpmLowM[] = {49, 49, 50, 50, 51, 50};
const int bpmHighM[] = {82, 82, 83, 84, 82, 80};
const int bpmLowF[] = {54, 54, 54, 54, 54, 54};
const int bpmHighF[] = {85, 83, 85, 84, 84, 85};
int ageGroup = 1;
const int maxAgeGroup = 6;

String getAgeRange(int _ageGroup) {
  return String(ages[_ageGroup - 1]) + "-" + String(ages[_ageGroup]);
}

int getLowThreshold(int _ageGroup) {
  if (gender == "Male") {
    return bpmLowM[_ageGroup - 1];
  } else {
    return bpmLowF[_ageGroup - 1];
  }
}

int getHighThreshold(int _ageGroup) {
  if (gender == "Male") {
    return bpmHighM[_ageGroup - 1];
  } else {
    return bpmHighF[_ageGroup - 1];
  }
}

int age_btnState = 0;
int age_lastBtnState = 0;
int gender_btnState;
int gender_lastBtnState = HIGH;
bool age_btnPress = false;
bool gender_btnPress = false; 

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");
  pinMode(btn_age, INPUT_PULLUP);
  pinMode(btn_gender, INPUT_PULLUP);
  lcd.begin();
  lcd.backlight(); 

  if (!sensorHeartRate.begin(Wire, I2C_SPEED_FAST)){
    Serial.println("Sensor Not Detected");
    while (1);
  }
  Serial.println("Place your finger on the sensor with steady pressure.");

  sensorHeartRate.setup();
  sensorHeartRate.setPulseAmplitudeRed(0x0A);
  sensorHeartRate.setPulseAmplitudeGreen(0);

}

void loop() {
  check_age();
  check_gender();
  set_default();

  if(age_btnPress){
    age_btnPress = false;
    lcd.setCursor(4, 0);
    lcd.print(age);
  }
  if(gender_btnPress) {
    gender_btnPress = false;
    lcd.setCursor(14, 0);
    lcd.print(gender);
  }

  long irValue = sensorHeartRate.getIR();

  if (checkForBeat(irValue) == true){

    long delta = millis() - lastBeat;
    lastBeat = millis();

    BPM = 60 / (delta / 1000.0);

    if (BPM < 255 && BPM > 20){
      rates[rateSpot++] = (byte)BPM; 
      rateSpot %= RATE_SIZE; 

      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  // Serial.print("IR=");
  Serial.print(irValue);
  // Serial.print(", BPM=");
  // Serial.print(BPM);
  // Serial.print(", Avg BPM=");
  // Serial.print(beatAvg);

  int finalIndex = 1;
  finalBPMs[finalIndex++] = beatAvg;
  finalIndex %= RATE_SIZE;
  if (finalIndex == 0) {
    double finalBPM = 0;
    for (int i = 0; i < RATE_SIZE; i++)
      finalBPM += finalBPMs[i];
    finalBPM /= RATE_SIZE;
    
  }

  String status = getStatus(beatAvg);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("A:");
  lcd.print(getAgeRange(ageGroup));
  lcd.setCursor(8, 0);
  lcd.print("G:");
  lcd.print(gender);
  lcd.setCursor(3, 1);
  lcd.print(beatAvg);
  lcd.setCursor(6, 1);
  lcd.print("(" + status + ")");



  if (irValue < 50000){
    Serial.print("    => No finger");
    beatAvg = 0;
  }
  Serial.println();
}

String getStatus(int beatAvg){
  int lowThreshold = getLowThreshold(ageGroup);
  int highThreshold = getHighThreshold(ageGroup);
  if (beatAvg < lowThreshold){
    return ("Low");
  } else if (beatAvg > highThreshold) {
    return ("High");
  } else {
    return ("Normal");
  }
}

void check_age() {
  age_btnState = digitalRead(btn_age);
  if (age_btnState != age_lastBtnState){
    if (age_btnState == LOW){
      age_btnPress = true;
      ageGroup++;
      ageGroup %= maxAgeGroup;
      if (ageGroup == 0) ageGroup = 1;
      Serial.print("A: ");
      Serial.println(getAgeRange(ageGroup));
    } else{
      Serial.println("off");
    }
    delay(20);
  }
  age_lastBtnState = age_btnState;
}

void check_gender() {
  gender_btnState = digitalRead(btn_gender);
  if (gender_btnState != gender_lastBtnState){
    if (gender_btnState == LOW) {
      gender_btnPress = true;
      if (gender == "Male") {
        gender = "Female";
      } else if (gender == "Female") {
        gender = "Male";
      }
      lcd.setCursor(14, 0);
      lcd.print(gender);
      Serial.println("Gender: " + gender);     
    }
    delay(20);
  }
  gender_lastBtnState = gender_btnState;
}

void set_default(){
  gender_btnState = digitalRead(btn_gender);
  age_btnState = digitalRead(btn_age);
  if (gender_btnState == HIGH && age_btnState == HIGH) {
    ESP.restart();
  }
  delay(20);
}
