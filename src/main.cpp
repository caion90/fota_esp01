#include <ESP8266WiFi.h>
#include <WiFiServer.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#ifndef STASSID
#define STASSID "Internet 2.4G"
#define STAPSK "internet"
#endif

WiFiClient espClient;
WiFiServer wifiServer(2222);
PubSubClient client(espClient);
#define MSG_BUFFER_SIZE (50)
unsigned long lastMsg = 0;
char msg[MSG_BUFFER_SIZE];
int value = 0;

const char *ssid = STASSID;
const char *password = STAPSK;
const char *host = "OTA-LEDS";
const char *mqtt_server = "caio-ipmh61r1.local";

int led_pin = 13;
#define N_DIMMERS 3
int dimmer_pin[] = {14, 5, 15};

void callback(char *topic, byte *payload, unsigned int length)
{
  if ((char)payload[0] == '1')
  {
    digitalWrite(LED_BUILTIN, LOW);
  }
  else
  {
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
  }
}

void log(char logmsg)
{
  if (wifiServer.hasClient())
  {
    wifiServer.write(logmsg);
  }
  return;
}

void reconnect()
{
   while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str()))
    {
      client.publish("outTopic", "hello world");
      client.subscribe("inTopic");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    WiFi.begin(ssid, password);
  }

  digitalWrite(LED_BUILTIN, HIGH);


  analogWriteRange(1000);
  analogWrite(LED_BUILTIN, 990);

  for (int i = 0; i < N_DIMMERS; i++)
  {
    pinMode(dimmer_pin[i], OUTPUT);
    analogWrite(dimmer_pin[i], 50);
  }

  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart([]() {
    for (int i = 0; i < N_DIMMERS; i++)
    {
      analogWrite(dimmer_pin[i], 0);
    }
    analogWrite(LED_BUILTIN, 0);
  });

  ArduinoOTA.onEnd([]() {
    for (int i = 0; i < 30; i++)
    {
      analogWrite(LED_BUILTIN, (i * 100) % 1001);
      delay(50);
    }
  });

  ArduinoOTA.onError([](ota_error_t error)
                     {
    (void)error;
    ESP.restart(); });

  
  ArduinoOTA.begin();
  wifiServer.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

int Update_Loop_Count = 0;

void loop()
{
  WiFiClient server = wifiServer.available();

  if (server)
  {

    while (server.connected())
    {

      while (server.available() > 0)
      {
        char received = server.read();
        server.write(received);
      }

      delay(10);
    }

    server.stop();
  }

  ArduinoOTA.handle();
}

}
