#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>


const char* ssid     = "Victor & Eliane";
const char* password = "8vivilindona!@";
const char* host     = "192.168.86.202"; // Your domain
String pathButton          = "/change";
String pathState          = "/state";
int botao = 4;
int led = 5;


void setup () {
  pinMode(botao, INPUT_PULLUP);
  pinMode(led, OUTPUT);
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting..");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: " + WiFi.localIP());

}

void loop() {
  delay(100);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    if (digitalRead(botao) == LOW) {
      Serial.println("testeeee");
      http.begin("http://192.168.86.202:5000/change");
      int httpCode = http.GET();
      if (httpCode > 0) {
        http.getString();
        while (digitalRead(botao) == LOW) {
          delay(100);
        }
      }
      http.end();
    }


    http.begin("http://192.168.86.202:5000/state");
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();

      //Serial.println(payload);
      const size_t bufferSize = JSON_OBJECT_SIZE(10)+ 370;
      DynamicJsonBuffer jsonBuffer(bufferSize);
      JsonObject& root = jsonBuffer.parseObject(http.getString());
      const char* jsonstate = root["state"];

      
      if (strcmp(jsonstate, "True") == 0) {
        digitalWrite(led, HIGH);
         Serial.println("LED ON");
      }
      else {
        digitalWrite(led, LOW);
        Serial.println("led off");
      }
      Serial.println();


    }
    http.end();

  }
}
