#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

ESP8266WebServer wserver(80);

// subistituir pelo eprom
const char* Essid     = "Victor & Eliane";
const char* Epassword = "8vivilindona!@";
String token = "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpYXQiOjE2MDg4MjU1MDksIm5iZiI6MTYwODgyNTUwOSwianRpIjoiMDI2MDYwNzctYWU2OC00MDFhLThiOTgtYWNmOGEyYzQ2OTU4IiwiaWRlbnRpdHkiOiJ2aWN0b3JAZXhhbXBsZS5jb20iLCJmcmVzaCI6ZmFsc2UsInR5cGUiOiJhY2Nlc3MifQ.DO4JXGio5cXVezNF9ZDMepxBaXjS9L8Q79Ojh9conj4";
String userID = "81862626159007";

// tem que ter
const char* ssid = "xurupita";
const char* password = "";
String host     = "http://192.168.86.202:5000";
String pathButton = "change/";
String pathState = "state/";
String pathWatts = "watts/";
int botao = 4;
int led = 5;
bool estadoLed = 0;
String deviceID = String(ESP.getChipId(), HEX);
//String deviceID = "fake001";
uint32_t lastUpdate = 0;
String bearerToken = "Bearer " + token;


void setup() {
  pinMode(botao, INPUT_PULLUP);
  pinMode(led, OUTPUT);
  Serial.begin(9600);
  if (wificonnetion()) {
    Serial.println("WiFi connected");
    Serial.println(" IP address: " + WiFi.localIP() );
    return; // remover o comentario depois
  }
  else {
    ADHOC();
    webserver();  // deve ser chamado depois do adhoc
  }
  // for test
  //webserver();
}

bool wificonnetion() {
  Serial.println("setting up the wifi connection");
  int wificount = 0;
  WiFi.begin(Essid, Epassword);
  while ( wificount < 40 ) {
    delay(1000);
    wificount++;
    Serial.println("tryig connection");
  }
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }
  else {
    return false;
  }
}

void ADHOC() {
  Serial.println("Turning the HotSpot On");
  WiFi.softAP(ssid, password);
  Serial.print("Access Point: \t");
  Serial.println(ssid);
  Serial.println("AdHoc - started");
  Serial.println("IP address: " + WiFi.softAPIP());
}

void webserver() {
  void handleRoot();
  void handleNotFound();
  void readJson();
  wserver.on("/", HTTP_POST, handleRoot);
  wserver.onNotFound(handleNotFound);
  wserver.begin();
  Serial.println("Server started");
  while ((WiFi.status() == WL_CONNECTED)) {
    delay(100);
    wserver.handleClient();
  }
}

void handleRoot() {
  if (wserver.args() == 1) {
    Serial.println(wserver.arg(0));
    DynamicJsonDocument webdoc(2048);
    deserializeJson( webdoc, wserver.arg(0) );
    String jsonstate = webdoc["state"];
    const char* jsonssid = webdoc["ssid"];
    const char* jsonssidpassword = webdoc["ssid_password"];
    const char* jsonoshuserid = webdoc["osh_user_id"];
    const char* jsonoshusertoken = webdoc["osh_user_token"];
    Serial.println(jsonssid);
  }
  // soh pra ter alguma saida por enquanto depois serah uma confirmacao de se deu certo!
  wserver.send(200, "text / plain", "Hello world!");
  // tem que matar o adhoc e conectar no wifi, no final dessa operacao
}

void login() {
  //  StaticJsonDocument<100> dict_json;
  //  dict_json["email"] = "victor@example.com";
  //  dict_json["password"] = "12";
  //  char bufferf[100];
  //  serializeJson(dict_json, bufferf);
  //  //Serial.println(bufferf);
  //  HTTPClient http;
  //  http.begin(host + pathlogin);
  //  http.addHeader("Content-Type", "application/json");
  //  http.POST(bufferf);
  //  Serial.println(http.getString());
  //  http.end();
}

void handleNotFound() {
  wserver.send(404, "text / plain", "404: Not found");
}

void state() {
  //String bearerToken = "Bearer " + token;
  delay(100);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    if (digitalRead(botao) == LOW) {
      http.begin(host + "/" + userID + "/" + deviceID + "/" + pathButton);
      http.addHeader("Authorization", bearerToken );
      int httpCode = http.GET();
      if (httpCode > 0) {
        http.getString();
        while (digitalRead(botao) == LOW) {
          delay(100);
        }
      }
      else {
        estadoLed = !estadoLed;
        digitalWrite(led, estadoLed);
        while (digitalRead(botao) == LOW);
        delay(100);
      }
      http.end();
    }
    http.begin(host + "/" + userID + "/" + deviceID + "/" + pathState);
    http.addHeader("Authorization", bearerToken );
    int httpCode = http.GET();
    if (httpCode > 0) {
      DynamicJsonDocument doc(2048);
      deserializeJson( doc, http.getStream() );
      String jsonstate = doc["state"];
      if (jsonstate == "True") {
        digitalWrite(led, HIGH);
      }
      else {
        digitalWrite(led, LOW);
      }
    }
    http.end();
  }
}

void sendUpdate() {
  if (millis() - lastUpdate > 5000) {
    int readValue;
    int maxValue = 0;
    int minValue = 1024;
    float Vpp;
    float Vrms;
    float current;
    float watts;
    uint32_t startTime = millis();
    while ( millis() - startTime < 1000) {
      // gato, para continuar funcionando mesmo dentro do while
      state();
      readValue = analogRead(A0);
      if (readValue > maxValue) maxValue = readValue;
      if (readValue < minValue) minValue = readValue;
    }
    Vpp = ((maxValue - minValue) * 3.3) / 1024.0;
    Vrms = (Vpp / 2.0) * 0.707;
    current = (Vrms * 1000.0) / 100.0;
    watts = 230.0 * current;
    //timeClient.update();
    //unsigned long time = timeClient.getEpochTime();
    StaticJsonDocument<100> summary;
    //summary["current"] = current;
    summary["watts"] = String(watts);
    //summary["time"] = time;
    char bufferf[100];
    serializeJson(summary, bufferf);
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(host + "/" + userID + "/" + deviceID + "/" + pathWatts);
      http.addHeader("Authorization", bearerToken );
      http.addHeader("Content-Type", "application/json");
      http.POST(bufferf);
    }
    Serial.println(bufferf);
    lastUpdate = millis();
  }
  else {
    return;
  }
}

void loop() {
  state();
  sendUpdate();
}
