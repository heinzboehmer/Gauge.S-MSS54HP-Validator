#include <Arduino.h>
#include <DS2.h>

#include "config.h"

// Anything above this level will not get printed.
#define DEBUG_LEVEL DEBUG_INFO

constexpr size_t requestSize = 5;
uint8_t dmeRequest[requestSize] = {0x12, 0x05, 0x0B, 0x03, 0x1F};
uint8_t readData[255] = {0};
constexpr uint8_t responseBytesBeforeData = 3;
constexpr uint8_t mss54hpResponseDataStartOffset = requestSize + responseBytesBeforeData;
constexpr uint8_t mss54hpTpsOffset = mss54hpResponseDataStartOffset + 23;
constexpr float mss54hpTpsMultiplier = 0.1;
DS2 ds2(Serial2);

// Hacky tiered debug prints cause too lazy to write a nice class implementation.
void debugPrint(DebugLevel debugLevel, const char* formatStr, ...) {
	if (DEBUG_LEVEL < debugLevel) return;

	char buffer[256];
	va_list args;
	va_start(args, formatStr);
	vsnprintf(buffer, 256, formatStr, args);
	Serial.print(buffer);
	va_end(args);
}

void setup() {
	// Set up debug prints.
	Serial.begin(115200);
	while(!Serial);

	// Set up communication with TJA1021.
	Serial2.begin(9600, SERIAL_8E1, SERIAL_KLINE_RX, SERIAL_KLINE_TX);
	while(!Serial2);

	ds2.setBlocking(false);
	ds2.setKwp(false);

	debugPrint(DEBUG_INFO, "%s", "Setup finished\n");
}

void loop() {
	// Clear RX buffer and DS2 lib flags.
	ds2.newCommand();


	debugPrint(DEBUG_VERBOSE_INFO, "%s", "Contents of dmeRequest[]: ");
	for (int i = 0; i < requestSize; i++) {
		debugPrint(DEBUG_VERBOSE_INFO, "0x%.2X ", dmeRequest[i]);
	}
	debugPrint(DEBUG_VERBOSE_INFO, "%s", "\n");

	// Send request to DME.
	uint8_t sendSize = ds2.sendCommand(dmeRequest);
	if (sendSize != requestSize) {
		debugPrint(DEBUG_ERROR, "Unexpected sendSize: %d\n", sendSize);
		return;
	}

	// Wait for response from DME and validate.
	ReceiveType receiveStatus = RECEIVE_WAITING;
	while (receiveStatus == RECEIVE_WAITING) {
		debugPrint(DEBUG_VERBOSE_INFO,"%s", "Waiting for response...\n");
		receiveStatus = ds2.receiveData(readData);
	}

	if (receiveStatus == RECEIVE_OK) {
		debugPrint(DEBUG_VERBOSE_INFO,"%s", "Received valid data\n");

		// Extract TPS value from valid DME response and print to console.
		int16_t throttlePedalValue =
				((readData[mss54hpTpsOffset] & 0xFF) << 8) |
				(readData[mss54hpTpsOffset + 1] & 0xFF);
		debugPrint(DEBUG_INFO, "Throttle Pedal: %.1f%%\n", throttlePedalValue * mss54hpTpsMultiplier);
	} else if (receiveStatus == RECEIVE_TIMEOUT) {
		debugPrint(DEBUG_INFO,"%s", "Timed out waiting for receiveData()\n");
	} else if (receiveStatus == RECEIVE_BAD) {
		debugPrint(DEBUG_ERROR,"%s", "Received bad data\n");
	}

	debugPrint(DEBUG_VERBOSE_INFO, "%s", "Contents of readData[]: ");
	for (int i = 0; i < 255; i++) {
		debugPrint(DEBUG_VERBOSE_INFO, "0x%.2X ", readData[i]);
	}
	debugPrint(DEBUG_VERBOSE_INFO, "%s", "\n");	

	delay(100);
}
