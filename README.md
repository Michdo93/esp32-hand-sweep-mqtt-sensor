# ESP32 Hand Sweep MQTT Sensor

Turn an inexpensive 5–24 V infrared hand-sweep switch module into a Wi-Fi/MQTT sensor.

The ESP32 monitors the **switched output** of the hand-sweep module and publishes its state and edge events over MQTT. This is useful for smart-home projects where the original module is normally used to switch a cabinet or wardrobe LED strip.

> **Important:** The inexpensive hand-sweep modules sold under names such as `XK-GK-4010A` are generally **power switches**, not 3.3 V logic sensors. Their output is intended to switch the connected DC load. Do not connect a 5–24 V output directly to an ESP32 GPIO. Use the recommended optocoupler interface or an appropriate voltage divider/level shifter.

## Features

- ESP32 Wi-Fi connectivity
- MQTT 3.x using PubSubClient
- Configurable Wi-Fi SSID/password
- Configurable MQTT broker, port, username and password
- Configurable MQTT base topic
- MQTT availability / Last Will
- Home Assistant-friendly MQTT state and event topics
- Debounced sensor input
- Publishes an event whenever the sensor changes from inactive to active
- Optional retained state
- Automatic Wi-Fi and MQTT reconnect
- No external web server or cloud service required
- Configuration stored in a separate header file

## Tested concept

The project is designed around the common `XK-GK-4010A` style hand-sweep module:

- Supply: typically 5–24 VDC depending on the exact version
- Output: switched DC supply voltage
- Typical detection range: approximately 10–80 mm depending on version and installation
- Some versions support a toggle/trigger mode
- Some versions provide an output capable of several amps for an LED load

Exact modules sold by different vendors can differ. **Verify the markings and wiring on your actual PCB before connecting anything.**

## Recommended architecture

```text
             +-----------------------+
             |  5 V DC power supply  |
             +-----------+-----------+
                         |
              +----------+----------+
              |                     |
              v                     v
       +-------------+       +-------------+
       | Hand Sweep  |       | ESP32 Dev   |
       | Sensor      |       | Board       |
       | Module      |       |             |
       |             |       | Wi-Fi       |
       | VCC   GND   |       | MQTT        |
       | OUT+  OUT-  |       | GPIO <------+
       +------+------+       +-------------+
              |
              | switched 5 V
              v
       +--------------+
       | Optocoupler  |
       | PC817 /      |
       | similar      |
       +------+-------+
              |
              | isolated GPIO signal
              v
          ESP32 GPIO
              |
              | Wi-Fi
              v
          MQTT Broker
```

## Why an optocoupler?

The sensor module switches a supply voltage intended for a load. An ESP32 GPIO is a 3.3 V logic input and must not be exposed to 5–24 V.

The optocoupler gives the ESP32 a simple, robust digital signal and avoids relying on the exact output voltage of a particular sensor module.

A PC817-class optocoupler is sufficient for this slow hand-sweep application.

## Bill of materials

### Required

| Part | Quantity | Notes |
|---|---:|---|
| ESP32 development board | 1 | ESP32-WROOM DevKit or similar |
| Hand sweep sensor module | 1 | XK-GK-4010A-style 5–24 V module |
| 5 V DC regulated power supply | 1 | Use a suitable, certified supply |
| PC817 optocoupler | 1 | DIP-4 or equivalent |
| 1 kΩ resistor | 1 | Optocoupler LED current limiting |
| 10 kΩ resistor | 1 | ESP32 GPIO pull-up |
| Breadboard / PCB | 1 | For prototyping |
| Jumper wires | as needed | Suitable for low-voltage DC |

### Optional

| Part | Purpose |
|---|---|
| 100 nF capacitor | Additional GPIO noise filtering |
| Fuse | Recommended for permanent installations |
| Screw terminals | Cleaner permanent wiring |
| Enclosure | Protects the electronics |
| Status LED + 1 kΩ resistor | Local status indication |
| 5 V buck converter | If using a higher-voltage DC supply |

## Power recommendation

For the first prototype, use **5 VDC**.

This makes the interface simple:

- Sensor module supply: 5 V
- ESP32 supply: 5 V input / USB or 5V/VIN pin, depending on the development board
- Sensor switched output: approximately 5 V
- Optocoupler LED: driven through 1 kΩ

Do not use an unknown 12/24 V supply until you have verified the exact sensor board and its wiring.

## Sensor wiring

The exact connector layout varies. A common XK-GK-4010A-style module has:

- `VCC` / `IN+` = positive supply
- `GND` / `IN-` = supply ground
- `OUT+` = switched positive output
- `OUT-` = switched negative/output return

Some versions label these differently. **Use the PCB markings and the documentation supplied with your module.**

For a typical module:

```text
5 V PSU +  -------------------- VCC / IN+
5 V PSU -  -------------------- GND / IN-

OUT+  ---- 1 kΩ ---->| PC817 LED ---- OUT-
                         |
                    optocoupler
```

The optocoupler LED is powered only when the sensor output is active.

## ESP32 / PC817 wiring

PC817 DIP-4 pinout is commonly:

```text
          PC817
       +---------+
 LED 1 | o     4 | Collector
 LED 2 | o     3 | Emitter
       +---------+
```

Connect:

```text
PC817 pin 4 (collector) ---- ESP32 GPIO27
                           |
                          10 kΩ
                           |
                          3.3 V

PC817 pin 3 (emitter) ------ ESP32 GND
```

The ESP32 GPIO is therefore pulled HIGH when the optocoupler is off and pulled LOW when the sensor output activates the optocoupler.

The firmware consequently uses:

```text
GPIO LOW  = sensor active
GPIO HIGH = sensor inactive
```

If your optocoupler has a different pinout, use its datasheet instead of the drawing above.

## Complete wiring diagram

See [`docs/wiring.svg`](docs/wiring.svg).

A compact representation:

```text
                      +5 V
                       |
             +---------+----------+
             |                    |
             |                    |
        ESP32 5V/VIN          Sensor VCC
             |                    |
             |              +-----+------+
             |              | Hand Sweep |
             |              |   Sensor   |
             |              +-----+------+
             |                    |
             |                 OUT+
             |                    |
             |                  1 kΩ
             |                    |
             |                  LED
             |               +---->|----+
             |               |          |
             |               |        OUT-
             |               |
             |             PC817
             |               |
             |       collector +---- GPIO27
             |                    |
             |                   10k
             |                    |
             +------------------ 3.3V

ESP32 GND ---------------- PC817 emitter
Sensor GND --------------- PSU GND
ESP32 GND ---------------- PSU GND
```

The sensor's output is **not** connected directly to GPIO27.

## MQTT topics

Default base topic:

```text
sensors/hand-sweep-01
```

Published topics:

```text
sensors/hand-sweep-01/state
sensors/hand-sweep-01/event
sensors/hand-sweep-01/availability
```

Examples:

```text
state       -> ON
state       -> OFF
event       -> SWEEP
availability-> online
availability-> offline
```

The availability topic uses MQTT Last Will and Testament.

## Home Assistant

The project does not require Home Assistant, but it can be integrated easily.

Example MQTT sensor:

```yaml
mqtt:
  binary_sensor:
    - name: "Hand Sweep Sensor"
      unique_id: "hand_sweep_sensor_01"
      state_topic: "sensors/hand-sweep-01/state"
      payload_on: "ON"
      payload_off: "OFF"
      availability_topic: "sensors/hand-sweep-01/availability"
      payload_available: "online"
      payload_not_available: "offline"
      device_class: "occupancy"
```

For a momentary automation trigger, subscribe to:

```text
sensors/hand-sweep-01/event
```

and react to:

```text
SWEEP
```

## Configuration

Copy:

```text
include/config.example.h
```

to:

```text
include/config.h
```

Then edit the values.

Example:

```cpp
#define WIFI_SSID       "MyWiFi"
#define WIFI_PASSWORD   "MyWiFiPassword"

#define MQTT_HOST       "192.168.0.5"
#define MQTT_PORT       1883
#define MQTT_USERNAME   ""
#define MQTT_PASSWORD   ""

#define MQTT_BASE_TOPIC "sensors/hand-sweep-01"
```

`config.h` is ignored by Git so that credentials are not accidentally committed.

## Build with PlatformIO

Install PlatformIO and run:

```bash
pio run
```

Upload:

```bash
pio run --target upload
```

Serial monitor:

```bash
pio device monitor
```

Default baud rate:

```text
115200
```

## Arduino IDE

The source is also a normal Arduino sketch.

Install:

- ESP32 board package
- `PubSubClient` library

Open:

```text
src/main.cpp
```

or copy it into an Arduino sketch.

## Input behavior

The firmware debounces the sensor input.

A transition from inactive to active generates:

```text
event = SWEEP
```

The current state is published separately:

```text
state = ON
state = OFF
```

This means the project works well with both:

1. A sensor module configured as a toggle switch.
2. A sensor module that produces a temporary active state.

## Sensor-module operating mode

Some XK-GK-4010A boards have a jumper or solder bridge that selects the operating mode.

A common configuration is:

- Toggle/trigger mode: first sweep activates the output, second sweep deactivates it.
- Sensor mode: output follows the presence of an object.

The exact jumper designation and behavior depend on the board revision. Do not assume that every Amazon module has the same jumper configuration.

## Safety

This project is intended for **low-voltage DC only**.

Do not connect:

- 230 VAC
- mains voltage
- an unknown power supply
- an unverified sensor output

to the ESP32.

Use a properly rated, isolated DC power supply.

The sensor module may be capable of switching several amps, but this project only uses its output as a sensing signal. Do not use the ESP32 GPIO to switch the LED load directly.

## Troubleshooting

### ESP32 always reports OFF

Check:

1. Sensor module has power.
2. Sensor output actually changes when waving your hand.
3. PC817 LED polarity.
4. PC817 pinout.
5. GPIO number in `config.h`.
6. Common ground is present for the ESP32 side.

### ESP32 always reports ON

Check:

1. PC817 transistor pinout.
2. 10 kΩ pull-up resistor.
3. GPIO configuration.
4. Whether the sensor output is active immediately after startup.

### MQTT does not connect

Check:

1. Wi-Fi SSID/password.
2. MQTT broker IP address.
3. MQTT port.
4. Firewall rules.
5. MQTT credentials.
6. Whether the broker accepts unauthenticated connections.

## Project structure

```text
esp32-hand-sweep-mqtt-sensor/
├── .gitignore
├── LICENSE
├── README.md
├── platformio.ini
├── include/
│   └── config.example.h
├── src/
│   └── main.cpp
└── docs/
    └── wiring.svg
```

## License

MIT License. See [`LICENSE`](LICENSE).
