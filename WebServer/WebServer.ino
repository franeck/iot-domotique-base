#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

/* ================================= SSID et mot de passe du point d'acces ================================= */
const char* ssid = "NodeMCU_test";  // Enter SSID here
const char* password = "12345678";  //Enter Password here

/* ================================= Details reseau ================================= */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

/* ================================= Port serveur web ================================= */

ESP8266WebServer server(80);

/* ================================= PINs ================================= */

uint8_t event_pin = D7;
bool event_status = LOW;

uint8_t state_pin = D6;
bool state_status = LOW;

/* ================================= SETUP ================================= */

void setup() {
  Serial.begin(115200);
  pinMode(event_pin, OUTPUT);
  pinMode(state_pin, OUTPUT);

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  
  server.on("/", handle_OnConnect);
  server.on("/event", handle_event);
  server.on("/state", handle_state);
  server.onNotFound(handle_NotFound);
  
  server.begin();
  Serial.println("HTTP server started");

  struct event
  {
    boolean opening;
    long timing; //time ne peut pas etre utilise
  };

  struct devices
  {
    int id;
    boolean closed;
  };

  
}


/* ================================= LOOP ================================= */

void loop() {
  server.handleClient();
  if(event_status)
  {digitalWrite(event_pin, HIGH);}
  else
  {digitalWrite(event_pin, LOW);}
  
  if(state_status)
  {digitalWrite(state_pin, HIGH);}
  else
  {digitalWrite(state_pin, LOW);}
}

void handle_OnConnect() {
  event_pin = LOW;
  state_pin = LOW;
  Serial.println("event | state");
  server.send(200, "text/html", SendHTML(event_pin,state_pin)); 
}

void handle_event() {
  event_status = HIGH;
  Serial.println("event : POST");
  server.send(200, "text/html", SendHTML(true,state_status)); 
}

void handle_state() {
  event_status = LOW;
  Serial.println("status : GET");
  server.send(200, "text/html", SendHTML(false,state_status)); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

/* ================================= Interface HTML ================================= */

String SendHTML(uint8_t event_stat,uint8_t state_stat){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>LED Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #62eb3b;}\n";
  ptr +=".button-on:active {background-color: #9efc84;}\n";
  ptr +=".button-off {background-color: #ff1111;}\n";
  ptr +=".button-off:active {background-color: #fc8484;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>Serveur Web RFID</h1>\n";
  ptr +="<h3>Par Point d'Acces (Access Point)</h3>\n";
  
   if(event_stat)
  {ptr +="<p>Event Status: ON</p><a class=\"button button-off\" href=\"/event\">OFF</a>\n";}
  else
  {ptr +="<p>Event Status: OFF</p><a class=\"button button-on\" href=\"/event\">ON</a>\n";}

  if(state_stat)
  {ptr +="<p>State Status: ON</p><a class=\"button button-off\" href=\"/state\">OFF</a>\n";}
  else
  {ptr +="<p>State Status: OFF</p><a class=\"button button-on\" href=\"/state\">ON</a>\n";}

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
