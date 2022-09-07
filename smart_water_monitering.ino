#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif
#define DEVICE "nodemcu"
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <SPI.h>
#include <Wire.h>
#define DO_PIN A3
#define VREF 5000    //VREF (mv)
#define ADC_RES 1024 //ADC Resolution
#define TWO_POINT_CALIBRATION 0
#define READ_TEMP (25) //Current water temperature ℃, Or temperature sensor function
#define CAL1_V (131) //mv
#define CAL1_T (25)   //℃
#define CAL2_V (1300) //mv
#define CAL2_T (15)   //℃
#define TdsSensorPin 27
#define VREF 3.3              // analog reference voltage(Volt) of the ADC
#define SCOUNT  30            // sum of sample point
const char* ssid = "R&D-LABS";
const char* pwd = "12345678";
#define INFLUXDB_URL "http://172.16.16.104:8086"
#define INFLUXDB_TOKEN "Q81TdACR277r2wZ4EY8IidFFbLDb-NaCAHvGv8Z7UiVntJ0Sr4Tma1PH3sLsJFh2NoesZdQF3DVuzRNtQdJgNg=="
#define INFLUXDB_ORG "2a787b058def883b"
#define INFLUXDB_BUCKET "iot-water"
#define TZ_INFO "PST8PDT"
#define TRIG_PIN 23 // ESP32 pin GIOP23 connected to Ultrasonic Sensor's TRIG pin
#define ECHO_PIN 22 // ESP32 pin GIOP22 connected to Ultrasonic Sensor's ECHO pin
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
const uint16_t DO_Table[41] = {
  14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
  11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
  9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
  7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410
};
int analogBuffer[SCOUNT];     // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;
float averageVoltage = 0;
float tdsValue = 0.0;
float temperature = 25;       // current temperature for compensation
float oxygen_value;
uint8_t Temperaturet;
uint16_t ADC_Raw;
uint16_t ADC_Voltage;
uint16_t DO;
int16_t readDO(uint32_t voltage_mv, uint8_t temperature_c)
{
#if TWO_POINT_CALIBRATION == 00
  uint16_t V_saturation = (uint32_t)CAL1_V + (uint32_t)35 * temperature_c - (uint32_t)CAL1_T * 35;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#else
  uint16_t V_saturation = (int16_t)((int8_t)temperature_c - CAL2_T) * ((uint16_t)CAL1_V - CAL2_V) / ((uint8_t)CAL1_T - CAL2_T) + CAL2_V;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#endif
}
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0) {
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}
Point sensor("nodemcu");   //Influxdb 1.7, measurement/table
float duration_us, distance_cm;
const int potPin = A0;
float ph;
float Value = 0;
float h ;
float t ;
float speed_mph;
String oxygen ;
void timeSync() {
  configTime(0, 0, "pool.ntp.org", "time.nis.gov");
  setenv("TZ", TZ_INFO, 1);
  Serial.print("Syncing time");
  int i = 0;
  while (time(nullptr) < 1000000000ul && i < 100) {
    Serial.print(".");
    delay(100);
    i++;
  }
  Serial.println();
  time_t tnow = time(nullptr);
  Serial.print("Synchronized time: ");
  Serial.println(String(ctime(&tnow)));
}
#include "DHTesp.h"
#define DHTPIN 15
DHTesp dht;

void mistral_hackfest()
{
  float h = dht.getHumidity();
  float t = dht.getTemperature();
  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.println("Temperature: " + String(t) + "  Humidity " + String(h));
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
delay(100);
  Value = analogRead(potPin);
  Serial.print(Value);
  Serial.print(" | ");
  float voltage = Value * (3.3 / 4095.0);
  ph = (3.3 * voltage);
  Serial.println(ph);
  delay(500);
  float sensorValue = analogRead(A7);
  Serial.print("Analog Value =");
  Serial.println(sensorValue);
  float voltages = (sensorValue / 1023) * 5;
  Serial.print("Voltage =");
  Serial.print(voltage);
  Serial.println(" V");
  float wind_speed = mapfloat(voltages, 0.4, 2, 0, 32.4);
  float speed_mph = ((wind_speed * 3600) / 1609.344);
  Serial.print("Wind Speed =");
  Serial.print(wind_speed);
  Serial.println("m/s");
  Serial.print(speed_mph);
  Serial.println("mph");
  Serial.println("Wind Speed");
  Serial.print(speed_mph, 1);
  Serial.print(" mph");
  delay(300);
  sensor.addField("Temp", t);
  sensor.addField("Hum", h);
  sensor.addField("Anemometer", speed_mph);
  sensor.addField("Water_level", distance_cm);
  sensor.addField("PH_range", ph);
  sensor.addField("Do", oxygen_value);
  uint16_t  value;
  //sensor.addField("TDS", value);
  Serial.println(sensor.toLineProtocol());
  if ((WiFi.RSSI() == 0) && (WiFi.status() != WL_CONNECTED))
    Serial.println("Wifi connection lost");
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  Serial.println("Wait 2s");
  delay(1000);
}
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
void  dissolved()
{
  Temperaturet = (uint8_t)READ_TEMP;
  ADC_Raw = analogRead(DO_PIN);
  ADC_Voltage = uint32_t(VREF) * ADC_Raw / ADC_RES;
  Serial.print(Temperaturet);
  Serial.print(ADC_Raw);
  Serial.print(ADC_Voltage);
  Serial.println(float(readDO(ADC_Voltage, Temperaturet)));
  oxygen_value =(float(readDO(ADC_Voltage, Temperaturet)));
  Serial.println(oxygen_value);
  delay(100);
}

void setup() {
  Serial.begin(115200);
  pinMode(TdsSensorPin, INPUT);
  pinMode(potPin, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pwd);    //Connect to your WiFi router
  Serial.println("");
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(400);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
  Serial.println();
  Serial.println();
  sensor.addTag("device", DEVICE);
  timeSync();
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  dht.setup(DHTPIN, DHTesp::DHT11);
}
void loop() {
//  static unsigned long analogSampleTimepoint = millis();
//  if (millis() - analogSampleTimepoint > 40U) { //every 40 milliseconds,read the analog value from the ADC
//    analogSampleTimepoint = millis();
//    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
//    analogBufferIndex++;
//    if (analogBufferIndex == SCOUNT) {
//      analogBufferIndex = 0;
//    }
//  }
//  static unsigned long printTimepoint = millis();
//  if (millis() - printTimepoint > 800U) {
//    printTimepoint = millis();
//    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++) {
//      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
//      averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 4096.0;
//      float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
//      float compensationVoltage = averageVoltage / compensationCoefficient;
//      tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5;
//      Serial.print("TDS Value:");
//      Serial.print(tdsValue, 2);
//      Serial.println("ppm");
//    }
//  }
  mistral_hackfest();
  dissolved();
}
