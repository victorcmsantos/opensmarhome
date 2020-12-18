#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

ESP8266WebServer wserver(80);

// subistituir pelo eprom
const char* Essid     = "Victor & Eliane";
const char* Epassword = "8vivilindona!@";

// tem que ter
const char* ssid = "xurupita";
const char* password = "";
const char* host     = "http://192.168.86.202:5000";
String pathButton          = "/change";
String pathState          = "/state";
int botao = 4;
int led = 5;
bool estadoLed = 0;



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
    const size_t bufferSize = JSON_OBJECT_SIZE(10) + JSON_OBJECT_SIZE(10) + JSON_OBJECT_SIZE(10) + JSON_OBJECT_SIZE(10) + 370;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& postroot = jsonBuffer.parseObject(wserver.arg(0));
    const char* jsonssid = postroot["ssid"];
    const char* jsonssidpassword = postroot["ssid_password"];
    const char* jsonoshuser = postroot["osh_user"];
    const char* jsonoshpassword = postroot["osh_password"];
    //Serial.println(jsonssid + jsonssidpassword + jsonoshuser + jsonoshpassword);
    Serial.println(jsonssid);
  }
  // soh pra ter alguma saida por enquanto depois serah uma confirmacao de se deu certo!
  wserver.send(200, "text / plain", "Hello world!");

  // tem que matar o adhoc e conectar no wifi, no final dessa operacao
}

void handleNotFound() {
  wserver.send(404, "text / plain", "404: Not found");
}

void loop() {
  delay(100);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    if (digitalRead(botao) == LOW) {
      http.begin(host + pathButton);
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
    http.begin(host + pathState);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      const size_t bufferSize = JSON_OBJECT_SIZE(10) + 370;
      DynamicJsonBuffer jsonBuffer(bufferSize);
      JsonObject& root = jsonBuffer.parseObject(http.getString());
      const char* jsonstate = root["state"];
      if (strcmp(jsonstate, "True") == 0) {
        digitalWrite(led, HIGH);
      }
      else {
        digitalWrite(led, LOW);
      }
    }
    http.end();
  }
}
