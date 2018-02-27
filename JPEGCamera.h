#ifndef JPEGCamera_h
#define JPEGCamera_h

#include <avr/pgmspace.h>
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include <inttypes.h>

class JPEGCamera {
private:
	Stream& cameraSerial;
	int	address;
	bool eof;
	int imageSize;
	size_t read_size = 56;


public:
	enum BaudRate {
		B9600 = 0xaec8,
		B19200 = 0x56e4,
		B38400 = 0x2af2,
		B57600 = 0x1c4c,
		B115200 = 0x0da6
	};

	enum ImageSize {
		VGA = 0x00,
		QVGA = 0x11,
		QQVGA = 0x22
	};

	JPEGCamera(Stream& serial);
	void reset();
	int getSize();
	void takePicture();
	void stopPicture();
	size_t readData(uint8_t* data);
	void setReadSize(size_t size);
	bool isEOF();
	void enterPowerSaving();
	void quitPowerSaving();
	void setCompressionRatio(uint8_t ratio);
	void setBaudRate(BaudRate rate);
	void setSize(ImageSize size);

private:
	void sendCommand(const uint8_t cmd[], size_t cmdLen, uint8_t res[], size_t resLen);
};

#endif
