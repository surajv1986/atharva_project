 
#include "esp_adc_cal.h"   /* Sensor Specific Header */
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define WLAN_SSID          "Galaxy A50sBEA5"                        /* Enter Hotspot SSID */
#define WLAN_PASS        "NarayanSharma@1986$"                      /* Enter Hotspot Password */ 
#define AIO_UPDATE_RATE_SEC 5
#define AIO_SERVER        "io.adafruit.com"
#define AIO_SERVERPORT  1883                 
#define AIO_USERNAME "mohansir"                                    /* Enter adafruit username */
#define AIO_KEY  "aio_sqFw918rT49mgSD83tgfVrPg1Rpb"                /* Enter adafruit key for authentication with cloud */

TaskHandle_t mqttPubTaskHld = NULL;

WiFiClient client;
/*Create an MQTT Client handle using the specified adafruit server name, port, user name and key */
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);        
/* MQTT Paths for adafruit follow the pattern: <username>/feeds/<feedname> */
Adafruit_MQTT_Publish temp_pub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/LM35_Temperature_test");

/* LM35 GPIO Pin on ESP32 i.e Analog Pin 35, can change to any supported ADC pin on ESP32 dev board */
#define LM35_Sensor1 35

/* Variable to store Raw Values read from the sensors */ 
int rawValue = 0;
/* Variable to store body temperature in Celsius,  processed from the raw values */
float tempInC = 0.0;
/* Variable to store body temperature in Farenheit,  processed from the raw values */
float tempInF = 0.0;
/* Variable to store voltages corresponding to read raw values before converting to celsisus/farenheit */
float Voltage = 0.0;

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

/* Runs only once */ 
void setup()
{
    Serial.begin(9600);
    WiFi.begin(WLAN_SSID, WLAN_PASS);
    while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: "); 
    Serial.println(WiFi.localIP());
                //Serial.begin(115200);
}

/*tempInC = random(3800, 3900) / 100.0; */ //Commented for test purposes
/* Runs continously */
void loop()
{
   
   /* Read raw value from the sensor */
   rawValue = analogRead(LM35_Sensor1);  
   /* Calibrate ADC & Get Voltage (in mV) */
   Voltage = readADC_Cal(rawValue);
   /* Convert Voltage to Celsius */
   tempInC = Voltage / 10;

   /* Connect to Adafruit MQTT server */
   MQTT_connect();

   /* Convert from celsius to farenheit */
   tempInF = (tempInC * 1.8) + 32; 
   /* Print the read and processed values on the serial monitor */
   Serial.print("Temperature = ");
   Serial.print(tempInF);
   Serial.print(" °F , ");
   Serial.print("Temperature = ");
   Serial.print(tempInC);
   Serial.println(" °C");

   /* Publish to cloud */
   if(!temp_pub.publish(tempInF)) {
    Serial.println(F("Failed"));
   }
    else {
    Serial.println(F("OK!"));
   }
   delay(300);
 
}

/*
 *@brief: This API calibrates the value from the LM35 ADC and converts the raw values to voltages in mV.
 *@param : An integer specifying the read raw values from the LM35 Sensor.
 *@return : Equivalent Voltage in milli Volts for the corresponding raw value supplied.
 */ 
uint32_t readADC_Cal(int ADC_Raw)
{
  esp_adc_cal_characteristics_t adc_chars;
  
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  return(esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}
