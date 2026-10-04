#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "config.h"

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

bool stableState = false;
bool lastRawState = false;
unsigned long lastRawChange = 0;
unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;

String topicState;
String topicEvent;
String topicAvailability;

bool readSensor()
{
    return digitalRead(SENSOR_GPIO) == SENSOR_ACTIVE_LEVEL;
}

void publishState()
{
    const char* payload = stableState ? "ON" : "OFF";
    mqttClient.publish(topicState.c_str(), payload, RETAIN_STATE);
}

void publishSweepEvent()
{
    mqttClient.publish(topicEvent.c_str(), "SWEEP", false);
}

void updateSensor()
{
    const bool rawState = readSensor();

    if (rawState != lastRawState)
    {
        lastRawState = rawState;
        lastRawChange = millis();
    }

    if ((millis() - lastRawChange) >= SENSOR_DEBOUNCE_MS &&
        stableState != rawState)
    {
        const bool previousState = stableState;
        stableState = rawState;

        publishState();

        // Publish an event only when the sensor becomes active.
        if (!previousState && stableState)
        {
            publishSweepEvent();
        }

        Serial.print("Sensor state: ");
        Serial.println(stableState ? "ON" : "OFF");
    }
}

void connectWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    if (millis() - lastWifiAttempt < 10000)
    {
        return;
    }

    lastWifiAttempt = millis();

    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectMQTT()
{
    if (WiFi.status() != WL_CONNECTED || mqttClient.connected())
    {
        return;
    }

    if (millis() - lastMqttAttempt < 5000)
    {
        return;
    }

    lastMqttAttempt = millis();

    const String clientId =
        "hand-sweep-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    Serial.print("Connecting to MQTT: ");
    Serial.println(MQTT_HOST);

    bool connected = false;

    if (strlen(MQTT_USERNAME) > 0)
    {
        connected = mqttClient.connect(
            clientId.c_str(),
            MQTT_USERNAME,
            MQTT_PASSWORD,
            topicAvailability.c_str(),
            0,
            true,
            "offline"
        );
    }
    else
    {
        connected = mqttClient.connect(
            clientId.c_str(),
            topicAvailability.c_str(),
            0,
            true,
            "offline"
        );
    }

    if (!connected)
    {
        Serial.print("MQTT connection failed, state=");
        Serial.println(mqttClient.state());
        return;
    }

    Serial.println("MQTT connected.");

    mqttClient.publish(topicAvailability.c_str(), "online", true);

    if (PUBLISH_INITIAL_STATE)
    {
        publishState();
    }
}

void setup()
{
    Serial.begin(115200);
    delay(100);

    pinMode(SENSOR_GPIO, INPUT_PULLUP);

    topicState = String(MQTT_BASE_TOPIC) + "/state";
    topicEvent = String(MQTT_BASE_TOPIC) + "/event";
    topicAvailability = String(MQTT_BASE_TOPIC) + "/availability";

    stableState = readSensor();
    lastRawState = stableState;

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);

    Serial.println();
    Serial.println("ESP32 Hand Sweep MQTT Sensor");
    Serial.println("----------------------------");
    Serial.print("Sensor GPIO: ");
    Serial.println(SENSOR_GPIO);

    connectWiFi();
}

void loop()
{
    connectWiFi();
    connectMQTT();

    if (mqttClient.connected())
    {
        mqttClient.loop();
    }

    updateSensor();

    delay(5);
}
