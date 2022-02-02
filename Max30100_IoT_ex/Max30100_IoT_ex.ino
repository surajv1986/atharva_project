#include <stdint.h>
#include <Wire.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "MAX30100_PulseOximeter.h"  
#define WLAN_SSID          "Galaxy A50sBEA5"
#define WLAN_PASS        "NarayanSharma@1986$"
#define AIO_UPDATE_RATE_SEC 5
#define AIO_SERVER        "io.adafruit.com"
#define AIO_SERVERPORT  1883                 
#define AIO_USERNAME "mohansir"
#define AIO_KEY                "aio_sqFw918rT49mgSD83tgfVrPg1Rpb"
#define I2C_SDA   21
#define I2C_SCL   22
TaskHandle_t pulse_oxReadTaskHandle = NULL;
TaskHandle_t mqttPubTaskHandle = NULL;
/* PulseOximeter is the higher-level interface to the sensor */
PulseOximeter pulse_ox;
uint32_t tsLastReport = 0;
float bpm_dt=0;
float spo2_dt = 0;
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);        
Adafruit_MQTT_Subscribe sw_sub = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/switch");
/* MQTT paths for AIO follow the form: <username>/feeds/<feedname> */
Adafruit_MQTT_Publish bpm_pub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/bpm");
Adafruit_MQTT_Publish spo2_pub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/spo2");
/* Callback (registered below) fired when a pulse is detected */
void onBeatDetected()
{
                Serial.println("Beat!");
}
/******************************************* Function for pause MAX30100 Read **************************************************/
void stopReadpulse_ox(){
  pulse_ox.shutdown();
}
/******************************************* Function for Start MAX30100 Read **************************************************/
void startReadpulse_ox(){
  pulse_ox.resume();
}
/******************************************* MAX30100 Read task **************************************************/
void pulse_oxReadTask( void * param )
{
  while(1){
                // Make sure to call update as fast as possible
                pulse_ox.update();
                vTaskDelay( 1 / portTICK_PERIOD_MS );
  }
  pulse_oxReadTaskHandle = NULL;
  vTaskDelete(NULL); // kill itself
}
/******************************************* MQTT publish task **************************************************/
void mqttPubTask( void * param )
{
  uint8_t sec_count=0;
  while(1){
                Serial.print("Heart rate:");
                float bpm_dt = pulse_ox.getHeartRate();
                Serial.print(bpm_dt);
                Serial.print("bpm / SpO2:");
                float spo2_dt = pulse_ox.getSpO2();
                Serial.print(spo2_dt);
                Serial.println("%");
                if(sec_count >= AIO_UPDATE_RATE_SEC){
                if (! bpm_pub.publish(bpm_dt)) {
                Serial.println(F("Failed to publish bmp.."));
                } else {
                Serial.println(F("bmp publish OK!"));
                }
                if (! spo2_pub.publish(spo2_dt)) {
                Serial.println(F("Failed to publish SpO2.."));
                } else {
                Serial.println(F("SpO2 publish OK!"));
                }
                sec_count=0;
                }
                vTaskDelay( 1000 / portTICK_PERIOD_MS );
                sec_count++;
  }
  mqttPubTaskHandle = NULL;
  vTaskDelete(NULL); // kill itself
}
/******************************************** MQTT Connect Function *******************************************************/
/* Function to connect and reconnect as necessary to the MQTT server. */
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
/**************************************************************************************************/
void setup()
{
                Serial.begin(115200);
                Wire.begin(I2C_SDA, I2C_SCL);
                WiFi.begin(WLAN_SSID, WLAN_PASS);
                while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
                }
                Serial.println();
                Serial.println("WiFi connected");
                Serial.println("IP address: "); Serial.println(WiFi.localIP());
                mqtt.subscribe(&sw_sub);
                Serial.print("Initializing pulse oximeter..");
                /* Initialize the PulseOximeter instance */
                /* Failures occur mostly due to an improper I2C wiring, missing power supply
                   or wrong target chip */
                if (!pulse_ox.begin()) {
                Serial.println("FAILED");
                for(;;);
                } else {
                Serial.println("SUCCESS");
                }
                /* The default current for the IR LED is 50mA and it could be changed
                   by uncommenting the following line. Check MAX30100_Registers.h for all the
                   available options */
                pulse_ox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
                /* Bind callback for the beat detection */
                pulse_ox.setOnBeatDetectedCallback(onBeatDetected);
                stopReadpulse_ox();
}
void loop() {
  MQTT_connect();
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000)))
  {
                if (subscription == &sw_sub)
                {
                Serial.print(F("Got: "));
                Serial.println((char *)sw_sub.lastread);
                if (!strcmp((char*) sw_sub.lastread, "ON"))
                {
                Serial.print(("Starting pulse_ox... "));
                startReadpulse_ox();
                BaseType_t xReturned;
                if(pulse_oxReadTaskHandle == NULL){
                xReturned = xTaskCreate(
                                pulse_oxReadTask,      /* Function that implements the task. */
                                "pulse_ox_read",           /* Text name for the task. */
                                1024*3,                  /* Stack size in words, not bytes. */
                                NULL,    /* Parameter passed into the task. */
                                2,/* Priority at which the task is created. */
                                &pulse_oxReadTaskHandle );          /* Used to pass out the created task's handle. */
                }
                delay(100);
                if(mqttPubTaskHandle == NULL){
                xReturned = xTaskCreate(
                                mqttPubTask,       /* Function that implements the task. */
                                "mqttPub",             /* Text name for the task. */
                                1024*3,                  /* Stack size in words, not bytes. */
                                NULL,    /* Parameter passed into the task. */
                                2,/* Priority at which the task is created. */
                                &mqttPubTaskHandle );            /* Used to pass out the created task's handle. */
                }
                }
                else
                {
                Serial.print(("Stoping pulse_ox... "));
                /* Detele pulse_ox read task*/
                if(pulse_oxReadTaskHandle != NULL){
                vTaskDelete(pulse_oxReadTaskHandle);
                pulse_oxReadTaskHandle = NULL;
                }
                /* Delete the MQTT Pub Task */
                if(mqttPubTaskHandle != NULL){
                vTaskDelete(mqttPubTaskHandle);
                mqttPubTaskHandle = NULL;
                }
                stopReadpulse_ox();
                }
                }
  }
}
