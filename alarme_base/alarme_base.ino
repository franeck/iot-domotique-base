#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

/* ================================= Wifi ================================= */
const char* ssid = "iotbipbip";  // Enter SSID here
const char* password = "esp2019iot";  //Enter Password here

/* ================================= Web Server ================================= */

ESP8266WebServer server(80);

/* ================================= RFID ================================= */
// System
#define CARDS 4

// NFC reader
#define RST_PIN 2
#define SS_PIN 15
MFRC522 mfrc522(SS_PIN, RST_PIN);

// EEPROM
boolean users[CARDS] = {false,false,false,false};
String userTags[CARDS] = {
  "95389620",
  "e5ce9220",
  "13426d4",
  "338022d4"
};
int rfidNextCheck = 0;

// Buzzer
#define BUZZ 5
int buzzTimeOn[20];
int buzzTimeOff[20];
boolean buzzOn = false;

/* ================================= Alarm ================================= */

boolean alarmActive = false;
boolean alarmArmed = false;
int alarmNextCheck = 0;

String alarmReason[2] = {"", ""};
boolean alarmSource[2] = {false, false};

/* ================================= Sensors ================================= */

String sensorIP[3] = {"192.168.43.121", "192.168.43.115", "192.168.43.77"};
boolean sensorOpen[3] = {false, false, false};
int sensorLostConnections[3] = {0, 0, 0};
int sensorNextCheck = 0;
int sensorMaxFail = 5;
boolean sensorsHealthy = true;

/* ================================= Tools ================================= */

int nextDebug = 0;

/* ================================= SETUP ================================= */

void setup() {
  Serial.begin(115200);

  // Wifi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
     delay(500);
  }

  // Http server routing
  server.on("/", handlePing);
  server.on("/event", handleEvent);
  server.on("/state", handleState);
  server.onNotFound(handleNotFound);
  server.begin();

  // RFID
  SPI.begin();
  mfrc522.PCD_Init(); 
  Serial.println();
  loadUsers();
  printUsers();
  pinMode(BUZZ, OUTPUT);
  digitalWrite(BUZZ, LOW);

  // OTA implementation
  ArduinoOTA.setHostname("iot-base");
  ArduinoOTA.begin();
}


/* ================================= LOOP ================================= */

void loop() {
  server.handleClient();
  ArduinoOTA.handle(); 

  checkBuzzer();
  checkRfidReader(500);
  checkBuzzer();
  checkAlarm(200);
  checkBuzzer();
  checkSensors(1000);
  checkBuzzer();
  
}

/* ================================= HTTP ================================= */

void handlePing() {
  server.send(200, "text/html", "pong"); 
}

void handleEvent() {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, server.arg("plain"));
  int sensorId = doc["event"]["sensorId"];  
  boolean opening = doc["event"]["opening"];
  if(opening) {
    Serial.println("New event, entrance "+String(sensorId)+" is now open");
  } else {
    Serial.println("New event, entrance "+String(sensorId)+" is now closed");
  }
  server.send(200); 
  handleSensorEvent(sensorId, opening);
}

void handleState() {
  DynamicJsonDocument doc(1024);
  JsonObject root = doc.to<JsonObject>();
  JsonArray deviceArray = root.createNestedArray("devices");
  for(int i = 0; i < 3; i++) {
    JsonObject device = deviceArray.createNestedObject();
    device["id"] = i; 
    device["closed"] = !sensorOpen[i];
  }
  String body;
  serializeJson(root, body);
  server.send(200, "application/json", body); 
}

void handleNotFound(){
  server.send(404, "text/plain", "Not found");
}

/* ================================= RFID ================================= */

void checkRfidReader(int checkDelay) {
  if(millis() > rfidNextCheck) {
    String tag = getNFCtag();
    if(tag != "") {
    
      Serial.println(tag);  
      int result = registerEntrance(tag);
      switch (result) {
        case 0:
          sendTone(90, 2, 100);
          break;
        case 1:
          sendTone(100, 1, 0);
          break;
        default:
          sendTone(100, 3, 50);
      }
      mfrc522.PICC_HaltA();
    }
    rfidNextCheck = millis() + checkDelay;
  }
}

String getNFCtag() {
  if ( !mfrc522.PICC_IsNewCardPresent()) {
   return "";
  }
  if(! mfrc522.PICC_ReadCardSerial()) {
    return "";
  }
  String cardId;
  for (int i = 0; i < 4; i++) { 
    cardId +=  String(mfrc522.uid.uidByte[i], HEX);
  }
  return cardId;
}

/* ================================= Presence monitoring ================================= */

int registerEntrance(String uid) {
  int id = -1;
  for(int i = 0; i < 4; i++) {
    if(uid == userTags[i]) {
      id = i;
      break;
    }
  }
  if(id > -1) {
    users[id] = !users[id];
    persistUsers();
    printUsers();
    return  users[id];
  } else {
    return -1;
  }
}

void persistUsers() {
  EEPROM.begin(4);
  for(int i = 0; i < 4; i++) {
    EEPROM.put(i, users[i]);
  }
  EEPROM.commit();
  EEPROM.end();
}

void loadUsers() {
  EEPROM.begin(4);
  for(int i = 0; i < 4; i++) {
    EEPROM.get(i, users[i]);
  }
  EEPROM.end();
}

void printUsers() {
  for(int i = 0; i < 4; i++) {
    Serial.print("User ");
    Serial.print(i);
    Serial.print(" is ");
    if(users[i] == 1) {
      Serial.println("HOME");
    } else {
      Serial.println("OUT");
    }
  }
}

boolean isHomeEmpty() {
  for(int i = 0; i < 4; i++) {
    if(users[i] == 1) {
      return false;
    }
  }
  return true;
}

/* ================================= Alarm ================================= */

void checkAlarm(int checkDelay) {
  if(millis() > alarmNextCheck) {
    findoutAlarmActive();

    if(alarmSource[0]) {
      if(!alarmArmed) {
        armTheAlarm(alarmReason[0]);
      }
    } else if (alarmSource[1]) {
      if(!alarmArmed) {
        armTheAlarm(alarmReason[1]);
      }
    } else {
      if(alarmArmed) {
        disarmTheAlarm();
      }
    }



    alarmNextCheck = millis() + checkDelay;
  }
}



void findoutAlarmActive() {
  if (isHomeEmpty()) {
    alarmActive = true;
  } else {
    alarmActive = false;
    if(alarmArmed) {
      alarmReason[1] = "";
      alarmSource[1] = false;
    }
  }
}

void armTheAlarm(String reason) {
  if(!alarmArmed) {
    // Call the server here
    alarmArmed = true;
    sendTone(250, 20, 2000);
    Serial.println("Arm the alarm!!! | " + reason);
  }
}

void disarmTheAlarm() {
  // Call the server here
  alarmArmed = false;
  clearTone();
  Serial.println("Nevermind, disarm the alarm");
}

/* ================================= Sensors ================================= */

void checkSensors(int checkDelay) {
  if(millis() > sensorNextCheck) {
    getSensorsState();
    String failedSensors = "";
    sensorsHealthy = true;
    for (int i = 0; i < 3; i++) { 
      if(sensorLostConnections[i] > sensorMaxFail) {
        failedSensors += String(i)+" ";
        sensorsHealthy = false;
      }
    }
    if(!sensorsHealthy ) {
      alarmReason[0] = "Following sensors are offline: "+failedSensors;
      alarmSource[0] = true;  
    } else {
      alarmReason[0] = "";
      alarmSource[0] = false;  
    }
    sensorNextCheck = millis() + checkDelay;
  }
}

void getSensorsState() {
  for (int i = 0; i < 3; i++) { 
    checkBuzzer();
    HTTPClient http;
    http.setTimeout(200);
    http.begin("http://"+sensorIP[i]+"/state");
    int httpCode = http.GET();
    
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, http.getString());
    
    if (httpCode == 200) {
      sensorOpen[i] = doc["open"];
      sensorLostConnections[i] = 0;
    } else {
      sensorOpen[i] = NULL;
      sensorLostConnections[i] += 1;
    }
    
    http.end();
  }
}

void handleSensorEvent(int sensorId, boolean opening) {
  if(alarmActive && opening) {
    alarmReason[1] = "Entrance "+String(sensorId)+" is open!";
    alarmSource[1] = true;
  }
}


/* ================================= Buzzer ================================= */

void sendTone(int duration, int count, int buzzDelay) {
  clearTone();
  for (int i = 0; i < count; i++) { 
    buzzTimeOn[i] = millis() + buzzDelay * i + duration * i;
    buzzTimeOff[i] = millis() + duration * (i+1) + buzzDelay * i;
  }
}

void clearTone() {
  digitalWrite(BUZZ, LOW);
  buzzOn = false;
  for (int i = 0; i < 20; i++) { 
    buzzTimeOn[i] = 0;
    buzzTimeOff[i] = 0;
  }
}

void checkBuzzer() {
  if(buzzOn) {
    for (int i = 0; i < 20; i++) { 
      if(buzzTimeOff[i] > 0) {
        if(millis() > buzzTimeOff[i]) {
          digitalWrite(BUZZ, LOW);
          buzzOn = false;
          buzzTimeOff[i] = 0;
          return;
        } else {
          return;
        }
      }
    }
  } else {
    for (int i = 0; i < 20; i++) { 
      if(buzzTimeOn[i] > 0) {
        if(millis() > buzzTimeOn[i]) {
          digitalWrite(BUZZ, HIGH);
          buzzOn = true;
          buzzTimeOn[i] = 0;
          return;
        } else {
          return;
        }
      }
    }
  }
}

/* ================================= Tools ================================= */

void printDebug() {
  if(millis() > nextDebug) {
    if(alarmActive) {
      Serial.println("Alarm is active");
    } else {
      Serial.println("Alarm is off");
    }
    nextDebug = millis() + 1000;
  }
}
