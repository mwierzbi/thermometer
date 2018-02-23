
#include "Arduino.h"
#include <stdlib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include "Dashboard.h"


#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);
ESP8266WebServer server(80);
MDNSResponder mdns;
DeviceAddress ProbeA = { 0x28, 0xFF, 0x6F, 0x23, 0x82, 0x15, 0x02, 0x4C };
DeviceAddress ProbeB = { 0x28, 0xFF, 0xDE, 0xF6, 0x81, 0x15, 0x02, 0xAA };
// ThingSpeak Settings
char thingSpeakAddress[] = "api.thingspeak.com";
String APIKey = "X37SGO87C4R7EVYB";             // enter your channel's Write API Key
const int updateThingSpeakInterval = 20 * 1000; // 20 second interval at which to update ThingSpeak
// Variable Setup
long lastConnectionTime = 0;
boolean lastConnected = false;

// Initialize Arduino Ethernet Client
WiFiClient client;

String getTemperaturesJson(void)
{
  sensors.requestTemperatures();
  String response = "{";
  response += "\"probeA\": ";
  response += (String)sensors.getTempC(ProbeA);
  response += ",";
  response += "\"probeB\": ";
  response += (String)sensors.getTempC(ProbeB);
  response += "}";
  return response;
}

void printTemperature()
{
  // Serial.print(" Requesting temperatures...");
  sensors.requestTemperatures();
  // Serial.println("DONE");

  // Serial.print("Temperature is: ");
  Serial.print(sensors.getTempC(ProbeA));
  Serial.print("  --  ");
  Serial.println(sensors.getTempC(ProbeB));
  delay(1000);
}

void discoverOneWireDevices(void) {
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];

  Serial.print("Looking for 1-Wire devices...\n\r");
  while(oneWire.search(addr)) {
    Serial.print("\n\rFound \'1-Wire\' device with address:\n\r");
    for( i = 0; i < 8; i++) {
      Serial.print("0x");
      if (addr[i] < 16) {
        Serial.print('0');
      }
      Serial.print(addr[i], HEX);
      if (i < 7) {
        Serial.print(", ");
      }
    }
    if ( OneWire::crc8( addr, 7) != addr[7]) {
        Serial.print("CRC is not valid!\n");
        return;
    }
  }
  Serial.print("\n\r\n\rThat's it.\r\n");
  oneWire.reset_search();
  return;
}

void setup(void)
{
  Serial.begin(9600);
  WiFiManager wifiManager;
  // wifiManager.resetSettings();
  wifiManager.autoConnect("Termometr");
  Serial.println("connected...yeey :)");
  sensors.begin();

  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }
  server.on("/", [](){
    server.send(200, "text/html", getMainPage());
  });
  server.on("/get-temp/", [](){
    server.send(200, "text/html", getTemperaturesJson());
  });

  server.begin();
  Serial.println("HTTP server started");
  discoverOneWireDevices();
}
void updateThingSpeak(String tsData) {
  if (client.connect(thingSpeakAddress, 80)) {
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + APIKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");
    client.print(tsData);
    lastConnectionTime = millis();

    if (client.connected()) {
      Serial.println("Connecting to ThingSpeak...");
      Serial.println();
    }
  } else {
    Serial.println("Connecting to ThingSpeak...NOPE");
  }
}

void loop(void)
{
  server.handleClient();
  // Disconnect from ThingSpeak
    if (lastConnected) {
      Serial.println("...disconnected");
      Serial.println();
      client.stop();
    }
    // Update ThingSpeak
    // if (!client.connected() && (millis() - lastConnectionTime > updateThingSpeakInterval)) {
    //   sensors.requestTemperatures();
    //   updateThingSpeak((String)"field1="+sensors.getTempC(ProbeA) + "&field2=" + sensors.getTempC(ProbeB));
    // }
    // lastConnected = client.connected();
}
