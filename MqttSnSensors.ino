#include <SPI.h>
#include <dht.h>
#include <BtMqttSn.h>

//-----

#define SENSOR_ID 1

//-----

#define xstr(s) str(s)
#define str(s) #s

#define CLIENT_NODE_ID (SENSOR_ID)
#define CLIENT_ID "BtSensor" xstr(SENSOR_ID)
#define PUBLISH_BASE "Bt/Sensors/" xstr(SENSOR_ID) "/"

#define PUBLISH_UPTIME_TOPIC PUBLISH_BASE "Up"
#define PUBLISH_CONNECT_UPTIME_TOPIC PUBLISH_BASE "ConUp"
#define PUBLISH_RECONNECT_COUNTER_TOPIC PUBLISH_BASE "ReCon"
#define PUBLISH_TEMPERATURE_TOPIC PUBLISH_BASE "T"
#define PUBLISH_HUMIDITY_TOPIC PUBLISH_BASE "H"

#define CLIENT_NODE_ID SENSOR_ID
#define GATEWAY_NODE_ID 0

#define DHT22_READ_INTERVAL 30000
#define RETRY_CONNECT_DELAY 5000

#define CHIP_ENABLE 9
#define CHIP_SELECT 10

#define DHT22_PIN 7

//-----

MqttSnClient client;
dht DHT;
unsigned long nextReadTime;
unsigned long lastConnect;
unsigned long reconnectCounter;

void setup() {
   Serial.begin(9600);
   Serial << endl << endl << endl << "*** MQTT-SN publish temperature example ***" << endl;
   Serial << endl;
   Serial << " - Node                   = " << CLIENT_NODE_ID << endl;
   Serial << " - Pup-Topic Temperature  = " << PUBLISH_TEMPERATURE_TOPIC << endl;
   Serial << " - Pup-Topic Humidity     = " << PUBLISH_HUMIDITY_TOPIC << endl;

   client.begin(CHIP_ENABLE, CHIP_SELECT, CLIENT_NODE_ID, GATEWAY_NODE_ID, CLIENT_ID);

   Serial << "try connect ..." << endl;

   while (!client.connect()) {
      Serial << "... connect failed, retry @ " <<  (millis() + RETRY_CONNECT_DELAY) << " ..." << endl;
      delay(RETRY_CONNECT_DELAY);
      Serial << "... retry connect ..." << endl;
   }

   unsigned long now = millis();
   lastConnect = now;
   nextReadTime = now;

   Serial << "... connected" << endl;
}

void loop() {
   if(client.loop()) {
     if(nextReadTime < millis()) {
       publishTemperature();
       nextReadTime = millis() + DHT22_READ_INTERVAL;
     }
   } else {
     Serial << "Connection lost try reconnect ..." << endl;
     while (!client.connect()) {
       Serial << "... reconnect failed, retry @ " <<  (millis() + RETRY_CONNECT_DELAY) << " ..." << endl;
       delay(RETRY_CONNECT_DELAY);
       Serial << "... retry reconnect ..." << endl;
     }
     reconnectCounter++;
     lastConnect = millis();
   }
}

void publishTemperature() {
  int answer = DHT.read22(DHT22_PIN);
  if(answer != DHTLIB_OK) {
    switch (answer)
    {
      case DHTLIB_ERROR_CHECKSUM: Serial << "DHT Checksum error" << endl; return;
      case DHTLIB_ERROR_TIMEOUT : Serial << "DHT Time out error" << endl; return;
    }
  }
  char buffer[20] = {0};
  unsigned long uptime = millis();
  unsigned long connectUptime = uptime - lastConnect;
  Serial << "publish T = " << DHT.temperature << ", H = " << DHT.humidity << " Up " << uptime << " ConUp " << connectUptime << " ReCon " << reconnectCounter << endl;
  client.publish(PUBLISH_UPTIME_TOPIC, ultoa(uptime,buffer,10));
  client.publish(PUBLISH_CONNECT_UPTIME_TOPIC, ultoa(connectUptime,buffer,10));
  client.publish(PUBLISH_RECONNECT_COUNTER_TOPIC, ultoa(reconnectCounter,buffer,10));
  client.publish(PUBLISH_TEMPERATURE_TOPIC, dtostrf(DHT.temperature,4,2,buffer));
  client.publish(PUBLISH_HUMIDITY_TOPIC, dtostrf(DHT.humidity,4,2,buffer));
}

/*
int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
*/
