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
/**
\example Communication/oled-screen/main.cpp

Working with OLED Screens and OSC
---------------------------------

This example shows how to interface with an OLED screen.

We use the u8g2 library for drawing on the screen.

This program is an OSC receiver which listens for OSC messages which can be sent
from another programmes on your host computer or from your Bela project running
on the board.

This example comes with a Pure Data patch which can be run on your host computer
for testing the screen. Download local.pd from the project files. This patch
sends a few different OSC messages which are interpreted by the cpp code which
then draws on the screen. We will now explain what each of the osc messages does.

Select the correct constructor for your screen (info to be updated). When you run
this project the first thing you will see is a Bela logo!

The important part of this project happens in the parseMessage() function which
we'll talk through now.

/osc-test

This is just to test whether the OSC link with Bela is functioning. When this message
is received you will see "OSC TEST SUCCESS!" printed on the screen. In the PD patch
click the bang on the left hand side. If you don't see anything change on the screen
then try clicking the `connect 192.168.6.2 7562` again to reestablish the connection.
Note that you will have to do this every time the Bela programme restarts.

/number

This prints a number to the screen which is received as a string and then cast to
a char. Every time you click the bang above this message a random number will be
generated and displayed on the screen.

/display-text

This draws text on the screen using UTF8.

/parameters

This simulates the visualisation of 3 parameters which are generated as ramps in
the PD patch. They are sent over as floats and then used to change the width of
three filled boxes. We also write to the screen for the parameter labelling directly
from the cpp file.

/lfos

This is similar to the /parameters but with a different animation and a different
means of taking a snapshot of an audio signal (see the PD Patch). In this case the
value from the oscillator is used to scale the size on an ellipse.

/waveform

This is most flexible message receiver. It will draw any number of floats (up to
maximum number of pixels of your screen) that are sent after /waveform. In the PD
patch we have two examples of this. The first sends 5 floats which are displayed
as 5 bars. No that the range of the screen is scaled to 0.0 to 1.0.

The second example stores the values of an oscillator into an array of 128 values
(the number of pixels on our OLED screen, update the array size in the PD patch if you
have a different sized screen). These 128 floats (between 0.0 and 1.0) are sent ten
times a second.
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

	int displayWidth = u8g2.getDisplayWidth();
	int displayHeight = u8g2.getDisplayHeight();
	std::string trString;
	float param1Value;
	float param2Value;
	float param3Value;
	std::string text1;
	std::string text2;
	std::string text3;

	if (msg.match("/osc-test").isOkNoMoreArgs())
	{
		printf("received /osc-test\n");
		u8g2.setFont(u8g2_font_ncenB08_tr);
		u8g2.setFontRefHeightText();
		u8g2.drawStr(0, displayHeight * 0.5, "OSC TEST SUCCESS!");
		u8g2.sendBuffer();
	} else if (msg.match("/number").popStr(trString).isOkNoMoreArgs())
	{
		const char *ctrStr = trString.c_str();
		printf("received /number %s\n", ctrStr);
		u8g2.setFont(u8g2_font_logisoso62_tn);
		u8g2.drawStr(0, 0, ctrStr);
		u8g2.sendBuffer();
	} else if (msg.match("/display-text").popStr(text1).popStr(text2).popStr(text3).isOkNoMoreArgs())
	{
		const char *ctrStr1 = text1.c_str();
		const char *ctrStr2 = text2.c_str();
		const char *ctrStr3 = text3.c_str();
		printf("received /display-text string %s %s %s\n", ctrStr1, ctrStr2, ctrStr3);
		u8g2.setFont(u8g2_font_4x6_tf);
		u8g2.setFontRefHeightText();
		u8g2.drawUTF8(displayWidth * 0.5, displayHeight * 0.25, ctrStr1);
		u8g2.drawUTF8(displayWidth * 0.5, displayHeight * 0.5, ctrStr2);
		u8g2.drawUTF8(displayWidth * 0.5, displayHeight * 0.75, ctrStr3);
		u8g2.sendBuffer();
	} else 	if (msg.match("/parameters").popFloat(param1Value).popFloat(param2Value).popFloat(param3Value).isOkNoMoreArgs())
	{
		printf("received /parameters float %f float %f float %f\n", param1Value, param2Value, param3Value);
		u8g2.setFont(u8g2_font_4x6_tf);
		u8g2.setFontRefHeightText();
		u8g2.drawStr(0, 0, "PARAMETER 1:");
		u8g2.drawBox(0, 10, u8g2.getDisplayWidth() * param1Value, 10);
		u8g2.drawStr(0, 22, "PARAMETER 2:");
		u8g2.drawBox(0, 32, u8g2.getDisplayWidth() * param2Value, 10);
		u8g2.drawStr(0, 44, "PARAMETER 3:");
		u8g2.drawBox(0, 54, u8g2.getDisplayWidth() * param3Value, 10);
		u8g2.sendBuffer();
	} else 	if (msg.match("/lfos").popFloat(param1Value).popFloat(param2Value).popFloat(param3Value).isOkNoMoreArgs())
	{
		printf("received /lfos float %f float %f float %f\n", param1Value, param2Value, param3Value);
		u8g2.drawEllipse(displayWidth * 0.2, displayHeight * 0.5, 10, displayHeight * 0.5 * param1Value);
		u8g2.drawEllipse(displayWidth * 0.5, displayHeight * 0.5, 10, displayHeight * 0.5 * param2Value);
		u8g2.drawEllipse(displayWidth * 0.8, displayHeight * 0.5, 10, displayHeight * 0.5 * param3Value);
		u8g2.drawHLine(0, displayHeight * 0.5, displayWidth);
		u8g2.sendBuffer();
	} else if (msg.match("/waveform"))
	{
		oscpkt::Message::ArgReader args(msg);
		if(args)
		{
			const unsigned int nValues = args.nbArgRemaining();
			float values[nValues];
			for(unsigned int n = 0; n < nValues; ++n)
			{
				if(args.isFloat())
				{
					args.popFloat(values[n]);
				} else if(args.isInt32()) {
					int i;
					args.popInt32(i);
					values[n] = i;
				} else {
					fprintf(stderr, "Wrong type at argument %d\n", n);
					return -1;
				}
			}
			// now tenValues contains the 10 values.
			// prepare a bitmap:
			printf("received /waveform floats:");
			for(unsigned int n = 0; n < 10; ++n)
			printf("%f ", values[n]);
			printf("\n");

			for(unsigned int x = 0; x < displayWidth; ++x)
			{
				// we interpret each value as the vertical displacement and
				// we want to draw a series of horizontal lines at the specified points
				unsigned int valIdx = x * float(nValues) / displayWidth;
				unsigned int y = values[valIdx] * displayHeight;
				u8g2.drawPixel(x, y);
			}
		}
	} else
	{
		printf("unhandled message to: %s \n", msg.addressPattern().c_str());
	}
	u8g2.sendBuffer();
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
	u8g2.setFont(u8g2_font_4x6_tf);
	u8g2.setFontRefHeightText();
	u8g2.setFontPosTop();
	u8g2.drawStr(0, 0, " ____  _____ _        _");
	u8g2.drawStr(0, 7, "| __ )| ____| |      / \\");
	u8g2.drawStr(0, 14, "|  _ \\|  _| | |     / _ \\");
	u8g2.drawStr(0, 21, "| |_) | |___| |___ / ___ \\");
	u8g2.drawStr(0, 28, "|____/|_____|_____/_/   \\_\\");
	u8g2.sendBuffer();

	// OSC
	oscReceiver.setup(localPort, parseMessage);
	while(!gStop)
	{
		usleep(100000);
	}
	return false;
}
