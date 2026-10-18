# Otter VerteilerBaustein — Firmware

ESP32 firmware for the Otter model railroad control system. The VerteilerBaustein (distribution building block) connects up to 5 plug-in modules and controls them via WiFi and MQTT.

## Supported module types

Up to one module type may be active per slot (0–4). Enabling more than one type in the same slot is detected at startup and logged as a fatal error.

| Module | Channels | MQTT topics |
|--------|----------|-------------|
| **TM** – Button module  | 4 per module (up to 20 total) | `otter/TM/0` … `otter/TM/19` |
| **SM** – Signal module  | 2 per module (up to 8 total)  | `otter/SM/0` … `otter/SM/7`  |
| **WM** – Turnout module | 2 per module (up to 8 total)  | `otter/WM/0` … `otter/WM/7`  |

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

Settings are persisted in NVS and can be changed at runtime via the **serial configuration console** (115200 baud). Compiled-in defaults are defined in [main/settings.cpp](main/settings.cpp).

| Setting | Description |
|---------|-------------|
| `ssid` / `password` | WiFi credentials |
| `mqtt_server` | MQTT broker IP address |
| `client_name` | MQTT client ID |
| `AbschNummer` | Third octet of the static IP |
| `VerteilerBaustein` | Fourth octet of the static IP |
| `networkByte1` / `networkByte2` | First two octets of the network address |
| `gatewayByte3` / `gatewayByte4` | Last two octets of the default gateway |
| `AusSchaltZeitWeiche` | Turnout pulse-off delay in ms (default: 50) |
| `TM_active[n]` / `SM_active[n]` / `WM_active[n]` | Enable a module type for slot *n* |

Static IP format: `networkByte1.networkByte2.AbschNummer.VerteilerBaustein`

## Project structure

```
main/
├── main.cpp              – Entry point; WiFi & MQTT initialisation
├── tm.cpp / tm.h         – Button module (TM) logic
├── sm.cpp / sm.h         – Signal module (SM) logic
├── wm.cpp / wm.h         – Turnout module (WM) logic
├── settings.cpp / .h     – NVS-backed configuration store
├── serial_config.cpp / .h – Serial configuration console
└── otter.cpp / otter.h   – Shared pin mapping & MQTT topic definitions
```
