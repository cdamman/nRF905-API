#ifdef ARDUINO_ARCH_ESP8266

#include <EEPROM.h>
#include <SPI.h>
#include "../../board.h"	// board-> functions
#include "../../nvram.h"	// NVRAMBuffer
#include "../../utils.h"	// serialPrintHex()
#include "esp8266.h"

// Create an instance of the server
#if NRF905API_HTTPS == 1
BearSSL::ESP8266WebServerSecure server(DEFAULT_HTTPS_PORT);
#else
ESP8266WebServer server(DEFAULT_HTTP_PORT);
#endif

ADC_MODE(ADC_VCC);	// Configure ADC to measure Vcc

/* ESP8266 Class functions */
Board::Board(void) {
	pinMode(PIN_LED, OUTPUT);
}

Board::~Board(void) {
}

const String Board::get_arch(void) {
	return "esp8266";
}

void Board::reset(void) {
	ESP.restart();
}

const String Board::restartReason(void) {
	return "{\"cpu0\":\"" + ESP.getResetReason() + "\"}";
}

const uint32_t Board::getCPUFreqMhz(void) {
	return (ESP.getCpuFreqMHz() * 1000000);
}

const char * Board::getSdkVersion(void) {
	return ESP.getSdkVersion();
}

void Board::setADCtoVccMode(void) {
}

const uint16_t Board::get_vcc(void) {
	return ESP.getVcc();
}

const uint32_t Board::get_cpu_id(void) {
	return ESP.getChipId();
}

const uint32_t Board::get_flash_chip_id(void) {
	return ESP.getFlashChipId();
}

const uint32_t Board::getFlashChipSpeed(void) {
	return ESP.getFlashChipSpeed();
}

const bool Board::checkFlashCRC(void) {
//	return ESP.checkFlashCRC();
	return true;
}

const char * Board::getFlashMode(void) {
	FlashMode_t mode [[gnu::unused]] = ESP.getFlashChipMode();

	return (mode == FM_QIO ? "QIO" : mode == FM_QOUT ? "QOUT" : mode == FM_DIO ? "DIO" : mode == FM_DOUT ? "DOUT" : "UNKNOWN");
}

const uint32_t Board::get_flash_chip_real_size(void) {
	return ESP.getFlashChipRealSize();
}

const uint32_t Board::getFlashChipSdkSize(void) {
	return ESP.getFlashChipSize();
}

const uint32_t Board::getSketchSize(void) {
	return ESP.getSketchSize();
}

const uint32_t Board::getFreeSketchSpace(void) {
	return ESP.getFreeSketchSpace();
}

const String Board::getSketchMD5(void) {
	return ESP.getSketchMD5();
}

const uint32_t Board::getFreeHeap(void) {
	return ESP.getFreeHeap();
}

const uint32_t Board::getHeapFragmentation(void) {
	return ESP.getHeapFragmentation();
}

const uint32_t Board::getHeapMaxFreeBlockSize(void) {
	return ESP.getMaxFreeBlockSize();
}

const String Board::get_core_version(void) {
	return ESP.getCoreVersion();
}

const uint32_t Board::get_core_revision(void) {
#ifdef ARDUINO_ESP8266_GIT_VER
	return ARDUINO_ESP8266_GIT_VER;
#else
	return "unknown";
#endif
}

String Board::get_hostname(void) {
	return WiFi.hostname();
}

void Board::set_hostname(const char *hostname) {
	WiFi.hostname(hostname);
}

String Board::get_localIPv6(void) {
	return "n.a.";	// TODO
}

bool Board::enable_IPv6(void) {
	return false;
}

void Board::setPinMode(const uint8_t pin, const uint8_t mode) {
	pinMode(pin, mode);
}

void Board::writePin(const uint8_t pin, const uint8_t value) {
	digitalWrite(pin, value);
}

uint8_t Board::readPin(const uint8_t pin) {
	if (pin == PIN_CD)			// TODO: temporary fix because I haven't found a suitable pin for CD to connect to. SSD2 or SSD3 don't work.
		return LOW;
	else
		return digitalRead(pin);
}

bool Board::getOnBoardLED(void) {
	return _led;
}

void Board::setOnBoardLED(const bool value) {
	this->_led = value;

	if (value == true)
		digitalWrite(PIN_LED, LOW);
	else
		digitalWrite(PIN_LED, HIGH);
}

bool Board::SPIBegin(const uint8_t mosi, const uint8_t miso, const uint8_t clk, const uint8_t cs) {
	this->_spi_cs = cs;
	this->_spi_cs_active = LOW;
	this->_spi = new SPIClass();
	this->_spi->pins(clk, miso, mosi, cs);
//	this->_spi->begin();
	SPI.begin();

	return true;
}

void Board::SPIEnd(void) {
	SPI.end();
//	this->_spi->end();
	free(this->_spi);
}

void Board::SPISetChipSelectPolarity (const uint8_t cs, const uint8_t active) {
	this->_spi_cs_active = active;
	if (this->_spi_cs_active == LOW)
		digitalWrite(this->_spi_cs, HIGH);
	else
		digitalWrite(this->_spi_cs, LOW);
}

void Board::SPISetBitOrder(const uint8_t order) {
	this->_spi_bitorder = MSBFIRST;		// TODO: use order and not fixed value
	SPI.setBitOrder(this->_spi_bitorder);

}

void Board::SPISetDataMode(const uint8_t mode) {
	this->_spi_datamode = mode;
	SPI.setDataMode(this->_spi_datamode);
}

void Board::SPISetFrequency(const uint32_t frequency) {
	this->_spi_frequency = frequency;
	SPI.setFrequency(this->_spi_frequency);
}

uint8_t Board::SPITransfer(const uint8_t out) {
	uint8_t in;

	if (this->_spi_cs_active == LOW)
		digitalWrite(this->_spi_cs, LOW);
	else
		digitalWrite(this->_spi_cs, HIGH);

	SPI.beginTransaction(SPISettings(SPISettings(this->_spi_frequency, this->_spi_bitorder, this->_spi_datamode)));
	in = SPI.transfer(out);
	SPI.endTransaction();

	if (this->_spi_cs_active == LOW)
		digitalWrite(this->_spi_cs, HIGH);
	else
		digitalWrite(this->_spi_cs, LOW);

	return in;
}

void Board::SPITransfern(uint8_t * buffer, const size_t size) {
//	size_t i;

	if (this->_spi_cs_active == LOW)
		digitalWrite(this->_spi_cs, LOW);
	else
		digitalWrite(this->_spi_cs, HIGH);

//	Serial.print("SPI Tx len=");
//	Serial.print(size);
//	Serial.print(": ");
//	serialPrintHex(buffer, size);

	SPI.beginTransaction(SPISettings(this->_spi_frequency, this->_spi_bitorder, this->_spi_datamode));
//	for (i = 0; i < size; i++)
//		buffer[i] = SPI.transfer(buffer[i]);
	_spi->transfer(buffer, size);
	SPI.endTransaction();

//	Serial.print("SPI Rx len=");
//	Serial.print(size);
//	Serial.print(": ");
//	serialPrintHex(buffer, size);

	if (this->_spi_cs_active == LOW)
		digitalWrite(this->_spi_cs, HIGH);
	else
		digitalWrite(this->_spi_cs, LOW);
}

void Board::ReadNVRAM(uint8_t * buffer, const size_t offset, const size_t size) {
	size_t i;

	for (i = 0; i < size; i++)
		buffer[i] = EEPROM.read(offset + i);
}

void Board::WriteNVRAM(const uint8_t * buffer, const size_t offset, const size_t size) {
	size_t i;

	for (i = 0; i < size; i++)
		EEPROM.write((offset + i), buffer[i]);
}

void Board::ClearNVRAM(void) {
	size_t i;

	for (i = 0 ; i < EEPROM.length() ; i++)
		EEPROM.write(i, 0xFF);
}

void Board::CommitNVRAM(void) {
	EEPROM.commit();
}

void Board::deepSleep(uint64_t time_us) {
	ESP.deepSleep(time_us);
}

void Board::attachInterrupt(uint8_t pin, void (*handler)(void), int mode) {
	attachInterrupt(pin, handler, mode);
	if (pin == PIN_CD)
		this->_cd_isr_handler_attached = true;
	if (pin == PIN_DR)
		this->_dr_isr_handler_attached = true;
}

void Board::detachInterrupt(uint8_t pin) {
	if ((pin == PIN_CD) && (this->_cd_isr_handler_attached == true)) {
		detachInterrupt(pin);
		this->_cd_isr_handler_attached = false;
	}
	if ((pin == PIN_DR) && (this->_dr_isr_handler_attached == true)) {
		detachInterrupt(pin);
		this->_dr_isr_handler_attached = false;
	}
}

#endif
