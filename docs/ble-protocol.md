# BLE protocol

Advertised name: `MidTable`. Primary advertising packet carries the service
UUID below (the BT remote matches on it), the name is in the scan response.

## Turntable protocol

Same characteristics as `~/projects/ali-turn-table-upgrade/docs/ble-protocol.md`,
but under its own service UUID: it's a different table, the remote registers it
as a separate device (`midTableDevice` in
`xiao-remote-slider-control/src/devices/turntable_device.cpp`).

Service `7ab1e101-0000-4000-8000-00000000cafe`

| Char | UUID | Props | Payload |
|---|---|---|---|
| Command | `7ab1e002-…cafe` | WRITE / WRITE_NR | 1 byte: `F` forward, `B` backward, `S` stop, `R` clear battery cutoff; service: `O` WiFi+OTA on, `o` off, `T` re-init TMC, `Z` buzzer test |
| Speed | `7ab1e003-…cafe` | WRITE / WRITE_NR | 1 byte: preset index, `SPEED_PRESETS_DPS` in `config.h` (2/3/4.5/6/9/12/24/45 deg/s = 180/120/80/60/40/30/15/8 s/rev) |
| Status | `7ab1e004-…cafe` | READ / NOTIFY | 10 bytes, 5 Hz, see below |

Status: `state` (0 stopped, 1 running, 2 OTA mode, 3 battery cutoff), `dir` (0 fwd, 1 rev),
`speedIdx` (nearest preset), `speedCount`, `battPct`, `flags` (bit0 low, bit1 cutoff,
bit2 TMC UART ok, bit3 OTA ready), bytes 6..9 IPv4 `a.b.c.d` while OTA is ready, else 0.
The first 6 bytes are the old table's layout.

WiFi is off by default (one radio with BLE). `O` / `w` connects and starts ArduinoOTA
at `mid-table.local`; it turns itself off after 25 s without WiFi or 5 min without an
upload. Both LEDs breathe in antiphase while in OTA mode.

## Nordic UART Service — text commands for debugging

Service `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`, RX `…0002` (write), TX `…0003` (notify).
Same commands as USB serial, one per write or newline-separated; send anything
unknown for help.
