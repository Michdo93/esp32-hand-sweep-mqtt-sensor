#pragma once

// Wi-Fi configuration
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// MQTT broker configuration
#define MQTT_HOST       "192.168.0.5"
#define MQTT_PORT       1883
#define MQTT_USERNAME   ""
#define MQTT_PASSWORD   ""

// MQTT topic prefix
#define MQTT_BASE_TOPIC "sensors/hand-sweep-01"

// ESP32 GPIO connected to the optocoupler output
#define SENSOR_GPIO     27

// The optocoupler pulls the GPIO LOW when the sensor is active.
#define SENSOR_ACTIVE_LEVEL LOW

// Debounce time in milliseconds
#define SENSOR_DEBOUNCE_MS 50

// Publish the current state immediately after MQTT connection.
#define PUBLISH_INITIAL_STATE true

// Publish state as a retained MQTT message.
#define RETAIN_STATE true
