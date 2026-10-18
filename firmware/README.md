# Otter WeichenBaustein — Firmware

ESP32 firmware for the Otter model railroad control system. The WeichenBaustein (turnout building block) drives up to 8 solenoid turnouts (Weichen) via four SN754410 H-bridges and is controlled over WiFi and MQTT.

## Turnout control

Each turnout listens on its own MQTT topic. The payload selects the direction; the coil is energised for `AusSchaltZeitWeiche` ms and then switched off again.

| Topic (default) | Payload | Action |
|-----------------|---------|--------|
| `otter/WM/0` … `otter/WM/7` | `0` | Pulse turnout in direction 0 |
| | `1` | Pulse turnout in direction 1 |

Any other payload is ignored and logged as a warning.

### Pin mapping

| Turnout | GPIOs | H-bridge | Terminal J2 |
|---------|-------|----------|-------------|
| 0 | 4, 5   | U11 (1A, 2A) | 1–2   |
| 1 | 13, 14 | U11 (3A, 4A) | 3–4   |
| 2 | 16, 17 | U21 (1A, 2A) | 5–6   |
| 3 | 18, 19 | U21 (3A, 4A) | 7–8   |
| 4 | 21, 22 | U31 (1A, 2A) | 9–10  |
| 5 | 23, 25 | U31 (3A, 4A) | 11–12 |
| 6 | 26, 27 | U41 (1A, 2A) | 13–14 |
| 7 | 32, 33 | U41 (3A, 4A) | 15–16 |

## Requirements

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/) v6.x
- Target: ESP32
- IDF components (resolved automatically via `idf_component.yml`):
  - `espressif/mqtt ^1.0.0`
  - `espressif/cjson >=1.0.0~1`

## Build & flash

```sh
# Source the IDF environment (adjust path to your installation)
. ~/esp/esp-idf/export.sh

# First-time setup — set the target chip
idf.py set-target esp32

# Build, flash, and open the serial monitor
idf.py build flash monitor
```

## Configuration

Settings are persisted in NVS. Compiled-in defaults are defined in [main/settings.cpp](main/settings.cpp).

| Setting | Default | Description |
|---------|---------|-------------|
| `ssid` / `password` | `SSID` / `PASSWORD` | WiFi credentials (serial `wifi` command only) |
| `mqtt_server` | `10.1.0.5` | MQTT broker IP address |
| `client_name` | `test_client` | MQTT client ID — must be unique per board |
| `networkByte1` / `networkByte2` | `10` / `1` | First two octets of the network address |
| `AbschNummer` | `1` | Third octet of the static IP |
| `WeichenBaustein` | `1` | Fourth octet of the static IP |
| `gatewayByte3` / `gatewayByte4` | `0` / `1` | Last two octets of the default gateway |
| `AusSchaltZeitWeiche` | `200` | Turnout pulse length in ms |
| `MqttWMW` | `otter/WM/0` … `otter/WM/7` | Array of 8 turnout topics |
| `MqttSet` | `otter/Set` | Topic for settings updates via MQTT |

Static IP: `networkByte1.networkByte2.AbschNummer.WeichenBaustein`, netmask `255.255.0.0`.
Gateway: `networkByte1.networkByte2.gatewayByte3.gatewayByte4`.

Changes are saved immediately but take effect after a restart.

### Serial console

115200 baud on the USB port of the ESP32 dev board.

| Command | Description |
|---------|-------------|
| `get` | Print all settings as JSON (password masked) |
| `set {JSON}` | Update any of the non-WiFi settings above, save to NVS |
| `wifi {"ssid":"x","password":"y"}` | Update WiFi credentials, save, restart |
| `restart` | Restart the device |
| `reset` | Erase saved settings, restart with defaults |
| `help` | Show command overview |

Example:

```
set {"AbschNummer":3,"WeichenBaustein":12,"client_name":"weichen_3_12"}
```

### MQTT

Publish the same JSON as for `set` to `MqttSet` (default `otter/Set`). Two extra keys are supported:

- `"tx": "<id>"` — echoed back in the acknowledgement
- `"reboot": true` — restart after saving so the changes apply immediately

The acknowledgement is published (retained) to `<MqttSet>/ack`:

```json
{"tx":"<id>"}
{"tx":"<id>","error":"JSON parse error"}
```

WiFi credentials cannot be changed via MQTT.

## Project structure

```
main/
├── main.cpp               – Entry point; WiFi & MQTT setup, main loop
├── wm.cpp / wm.h          – Turnout (WM) pulse logic
├── otter.cpp / otter.h    – GPIO pin mapping
├── settings.cpp / .h      – NVS-backed configuration store
└── serial_config.cpp / .h – Serial configuration console
```
