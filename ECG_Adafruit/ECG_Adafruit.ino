#include <stdint.h>
#include <Wire.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define WLAN_SSID          "Galaxy A50sBEA5"                      /* Wifi SSID */
#define WLAN_PASS        "NarayanSharma@1986$"                    /* Wifi Password */
#define AIO_UPDATE_RATE_SEC 5
#define AIO_SERVER        "io.adafruit.com"
#define AIO_SERVERPORT  1883                 
#define AIO_USERNAME "mohansir"
#define AIO_KEY  "aio_sqFw918rT49mgSD83tgfVrPg1Rpb"

TaskHandle_t mqttPubTaskHld = NULL;

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);        
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish ecgPubHndle = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Heart_Rate");

/* ESP32 Analog pin connected to the output of the AD8232 ECG Sensor*/
const int outputPin = 26;
/******************************************** MQTT Connect Function *******************************************************/
/* Function to connect and reconnect as necessary to the MQTT server.*/
void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) {
                return;
  }
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {
                Serial.println(mqtt.connectErrorString(ret));
                Serial.println("Retrying MQTT connection in 5 seconds...");
                mqtt.disconnect();
                delay(5000);
                retries--;
                if (retries == 0) {
                while (1);
                }
  }
  Serial.println("MQTT Connected!");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
 
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP());
 
}

void loop() {

  int ecg_val = 0;
  // put your main code here, to run repeatedly:
  MQTT_connect();

  /* Read ECG Val */
  ecg_val = analogRead(outputPin);
  Serial.println(ecg_val);
  if (!ecgPubHndle.publish(ecg_val)) {
 Serial.println(F("Failed"));
 }
 else {
 Serial.println(F("OK!"));
 }
 delay(300);
}
