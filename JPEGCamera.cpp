#define NDEBUG

#define __ASSERT_USE_STDERR
#include <assert.h>
#include "JPEGCamera.h"

#ifndef NDEBUG
#include <inttypes.h>
void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp)
{
    Serial.println(__func);
    Serial.println(__file);
    Serial.println(__lineno, DEC);
    Serial.println(__sexp);
    Serial.flush();

    abort();
}
#endif // NDEBUG

JPEGCamera::JPEGCamera(Stream& serial)
: cameraSerial(serial), address(0), eof(false), imageSize(0)
{
}

//Get the size of the image currently stored in the camera
//return: the size of the image currently stored in the camera.
int JPEGCamera::getSize()
{
	const uint8_t cmd[] PROGMEM = {0x56, 0x00, 0x34, 0x01, 0x00};
	uint8_t res[9];

	sendCommand(cmd, sizeof(cmd), res, sizeof(res));
	assert(memcmp(res, (const uint8_t[]){0x76, 0x00, 0x34, 0x00, 0x04, 0x00, 0x00}, sizeof(res) - 2) == 0);

	int fileSize = 0;
	fileSize = (res[7] << 8);
	fileSize += res[8];
#ifndef NDEBUG
	Serial.print("filesize=");
	Serial.println(fileSize);
	Serial.flush();
#endif /* NDEBUG */

	return fileSize;
}

//Reset the camera
void JPEGCamera::reset()
{
	const uint8_t cmd[] PROGMEM = {0x56, 0x00, 0x26, 0x00};
	uint8_t res[20];

	while (cameraSerial.available() > 0)
		cameraSerial.read();

	sendCommand(cmd, sizeof(cmd), res, 4);
	assert(memcmp(res, (const uint8_t[]){ 0x76, 0x00, 0x26, 0x00 }, 4) == 0);

	const char initEnd[] PROGMEM = "Init end\r";
	do {
		cameraSerial.readBytesUntil('\n', res, sizeof(res));
	} while (strncmp(initEnd, res, strlen(initEnd)) != 0);
}

//Take a new picture
void JPEGCamera::takePicture() {
	const uint8_t cmd[] PROGMEM = {0x56, 0x00, 0x36, 0x01, 0x00};
	uint8_t res[5];

	sendCommand(cmd, sizeof(cmd), res, sizeof(res));
	assert(memcmp(res, (const uint8_t[]){0x76, 0x00, 0x36, 0x00, 0x00}, sizeof(res)) == 0);

	eof = false;
	address = 0;
	imageSize = getSize();
}

//Stop taking pictures
void JPEGCamera::stopPicture()
{
	const uint8_t cmd[] PROGMEM = {0x56, 0x00, 0x36, 0x01, 0x03};
	uint8_t res[5];

	sendCommand(cmd, sizeof(cmd), res, sizeof(res));
	assert(memcmp(res, (const uint8_t[]) { 0x76, 0x00, 0x36, 0x00, 0x00 }, sizeof(res)) == 0);
}

//Get the read_size bytes picture data from the camera
//NOTE: This function must be called repeatedly until all of the data is read
size_t JPEGCamera::readData(uint8_t* data)
{
	uint8_t cmd[] PROGMEM = {0x56, 0x00, 0x32, 0x0c, 0x00, 0x0a, 0x00, 0x00};

	cameraSerial.write(cmd, sizeof(cmd));
	cameraSerial.write((uint8_t)(address >> 8));
	cameraSerial.write((uint8_t)address);
	cameraSerial.write((uint8_t)0x00);
	cameraSerial.write((uint8_t)0x00);
	cameraSerial.write((uint8_t)(read_size >> 8));
	cameraSerial.write((uint8_t)read_size);
	cameraSerial.write((uint8_t)0x00);
	cameraSerial.write((uint8_t)0x0A);
	cameraSerial.flush();

	uint8_t res[5];
	for (size_t len = 0; len < sizeof(res);) {
		len += cameraSerial.readBytes(res + len, sizeof(res) - len);
		assert(memcmp(res, (const char[]){0x76, 0x00, 0x32, 0x00, 0x00}, sizeof(res)) == 0);
	}

	uint8_t count;
	uint8_t size = (imageSize - address > read_size) ? read_size : imageSize - address;
	for (count = 0; count < size;) {
		count += cameraSerial.readBytes(data + count, size - count);
#ifndef NDEBUG
		Serial.print("Count=");
		Serial.print(count);
		Serial.print(" Address=");
		Serial.print(address);
		Serial.print(" Image data remaining=");
		Serial.println(imageSize - address - count);
#endif /* NDEBUG */
	}
	address += count;

	if ((data[count - 1] == 0xD9) && (data[count - 2] == 0xFF)) {
		eof = true;
#ifndef NDEBUG
		Serial.println("=== EOF ===");
#endif /* NDEBUG */
	}

	// 5 for 0x76, 0x00, 0x32, 0x00, 0x00
	for (size_t len = 0; len < read_size - size + 5; ) {
		while (cameraSerial.available()) {
			cameraSerial.read();
			++len;
		}
	}
	
	return count;
}

void JPEGCamera::setReadSize(size_t size)
{
	read_size = size & ~0x07;
#ifndef NDEBUG
	Serial.print("read_size=");
	Serial.println(read_size);
#endif /* NDEBUG */
}

// Check the stream of the camera is EOF or not.
bool JPEGCamera::isEOF()
{
	return (eof);
}

// Enter the camera into power saving mode.
void JPEGCamera::enterPowerSaving()
{
	const uint8_t cmd[] PROGMEM = {0x56, 0x00, 0x3E, 0x03, 0x00, 0x01, 0x01};
	uint8_t res[5];

	sendCommand(cmd, sizeof(cmd), res, sizeof(res));
	assert(memcmp(res, (const char[]){0x76, 0x00, 0x3E, 0x00, 0x00}, sizeof(res)) == 0);
}

// Quit the camera from power saving mode.
void JPEGCamera::quitPowerSaving() {
	const uint8_t cmd[] PROGMEM = {0x56, 0x00, 0x3E, 0x03, 0x00, 0x01, 0x00};
	uint8_t res[5];

	sendCommand(cmd, sizeof(cmd), res, sizeof(res));
	assert(memcmp(res, (const char[]){0x76, 0x00, 0x3E, 0x00, 0x00}, sizeof(res)) == 0);
}

// Set the compression ratio of the camera to the argument 'ratio'
void JPEGCamera::setCompressionRatio(uint8_t ratio)
{
	const uint8_t cmd[] PROGMEM = {0x56, 0x00, 0x31, 0x05, 0x01, 0x01, 0x12, 0x04};
	cameraSerial.write(cmd, sizeof(cmd));
	cameraSerial.write(ratio);

	uint8_t res[5];
	cameraSerial.readBytes(res, sizeof(res));
	assert(memcmp(res, (const char[]){0x76, 0x00, 0x31, 0x00, 0x00}, sizeof(res)) == 0);
}

// Set the baud rate of serial interface for the camera
void JPEGCamera::setBaudRate(BaudRate rate)
{
	uint8_t cmd[] = {0x56, 0x00, 0x24, 0x03, 0x01, 0x00, 0x00};
	uint8_t res[5];

	cmd[5] = rate >> 8;
	cmd[6] = rate & 0xff;
	sendCommand(cmd, sizeof(cmd), res, sizeof(res));
	assert(memcmp(res, (const char[]){0x76, 0x00, 0x24, 0x00, 0x00}, sizeof(res)) == 0);
}

// Set the image size to take a picture
void JPEGCamera::setSize(ImageSize size)
{
	uint8_t cmd[] = {0x56, 0x00, 0x31, 0x05, 0x04, 0x01, 0x00, 0x19, 0x00};
	uint8_t res[5];

	cmd[sizeof(cmd)-1] = size;
	sendCommand(cmd, sizeof(cmd), res, sizeof(res));
	assert(memcmp(res, (const char[]){0x76, 0x00, 0x31, 0x00, 0x00}, sizeof(res)) == 0);

	reset();
}

//Send a command to the camera
void JPEGCamera::sendCommand(const uint8_t cmd[], size_t cmdLen, uint8_t res[], size_t resLen)
{
	cameraSerial.write(cmd, cmdLen);
	cameraSerial.flush();

	for (size_t count = 0; (resLen - count) > 0;) {
		count += cameraSerial.readBytes(&res[count], resLen - count);
	}
}
