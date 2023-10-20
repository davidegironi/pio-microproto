/**
 * microproto sample
 * 
 * copyright (c) Davide Gironi, 2020
 * 
 * Released under GPLv3.
 * Please refer to LICENSE file for licensing information.
 **/

#include <Arduino.h>

//include microproto
#include "microproto.h"

//set microproto
microprotoconfig_t microprotoconfig[1];
MicroProto microproto = MicroProto(microprotoconfig, 1);

//data length
#define MAXLENGTH 50

//local vars
uint16_t microprotocmd = 0;
int microprotodatalength = 0;
char microprotodata[MAXLENGTH];

void setup() {
	Serial.begin(9600);
	
	//set microproto config
	microprotoconfig_t microprotoconfig[1];
	microprotoconfig[0].fputc = microproto.defaultserialputc;
	microprotoconfig[0].fgetc = microproto.defaultserialgetc;
	microprotoconfig[0].datasize = MAXLENGTH;
	
	//init microproto
	microproto = MicroProto(microprotoconfig, 1);
}

void loop() {
	microproto.getcommandrunner();

	//get a command
	if(microproto.getlastcommand(&microprotocmd, microprotodata, &microprotodatalength)) {
		if(microprotocmd == 1) {
			//reply with same command
			microproto.sendcommand(microprotocmd, microprotodata, microprotodatalength, 0);
		} else if(microprotocmd == 2) {
			//reply with ack
			microproto.sendack(microprotocmd, microprotodata, microprotodatalength);
		} else if(microprotocmd == 3) {
			//reply with nak
			microproto.sendnak(microprotocmd, microprotodata, microprotodatalength);
		} else if(microprotocmd == 4) {
			//trigger an ask for a ack
			uint16_t cmd = 1004;
			if(microproto.sendcommand(cmd, microprotodata, microprotodatalength, 1)) {
				sprintf(microprotodata, "askack ok");
				microproto.sendcommand(microprotocmd, microprotodata, strlen(microprotodata), 0);
			} else {
				sprintf(microprotodata, "askack err");
				microproto.sendcommand(microprotocmd, microprotodata, strlen(microprotodata), 0);
			}
		} else if(microprotocmd == 5) {
			//trigger an ask for a nak
			uint16_t cmd = 1005;
			if(microproto.sendcommand(cmd, microprotodata, microprotodatalength, 1)) {
				sprintf(microprotodata, "asknak ok");
				microproto.sendcommand(microprotocmd, microprotodata, strlen(microprotodata), 0);
			} else {
				sprintf(microprotodata, "asknak err");
				microproto.sendcommand(microprotocmd, microprotodata, strlen(microprotodata), 0);
			}
		}
	}
}