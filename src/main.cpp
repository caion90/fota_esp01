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
  // log("Message arrived");
  // log(topic[0]);
  // log((char)payload[0]);
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1')
  {
    digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
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
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      //     log("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      //    log(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);

  /* switch on led */
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  //  log("Booting");
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    WiFi.begin(ssid, password);
    //  log("Retrying connection...");
  }
  /* switch off led */
  digitalWrite(LED_BUILTIN, HIGH);

  /* configure dimmers, and OTA server events */
  analogWriteRange(1000);
  analogWrite(LED_BUILTIN, 990);

  for (int i = 0; i < N_DIMMERS; i++)
  {
    pinMode(dimmer_pin[i], OUTPUT);
    analogWrite(dimmer_pin[i], 50);
  }

  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart([]() { // switch off all the PWMs during upgrade
    for (int i = 0; i < N_DIMMERS; i++)
    {
      analogWrite(dimmer_pin[i], 0);
    }
    analogWrite(LED_BUILTIN, 0);
  });

  ArduinoOTA.onEnd([]() { // do a fancy thing with our board led at end
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

  /* setup the OTA server */
  ArduinoOTA.begin();
  wifiServer.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //  log("Ready");
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
        // char received = server.read();
        // server.write(received);
        char received = server.read();
        server.write(received);
      }

      delay(10);
    }

    server.stop();
    //   log("Client disconnected");
  }

  //  if (!client.connected())
  //  {
  //    reconnect();
  //  }

  // if (Update_Loop_Count < 30000)
  // {
  //   delay(1);
  ArduinoOTA.handle();
  //   Update_Loop_Count++;
  // }

  // client.loop();
}