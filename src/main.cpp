
#include "painlessMesh.h"
#include <SPI.h>
#include <LoRa.h>
#include <EEPROM.h>
// GPIO5  -- SX1278's SCK
// GPIO19 -- SX1278's MISO
// GPIO27 -- SX1278's MOSI
// GPIO18 -- SX1278's CS
// GPIO14 -- SX1278's RESET
// GPIO26 -- SX1278's IRQ(Interrupt Request)
// OLED pins to ESP32 0.96OLEDGPIOs :
// OLED_SDA -- GPIO4
// OLED_SCL -- GPIO15
// OLED_RST -- GPIO16
#define MESH_PREFIX "AlphaMesh"
#define MESH_PASSWORD "MESHpassword"
#define MESH_PORT 5555
#define WIFI_CHANNEL 6 

#define SCK 14
#define MISO 12
#define MOSI 13
#define SS 15
#define RST 16
#define DIO0 4
#define BAND 915E6

int value;
int address = 0;

void readEEPROM()
{
  // read data from eeprom

  value = EEPROM.read(address);
  Serial.print("Read Id = ");
  Serial.print(value, DEC);
}

void receivedCallback(uint32_t from, String &msg);
painlessMesh mesh;
Scheduler userScheduler;
// Send my ID every 10 seconds to inform others
Task logServerTask(25000, TASK_FOREVER, []()
                   {
  DynamicJsonDocument msg(1024);
//  JsonObject& msg = jsonBuffer.createObject();
  msg["topic"] = "logServer";
  msg["idNode"] = value;
  String str;
  serializeJson(msg, str);
  mesh.sendBroadcast(str);
  // log to serial
  serializeJson(msg, Serial);
  Serial.printf("\n"); });

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(512);
  readEEPROM();
  Serial.println("LoRa PainlessMesh Server");
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(BAND))
  {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
  Serial.println("LoRa Initial OK!");
  mesh.setDebugMsgTypes(ERROR | CONNECTION | S_TIME);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, WIFI_CHANNEL, PHY_MODE_11G);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection([](size_t nodeId)
                       { Serial.printf("New Connection %u\n", nodeId); });
  mesh.onDroppedConnection([](size_t nodeId)
                           { Serial.printf("Dropped Connection %u\n", nodeId); });
  // Add the task to the mesh scheduler
  userScheduler.addTask(logServerTask);
  logServerTask.enable();
}
void loop()
{
  mesh.update();
}

void receivedCallback(uint32_t from, String &msg)
{

  String tmp_string = msg.c_str();

  Serial.printf("logServer: Received from %u msg=%s\n", from, msg.c_str());

  Serial.println("");
  Serial.println("Sending LoRa packet: " + tmp_string);

  //เมื่อได้รับข้อความจากใน mesh network ก็ส่งต่อผ่านไปยัง LoRa
  LoRa.enableCrc();
  LoRa.beginPacket();
  LoRa.print(tmp_string);
  LoRa.endPacket();
}
