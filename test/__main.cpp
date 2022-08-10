#include <ESP8266WiFi.h>
// #include <DNSServer.h>
// #include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include "LittleFS.h"

#define SPIFFS LittleFS

// ESP8266WebServer server;

AsyncWebServer server(80);

char *ssid = (char *)"POCO X3 Pro";
char *password = (char *)"64287900";

unsigned long streamTime = millis();
boolean messageReady = false;
String message = "";
String handleIndex()
{
  // Send a JSON-formatted request with key "type" and value "request"
  // then parse the JSON-formatted response with keys "gas" and "distance"
  DynamicJsonDocument doc(1024);
  int cx1 = 0;
  // Sending the request
  doc["type"] = "request";
  serializeJson(doc, Serial);
  // Reading the response

  // while (messageReady == false)
  // { // blocking but that's ok
  if (Serial.available())
  {
    message = Serial.readString();
    messageReady = true;
  }
  // }
  // Attempt to deserialize the JSON-formatted message
  DeserializationError error = deserializeJson(doc, message);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return "";
  }
  cx1 = doc["cx1"];
  // gas = doc["gas"];
  // Prepare the data for serving it over HTTP
  String output = F("CX1 Status: \n");
  int cx1_3 = bitRead(cx1, 2);
  int cx1_2 = bitRead(cx1, 1);
  int cx1_1 = bitRead(cx1, 0);

  output += F("Nivel > 70%: ") + String(cx1_3) + F("\n");
  output += F("Nivel > 50%: ") + String(cx1_2) + F("\n");
  output += "Nivel > 30%: " + String(cx1_1) + F("\n");
  // Serve the data as plain text, for example
  // server.send(200, "text/plain", output);
  return output;
}

String processor(const String &var)
{
  Serial.println(var);
  if (var == "STATE")
  {
    // if(digitalRead(ledPin)){
    //   ledState = "ON";
    // }
    // else{
    //   ledState = "OFF";
    // }
    // Serial.print(ledState);
    // return ledState;
    return "OFF";
  }
  else if (var == "TEMPERATURE")
  {
    // return getTemperature();
    return handleIndex();
  }
  else if (var == "HUMIDITY")
  {
    return "0";
    // return getHumidity();
  }
  else if (var == "PRESSURE")
  {
    return "0";
    // return getPressure();
  }
  return "0";
}

void setup()
{

  Serial.begin(9600);

  // Initialize SPIFFS
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  delay(500);
  // Serial.println("/style.css: " + SPIFFS.exists("/style.css"));
  // Serial.println("/index.html: " + SPIFFS.exists("/index.html"));

  // File f = SPIFFS.open("/style.css", "r");
  // if (!f)
  // {
  //   Serial.println("file open failed");
  // }
  // else
  // {
  //   // read 10 strings from file
  //   for (int i = 1; i <= 10; i++)
  //   {
  //     String s = f.readStringUntil('\n');
  //     Serial.print(i);
  //     Serial.print(" : ");
  //     Serial.println(s);
  //   }
  // }
  // f.close();

  //   pinMode(pin_led, OUTPUT);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              DynamicJsonDocument doc(1024);
              doc["type"] = "request";
              serializeJson(doc, Serial);
              request->send(SPIFFS, "/index.html", String(), false); });

  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)

  //           { 
  //             DynamicJsonDocument doc(1024);
  //             doc["type"] = "request";
  //             serializeJson(doc, Serial);
  //             request->send_P(200, "text/plain", "Index!!"); });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/style.css", "text/css"); });

  // Route to set GPIO to HIGH
  // server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
  //   digitalWrite(ledPin, HIGH);
  //   request->send(SPIFFS, "/index.html", String(), false, processor);
  // });

  server.on("/cx1", HTTP_GET, [&](AsyncWebServerRequest *request)
            { 

            request->send_P(200, "text/plain", message.c_str()); 
            
              messageReady = false;
              message = ""; });

  // server.on("/", handleIndex);
  server.begin();
}

#define EXE_INTERVAL_1 5000
#define EXE_INTERVAL_2 3000

unsigned long lastExecutedMillis_1 = 0; // vairable to save the last executed time for code block 1
unsigned long lastExecutedMillis_2 = 0; // vairable to save the last executed time for code block 2

void loop()
{
  // server.handleClient();

  unsigned long currentMillis = millis();

  if (currentMillis - lastExecutedMillis_1 >= EXE_INTERVAL_1)
  {
    lastExecutedMillis_1 = currentMillis; // save the last executed time
    Serial.println(F("each 1 or 5secs"));
    // while (messageReady == false)
    // { // blocking but that's ok
      if (Serial.available())
      {
        message += Serial.readString();
        Serial.println("m:" + message);
        messageReady = true;
      }else{
        messageReady = false;
      }
    // }
  }

  if (messageReady)
  {
  }
}