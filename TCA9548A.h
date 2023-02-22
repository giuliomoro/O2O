#pragma once
#include <I2c.h>

class TCA9548A : public I2c
{
public:
	TCA9548A() {i2C_file = -1;};
	TCA9548A(int bus, int address)
	{
		int ret = initI2C_RW(bus, address, -1);
		if(ret || select(-1)) // disable all channels and at the same time verify address is valid
			throw std::runtime_error("Unable to open TCA9548A. Ensure the multiplexer is connected"
			"and the bus and address are correct.");
	}
	int select(int channel)
	{
		i2c_char_t byte = channel < 0 || channel >= 8 ? 0 : 1 << channel;
		return (sizeof(byte) != write(i2C_file, &byte, sizeof(byte)));
	}
};
