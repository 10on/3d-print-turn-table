.PHONY: all build upload flash monitor clean check test-motor test-uart

# Default: build + OTA upload (HTTP POST /update). Put the table in OTA mode
# first: remote's Service menu, or "w" over USB serial / NUS.
# mDNS is flaky while BLE keeps WiFi in modem sleep: make upload OTA_HOST=<ip from the remote>
export OTA_HOST ?= mid-table.local
all: upload

build:
	pio run

upload:
	pio run -e ota -t upload

# USB flash — first flash or recovery only
flash:
	pio run -e usb -t upload

monitor:
	pio device monitor --filter esp32_exception_decoder

clean:
	pio run -t clean

check:
	pio check --fail-on-defect=high

# Bring-up: flash the +-90 deg swing test over USB and open the monitor
test-motor:
	pio run -e motor_test -t upload
	sleep 2
	pio device monitor --filter esp32_exception_decoder

# Bring-up: flash the raw TMC2208 UART check over USB and open the monitor
test-uart:
	pio run -e uart_test -t upload
	sleep 2
	pio device monitor
