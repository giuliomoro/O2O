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
then try clicking the `connect bela.local 7562` again to reestablish the connection.
Note that you will have to do this every time the Bela programme restarts.

/number

This prints a number to the screen which is received as a float. Every time you click the bang
above this message a random number will be generated and displayed on the screen.

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
#include "u8g2/U8g2LinuxI2C.h"
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <MiscUtilities.h>

const unsigned int gI2cBus = 1;

// #define I2C_MUX // allow I2C multiplexing to select different target displays
struct Display {U8G2 d; int mux;};
std::vector<Display> gDisplays = {
	// use `-1` as the last value to indicate that the display is not behind a mux, or a number between 0 and 7 for its muxed channel number
	{ U8G2_SH1106_128X64_NONAME_F_HW_I2C_LINUX(U8G2_R0, gI2cBus, 0x3c), -1},
	// add more displays / addresses here
};

unsigned int gActiveTarget = 0;
const int gLocalPort = 7562; //port for incoming OSC messages

#ifdef I2C_MUX
#include "TCA9548A.h"
const unsigned int gMuxAddress = 0x70;
TCA9548A gTca;
#endif // I2C_MUX

/// Determines how to select which display a message is targeted to:
typedef enum {
	kTargetSingle, ///< Single target (one display).
	kTargetEach, ///< The first argument to each message is an index corresponding to the target display
	kTargetStateful, ///< Send a message to /target <float> to select which is the active display that all subsequent messages will be sent to
} TargetMode;

TargetMode gTargetMode = kTargetSingle; // can be changed with /targetMode
OscReceiver oscReceiver;
int gStop = 0;

// Handle Ctrl-C by requesting that the audio rendering stop
void interrupt_handler(int var)
{
	gStop = true;
}

static void switchTarget(int target)
{
#ifdef I2C_MUX
	if(target >= gDisplays.size())
		return;
	int mux = gDisplays[target].mux;
	static int oldMux = -1;
	if(oldMux != mux)
		gTca.select(mux);
#endif // I2C_MUX
	gActiveTarget = target;
}

int parseMessage(oscpkt::Message msg, const char* address, void*)
{
	float param1Value;
	float param2Value;
	float param3Value;
	oscpkt::Message::ArgReader args = msg.arg();
	enum {
		kOk = 0,
		kUnmatchedPattern,
		kWrongArguments,
		kInvalidMode,
		kOutOfRange,
	} error = kOk;
	printf("Message from %s\n", address);
	bool stateMessage = false;
	// check state (non-display) messages first
	if (msg.match("/target")) {
		stateMessage = true;
		if(kTargetStateful != gTargetMode) {
			fprintf(stderr, "Target mode is not stateful, so /target messages are ignored\n");
			error = kInvalidMode;
		} else {
			int target;
			if(args.popNumber(target).isOkNoMoreArgs()) {
				printf("Selecting /target %d\n", target);
				switchTarget(target);
			} else {
				fprintf(stderr, "Argument to /target should be numeric (int or float)\n");
				error = kWrongArguments;
			}
		}
	} else if (msg.match("/targetMode")) {
		stateMessage = true;
		int mode;
		if(args.popNumber(mode).isOkNoMoreArgs())
		{
			if(mode != kTargetSingle && mode != kTargetStateful && mode != kTargetEach)
				error = kOutOfRange;
			else {
				gTargetMode = (TargetMode)mode;
				printf("Target mode: %d\n", mode);
			}
		} else
			error = kWrongArguments;
	}
	if(gActiveTarget >= gDisplays.size())
	{
		fprintf(stderr, "Target %u out of range. Only %u displays are available\n", gActiveTarget, gDisplays.size());
		return 1;
	}
	U8G2& u8g2 = gDisplays[gActiveTarget].d;
	u8g2.clearBuffer();
	int displayWidth = u8g2.getDisplayWidth();
	int displayHeight = u8g2.getDisplayHeight();
	if(!stateMessage && kTargetEach == gTargetMode)
	{
		// if we are in kTargetEach and the message is for a display, we need to peel off the
		// first argument (which denotes the target display) before processing the message
		int target;
		if(args.popNumber(target))
		{
			switchTarget(target);
		} else {
			fprintf(stderr, "Target mode is \"Each\", therefore the first argument should be an int or float specifying the target display\n");
			error = kWrongArguments;
		}
	}

	// code below MUST use msg.match() to check patterns and args.pop... or args.is ... to check message content.
	// this way, anything popped above (if we are in kTargetEach mode), won't be re-used below
	if(error || stateMessage) {
		// nothing to do here, just avoid matching any of the others
	} else if (msg.match("/osc-test"))
	{
		if(!args.isOkNoMoreArgs()){
			error = kWrongArguments;
		}
		else {
			printf("received /osc-test\n");
			u8g2.setFont(u8g2_font_ncenB08_tr);
			u8g2.setFontRefHeightText();
			u8g2.drawStr(0, displayHeight * 0.5, "OSC TEST SUCCESS!");
		}
	} else if (msg.match("/number"))
	{
		int number;
		if(args.popNumber(number).isOkNoMoreArgs())
		{
			printf("received /number %d\n", number);
			u8g2.setFont(u8g2_font_logisoso62_tn);
			u8g2.drawUTF8(0, 0, std::to_string(number).c_str());
		}
		else
			error = kWrongArguments;
	} else if (msg.match("/display-text"))
	{
		std::string text1;
		std::string text2;
		std::string text3;
		if(!args.popStr(text1).popStr(text2).popStr(text3).isOkNoMoreArgs())
			error = kWrongArguments;
		else {
			const char *ctrStr1 = text1.c_str();
			const char *ctrStr2 = text2.c_str();
			const char *ctrStr3 = text3.c_str();
			printf("received /display-text string %s %s %s\n", ctrStr1, ctrStr2, ctrStr3);
			u8g2.setFont(u8g2_font_4x6_tf);
			u8g2.setFontRefHeightText();
			u8g2.drawUTF8(displayWidth * 0.5, displayHeight * 0.25, ctrStr1);
			u8g2.drawUTF8(displayWidth * 0.5, displayHeight * 0.5, ctrStr2);
			u8g2.drawUTF8(displayWidth * 0.5, displayHeight * 0.75, ctrStr3);
		}
	} else if (msg.match("/display-strings-and-numbers"))
	{
		// send a mix of strings and numbers with explicit newline characters
		std::stringstream out;
		while(args.nbArgRemaining() && args.isOk() && kOk == error)
		{
			if(args.isStr())
			{
				std::string str;
				args.popStr(str);
				// Pd cannot send \n, but will send a literal \\n
				if("\\n" == str || "\n" == str || "\n\r" == str)
					out << "\n"; // avoid whitespace at beginning of line
				else
					out << str << " ";
			} else if(args.isInt32())
			{
				int32_t num;
				args.popInt32(num);
				out << std::fixed << std::setprecision(0) << num << " ";
			} else if(args.isNumber())
			{
				double num;
				args.popNumber(num);
				out << std::fixed << std::setprecision(2) << num << " ";
			} else
				error = kWrongArguments;
		}
		if(!args.isOkNoMoreArgs())
			error = kWrongArguments;
		if(kOk == error)
		{
			printf("received /display-strings-and-numbers _%s_\n", out.str().c_str());
			auto strs = StringUtils::split(out.str(), '\n', false, 3);
			// remove last element if empty (bug in StringUtils::split())
			if(strs.size() && !strs.back().size())
				strs.resize(strs.size() - 1);
			if(strs.size())
			{
				if(3 == strs.size())
					u8g2.setFont(u8g2_font_4x6_tf);
				else if(2 == strs.size())
					u8g2.setFont(u8g2_font_6x10_tf);
				else if(1 == strs.size())
					u8g2.setFont(u8g2_font_8x13_tf);
				u8g2.setFontRefHeightText();
				for(size_t n = 0; n < strs.size(); ++n)
					u8g2.drawUTF8(0, displayHeight * float(n + 1) / (strs.size() + 1), strs[n].c_str());
			}
		}
	} else if (msg.match("/parameters"))
	{
		if(!args.popFloat(param1Value).popFloat(param2Value).popFloat(param3Value).isOkNoMoreArgs())
			error = kWrongArguments;
		else {
			printf("received /parameters float %f float %f float %f\n", param1Value, param2Value, param3Value);
			u8g2.setFont(u8g2_font_4x6_tf);
			u8g2.setFontRefHeightText();
			u8g2.drawStr(0, 0, "PARAMETER 1:");
			u8g2.drawBox(0, 10, u8g2.getDisplayWidth() * param1Value, 10);
			u8g2.drawStr(0, 22, "PARAMETER 2:");
			u8g2.drawBox(0, 32, u8g2.getDisplayWidth() * param2Value, 10);
			u8g2.drawStr(0, 44, "PARAMETER 3:");
			u8g2.drawBox(0, 54, u8g2.getDisplayWidth() * param3Value, 10);
		}
	} else if(msg.match("/lfos"))
	{
		if(!args.popFloat(param1Value).popFloat(param2Value).popFloat(param3Value).isOkNoMoreArgs())
			error = kWrongArguments;
		printf("received /lfos float %f float %f float %f\n", param1Value, param2Value, param3Value);
		u8g2.drawEllipse(displayWidth * 0.2, displayHeight * 0.5, 10, displayHeight * 0.5 * param1Value);
		u8g2.drawEllipse(displayWidth * 0.5, displayHeight * 0.5, 10, displayHeight * 0.5 * param2Value);
		u8g2.drawEllipse(displayWidth * 0.8, displayHeight * 0.5, 10, displayHeight * 0.5 * param3Value);
		u8g2.drawHLine(0, displayHeight * 0.5, displayWidth);
	} else if (msg.match("/waveform"))
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
				error = kWrongArguments;
			}
		}
		if(!error)
		{
			printf("received /waveform with %d values\n", nValues);

			// prepare a bitmap:
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
		error = kUnmatchedPattern;
	if(error)
	{
		std::string str;
		switch(error){
			case kUnmatchedPattern:
				str = "no matching pattern available\n";
				break;
			case kWrongArguments:
				str = "unexpected types and/or length\n";
				break;
			case kInvalidMode:
				str = "invalid target mode\n";
				break;
			case kOutOfRange:
				str = "argument(s) value(s) out of range\n";
				break;
			case kOk:
				str = "";
				break;
		}
		fprintf(stderr, "An error occurred with message to: %s: %s\n", msg.addressPattern().c_str(), str.c_str());
		return 1;
	} else
	{
		if(!stateMessage)
			u8g2.sendBuffer();
	}
	return 0;
}

int main(int main_argc, char *main_argv[])
{
	if(0 == gDisplays.size())
	{
		fprintf(stderr, "No displays in gDisplays\n");
		return 1;
	}
#ifdef I2C_MUX
	if(gTca.initI2C_RW(gI2cBus, gMuxAddress, -1) || gTca.select(-1))
	{
		fprintf(stderr, "Unable to initialise the TCA9548A multiplexer. Are the address and bus correct?\n");
		return 1;
	}
#endif // I2C_MUX
	for(unsigned int n = 0; n < gDisplays.size(); ++n)
	{
		switchTarget(n);
		U8G2& u8g2 = gDisplays[gActiveTarget].d;
#ifndef I2C_MUX
		int mux = gDisplays[gActiveTarget].mux;
		if(-1 != mux)
		{
			fprintf(stderr, "Display %u requires mux %d but I2C_MUX is disabled\n", n, mux);
			return 1;
		}
#endif // I2C_MUX
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
		if(gDisplays.size() > 1)
		{
			std::string targetString = "Target ID: " + std::to_string(n);
			u8g2.drawStr(0, 50, targetString.c_str());
		}
		u8g2.sendBuffer();
	}
	// Set up interrupt handler to catch Control-C and SIGTERM
	signal(SIGINT, interrupt_handler);
	signal(SIGTERM, interrupt_handler);
	// OSC
	oscReceiver.setup(gLocalPort, parseMessage);
	while(!gStop)
	{
		usleep(100000);
	}
	return 0;
}
