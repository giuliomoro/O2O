/*
 ____  _____ _        _
| __ )| ____| |      / \
|  _ \|  _| | |     / _ \
| |_) | |___| |___ / ___ \
|____/|_____|_____/_/   \_\

The platform for ultra-low latency audio and sensor processing

http://bela.io

A project of the Augmented Instruments Laboratory within the
Centre for Digital Music at Queen Mary University of London.
http://www.eecs.qmul.ac.uk/~andrewm

(c) 2016 Augmented Instruments Laboratory: Andrew McPherson,
  Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack,
  Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

The Bela software is distributed under the GNU Lesser General Public License
(LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt
*/

#include <signal.h>
#include <libraries/OscReceiver/OscReceiver.h>
#include <unistd.h>
#include "u8g2/cppsrc/U8g2lib.h"

const unsigned int SH1106_addr = 0x3c;
U8G2_SH1106_128X64_NONAME_F_HW_I2C_LINUX u8g2(U8G2_R0);
OscReceiver oscReceiver;
int localPort = 7562; //port for incoming OSC messages
int gStop = 0;

// Handle Ctrl-C by requesting that the audio rendering stop
void interrupt_handler(int var)
{
	gStop = true;
}

int parseMessage(oscpkt::Message msg, void* arg)
{
	u8g2.clearBuffer();
	float floatArg;
	std::string trString;
	if (msg.match("/osc-test").popFloat(floatArg).isOkNoMoreArgs())
	{
		printf("received /osc-test float %f\n", floatArg);
		// setTextSize(1);
		// setTextColor(WHITE);
		// setCursor(10,0);
		// print_str("OSC TEST!");
		// printNumber(floatArg, DEC);
		u8g2.sendBuffer();
	} else if (msg.match("/tr").popFloat(floatArg).popStr(trString).isOkNoMoreArgs())
	{
		const char *ctrStr = trString.c_str();
		printf("received /tr float %f string %s\n", floatArg, ctrStr);
		// setTextSize(2);
		// setTextColor(WHITE);
		// setCursor(10,0);
		// print_str(ctrStr);
		u8g2.sendBuffer();
	} else if (msg.match("/tr").popStr(trString).isOkNoMoreArgs())
	{
		const char *ctrStr = trString.c_str();
		printf("received /tr string %s\n", ctrStr);
		// setTextSize(1);
		// setTextColor(WHITE);
		// setCursor(10,20);
		// print_str(ctrStr);
		u8g2.sendBuffer();
	} else 	if (msg.match("/param1").popFloat(floatArg).isOkNoMoreArgs())
	{
		printf("received /param1 float %f\n", floatArg);
		// fillRect(10,10, (100*floatArg), 10, WHITE);
		u8g2.sendBuffer();
	} else {
		printf("unhandled message to: %s \n", msg.addressPattern().c_str());
	}
	return 0;
}

int main(int main_argc, char *main_argv[])
{
	// Set up interrupt handler to catch Control-C and SIGTERM
	signal(SIGINT, interrupt_handler);
	signal(SIGTERM, interrupt_handler);

	u8g2.setI2CAddress(SH1106_addr);
	u8g2.initDisplay();
	u8g2.setPowerSave(0);
	u8g2.clearBuffer();
	u8g2.setFont(u8g2_font_smart_patrol_nbp_tr);
	u8g2.setFontRefHeightText();
	u8g2.setFontPosTop();
	u8g2.drawStr(0, 0, "u8g2 Linux I2C");
	u8g2.sendBuffer();
	// OSC
	oscReceiver.setup(localPort, parseMessage);
	while(!gStop)
	{
		usleep(100000);
	}
	return false;
}
