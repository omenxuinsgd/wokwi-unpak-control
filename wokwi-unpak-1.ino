#include "DHTesp.h"
#include <WiFi.h>
#include <PubSubClient.h>

#define ECHO 13
#define TRIG 12
#define DHT_PIN 15
#define POMPA_PIN 2
#define LAMPU_PIN 4

char clientId[50];

WiFiClient espClient;
PubSubClient client(espClient);

static char strTemperature[10] = {0};
static char strHumidity[10] = {0};
static char strUltra[10] = {0};
// setting variabel hitung
unsigned long waktu = 0;
double jarak = 0;

DHTesp dhtSensor;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LAMPU_PIN, OUTPUT);

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  WiFi.mode(WIFI_STA);
  WiFi.begin("Wokwi-GUEST", "", 6);
  // Wait for WiFi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  client.setServer("test.mosquitto.org", 1883);
  client.setCallback(callback);
}

float sensorUltra(void){
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(100);
  digitalWrite(TRIG, LOW);
  // hitung waktu data
  waktu = pulseIn(ECHO, HIGH);  
  jarak = waktu / 58;
  // Serial.println(waktu/58);
  return jarak;
}

float getTemperature(void)
{
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  if (!(isnan(data.temperature)))
    return data.temperature;
  else
    return -99.99;
}


float getHumidity(void)
{
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  if (!(isnan(data.humidity)))
    return data.humidity;
  else
    return -99.99;
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String stMessage;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    stMessage += (char)message[i];
  }
  Serial.println();
  if (String(topic) == "bolabot/v1/led") {
    Serial.print("Changing output to ");
    if(stMessage == "ON"){
      Serial.println("on");
      digitalWrite(LAMPU_PIN, HIGH);
    }
    else if(stMessage == "OFF"){
      Serial.println("off");
      digitalWrite(LAMPU_PIN, LOW);
    }
  }
  pompa(topic, stMessage);
}

void mqttReconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientId)) {
      Serial.println(" connected");
      client.subscribe("bolabot/v1/+");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void pompa(char* topic, String stMessage){
  if (String(topic) == "bolabot/v1/led") {
    Serial.print("Changing output to ");
    if(stMessage == "ON" ){
      Serial.println("ON");
      digitalWrite(POMPA_PIN, HIGH);
    }
    else if(stMessage == "OFF"){
      Serial.println("OFF");
      digitalWrite(POMPA_PIN, LOW);
    }
  }
}

void loop() {
  if (!client.connected()) {
    mqttReconnect();
  }
  client.loop();
  TempAndHumidity  data = dhtSensor.getTempAndHumidity();
  sprintf(strTemperature, "%.2f", getTemperature());
  sprintf(strHumidity, "%.2f", getHumidity());
  sprintf(strUltra, "%.2f", sensorUltra());
  client.publish("bolabot/v1/suhu", strTemperature);
  client.publish("bolabot/v1/kelembapan", strHumidity);
  client.publish("bolabot/v1/tanah", strUltra);

  delay(1000);
}
