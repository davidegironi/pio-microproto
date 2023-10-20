/*
microproto 0.0.2

copyright (c) Davide Gironi, 2020

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#include "microprotoinc.h"

//crc8 lookup for poly X^8 + X^7 + X^2 + 1
#if defined(__AVR)
static const unsigned char PROGMEM microproto_crc8table[256] =
#else
static const unsigned char microproto_crc8table[256] =
#endif
{
  0x00,0x85,0x8F,0x0A,0x9B,0x1E,0x14,0x91,0xB3,0x36,
  0x3C,0xB9,0x28,0xAD,0xA7,0x22,0xE3,0x66,0x6C,0xE9,
  0x78,0xFD,0xF7,0x72,0x50,0xD5,0xDF,0x5A,0xCB,0x4E,
  0x44,0xC1,0x43,0xC6,0xCC,0x49,0xD8,0x5D,0x57,0xD2,
  0xF0,0x75,0x7F,0xFA,0x6B,0xEE,0xE4,0x61,0xA0,0x25,
  0x2F,0xAA,0x3B,0xBE,0xB4,0x31,0x13,0x96,0x9C,0x19,
  0x88,0x0D,0x07,0x82,0x86,0x03,0x09,0x8C,0x1D,0x98,
  0x92,0x17,0x35,0xB0,0xBA,0x3F,0xAE,0x2B,0x21,0xA4,
  0x65,0xE0,0xEA,0x6F,0xFE,0x7B,0x71,0xF4,0xD6,0x53,
  0x59,0xDC,0x4D,0xC8,0xC2,0x47,0xC5,0x40,0x4A,0xCF,
  0x5E,0xDB,0xD1,0x54,0x76,0xF3,0xF9,0x7C,0xED,0x68,
  0x62,0xE7,0x26,0xA3,0xA9,0x2C,0xBD,0x38,0x32,0xB7,
  0x95,0x10,0x1A,0x9F,0x0E,0x8B,0x81,0x04,0x89,0x0C,
  0x06,0x83,0x12,0x97,0x9D,0x18,0x3A,0xBF,0xB5,0x30,
  0xA1,0x24,0x2E,0xAB,0x6A,0xEF,0xE5,0x60,0xF1,0x74,
  0x7E,0xFB,0xD9,0x5C,0x56,0xD3,0x42,0xC7,0xCD,0x48,
  0xCA,0x4F,0x45,0xC0,0x51,0xD4,0xDE,0x5B,0x79,0xFC,
  0xF6,0x73,0xE2,0x67,0x6D,0xE8,0x29,0xAC,0xA6,0x23,
  0xB2,0x37,0x3D,0xB8,0x9A,0x1F,0x15,0x90,0x01,0x84,
  0x8E,0x0B,0x0F,0x8A,0x80,0x05,0x94,0x11,0x1B,0x9E,
  0xBC,0x39,0x33,0xB6,0x27,0xA2,0xA8,0x2D,0xEC,0x69,
  0x63,0xE6,0x77,0xF2,0xF8,0x7D,0x5F,0xDA,0xD0,0x55,
  0xC4,0x41,0x4B,0xCE,0x4C,0xC9,0xC3,0x46,0xD7,0x52,
  0x58,0xDD,0xFF,0x7A,0x70,0xF5,0x64,0xE1,0xEB,0x6E,
  0xAF,0x2A,0x20,0xA5,0x34,0xB1,0xBB,0x3E,0x1C,0x99,
  0x93,0x16,0x87,0x02,0x08,0x8D
};

//define compute crc function
#if defined(__AVR)
#define microproto_computecrc(c, v) pgm_read_word(&(microproto_crc8table[c ^ v]));
#else
#define microproto_computecrc(c, v) microproto_crc8table[c ^ v];
#endif

//main microproto config
static microprotoconfig_t *microproto_config;
//config elements
static uint8_t microproto_configsize = 0;
//config channel id
static uint8_t microproto_channelid = 0;

//disable command runner from external call, usefull if called by timers
static uint8_t microproto_disablegetcommandrunner = 0;


/**
 * initialize the micro protocol
 */
void microproto_init(microprotoconfig_t *config, uint8_t configsize) {
	microproto_config = config;
	microproto_configsize = configsize;

	//initialize confit items
	for(uint8_t i=0; i<microproto_configsize; i++) {
		if(microproto_config[i].continousreadchars <= 0)
			microproto_config[i].continousreadchars = MICROPROTO_DEFAULTCONTINOUSREADCHARS;
		microproto_config[i].internaldata = malloc(microproto_config[i].datasize*sizeof(char));
		microproto_config[i].internaldatalength = 0;
		microproto_config[i].internalhascommand = 0;
		microproto_config[i].internalstartchar = 0x00;
		microproto_config[i].internalcmd = 0;
		microproto_config[i].internallastreadcharisvalid = 0;
		microproto_config[i].internalrunnerreadingcounter = 0;
		microproto_config[i].internalrunnerreading = 0;
		microproto_config[i].internalrunnerreadingcmdindex = 0;
		microproto_config[i].internalrunnerpreviuoschar = 0x00;
	}
}


/**
 * set the active channel
 */
void microproto_setchannel(uint8_t id) {
	if(id < microproto_configsize)
		microproto_channelid = id;
}


/**
 * get command by single char reading
 */
uint8_t microproto_getcommandrunnerint() {
	uint8_t checkcommand = 0;
	uint8_t crc = 0;
	uint8_t crcc = 0;

	microproto_config[microproto_channelid].internallastreadcharisvalid = 0;

	//get command
	uint16_t c = MICROPROTO_INVALIDC;
	if(microproto_config[microproto_channelid].internalrunnerreading)
		c = microproto_config[microproto_channelid].fgetc();
	else {
		if(microproto_config[microproto_channelid].internalrunnerreadingcounter == MICROPROTO_READINGCOUNTERTHRESHOLD) {
			microproto_config[microproto_channelid].internalrunnerreadingcounter = 0;
			c = microproto_config[microproto_channelid].fgetc();
		} else
			microproto_config[microproto_channelid].internalrunnerreadingcounter++;
	}
	if(!microproto_config[microproto_channelid].internalhascommand) {
		if(c != MICROPROTO_INVALIDC) {
			microproto_config[microproto_channelid].internallastreadcharisvalid = 1;
			//start reading
			if (!microproto_config[microproto_channelid].internalrunnerreading && ((uint8_t)c == MICROPROTO_STX || (uint8_t)c == MICROPROTO_ACK || (uint8_t)c == MICROPROTO_NAK)) {				
				microproto_config[microproto_channelid].internalrunnerreading = 1;
				microproto_config[microproto_channelid].internalrunnerreadingcmdindex = 2;
				//func reset
				microproto_config[microproto_channelid].internalrunnerpreviuoschar = 0;

				//reset
				microproto_config[microproto_channelid].internalstartchar = c;
				microproto_config[microproto_channelid].internalhascommand = 0;
				microproto_config[microproto_channelid].internalcmd = 0;
				microproto_config[microproto_channelid].internaldatalength = 0;
				memset(microproto_config[microproto_channelid].internaldata, 0, microproto_config[microproto_channelid].datasize);
				microproto_config[microproto_channelid].internalrunnerreadingcounter = 0;
			//reading...
			} else {
				uint8_t r = c;
				if(microproto_config[microproto_channelid].internalrunnerreadingcmdindex > 0) {
					microproto_config[microproto_channelid].internalcmd = (((uint16_t)microproto_config[microproto_channelid].internalcmd)<<(8*microproto_config[microproto_channelid].internalrunnerreadingcmdindex)) + r;
					microproto_config[microproto_channelid].internalrunnerreadingcmdindex--;
				} else {
					//last char was an escape char
					if(microproto_config[microproto_channelid].internalrunnerpreviuoschar == MICROPROTO_ESC) {
						//add the actual read
						microproto_config[microproto_channelid].internaldata[microproto_config[microproto_channelid].internaldatalength] = r;
						microproto_config[microproto_channelid].internaldatalength++;
						if(r == MICROPROTO_ESC)
							microproto_config[microproto_channelid].internalrunnerpreviuoschar = 0;
						else
							microproto_config[microproto_channelid].internalrunnerpreviuoschar = r;					
					} else {
						//escape char, skip the next char
						if(r != MICROPROTO_ESC) {
							//stop reading
							if(r == MICROPROTO_ETX) {
								if(microproto_config[microproto_channelid].internaldatalength > 1) {
									crc = microproto_config[microproto_channelid].internaldata[microproto_config[microproto_channelid].internaldatalength-1]; //get crc

									microproto_config[microproto_channelid].internaldata[microproto_config[microproto_channelid].internaldatalength-1] = 0; //set end char
									microproto_config[microproto_channelid].internaldatalength = microproto_config[microproto_channelid].internaldatalength-1;

									//compute crc
									crcc = microproto_computecrc(crcc, (uint8_t)(microproto_config[microproto_channelid].internalcmd>>8));
									crcc = microproto_computecrc(crcc, (uint8_t)(microproto_config[microproto_channelid].internalcmd>>0));
									for(uint16_t i=0; i<microproto_config[microproto_channelid].internaldatalength; i++)
										crcc = microproto_computecrc(crcc, (uint8_t)(microproto_config[microproto_channelid].internaldata[i]));

									microproto_config[microproto_channelid].internalrunnerreading = 0;
									microproto_config[microproto_channelid].internalrunnerreadingcmdindex = 0;

									checkcommand = 1;
								} else {
									microproto_config[microproto_channelid].internalrunnerreading = 0;
									//func reset
									microproto_config[microproto_channelid].internalrunnerreadingcmdindex = 0;
									microproto_config[microproto_channelid].internalrunnerpreviuoschar = 0;

									//reset
									microproto_config[microproto_channelid].internalstartchar = 0;
									microproto_config[microproto_channelid].internalhascommand = 0;
									microproto_config[microproto_channelid].internalcmd = 0;
									microproto_config[microproto_channelid].internaldatalength = 0;
									memset(microproto_config[microproto_channelid].internaldata, 0, microproto_config[microproto_channelid].datasize);
									microproto_config[microproto_channelid].internalrunnerreadingcounter = 0;
								}							
							//add the char
							} else {
								microproto_config[microproto_channelid].internaldata[microproto_config[microproto_channelid].internaldatalength] = r;
								microproto_config[microproto_channelid].internaldatalength++;
							}
						}
						microproto_config[microproto_channelid].internalrunnerpreviuoschar = r;
					}
					if(microproto_config[microproto_channelid].internaldatalength > microproto_config[microproto_channelid].datasize+1) {
						microproto_config[microproto_channelid].internalrunnerreading = 0;
						//func reset
						microproto_config[microproto_channelid].internalrunnerreadingcmdindex = 0;
						microproto_config[microproto_channelid].internalrunnerpreviuoschar = 0;

						//reset
						microproto_config[microproto_channelid].internalstartchar = 0;
						microproto_config[microproto_channelid].internalhascommand = 0;
						microproto_config[microproto_channelid].internalcmd = 0;
						microproto_config[microproto_channelid].internaldatalength = 0;
						memset(microproto_config[microproto_channelid].internaldata, 0, microproto_config[microproto_channelid].datasize);
						microproto_config[microproto_channelid].internalrunnerreadingcounter = 0;
					}
				}
			}
		} else {
			microproto_config[microproto_channelid].internallastreadcharisvalid = 0;
			return 0;
		}
	} else {
		return 0;
	}		

	if(checkcommand) {
		if(crcc == MICROPROTO_ETX || crcc == MICROPROTO_ESC)
			crcc = 0;

		//check crc
		if(crcc == crc) {
			microproto_config[microproto_channelid].internalhascommand = 1;
		} else {
#if MICROPROTO_ENABLECRCCHECKONGETCOMMAND == 0
			microproto_config[microproto_channelid].internalhascommand = 1;
#endif
		}
	}

	return 1;
}


/**
 * get last command readed
 */
uint8_t microproto_getlastcommand(uint16_t *cmd, char *data, int *datalength) {
	//reset input
	memset(data, 0, microproto_config[microproto_channelid].datasize);

	uint8_t ret = microproto_config[microproto_channelid].internalhascommand;

	//set last command
	if(microproto_config[microproto_channelid].internalhascommand) {
		microproto_config[microproto_channelid].internalhascommand = 0;
		*cmd = microproto_config[microproto_channelid].internalcmd;
		*datalength = microproto_config[microproto_channelid].internaldatalength;
		memcpy(data, microproto_config[microproto_channelid].internaldata, microproto_config[microproto_channelid].datasize);
	}

	return ret;
}


/**
 * get command
 */
uint8_t microproto_getcommand(uint16_t *cmd, char *data, int *datalength) {
	uint8_t tryread = 0;

	//disable command runner from external call
	microproto_disablegetcommandrunner = 1;

	while(!microproto_config[microproto_channelid].internalhascommand) {
#if MICROPROTO_WATCHDOGENABLED == 1
		MICROPROTO_RESETWATCHDOG;
#endif
		//get command
		microproto_getcommandrunnerint();

		if(!microproto_config[microproto_channelid].internallastreadcharisvalid) {
			delay_us(MICROPROTO_WAITUSNOTREADC);
			tryread++;
			if(tryread > MICROPROTO_MAXTRYREAD)
				break;
		} else
			delay_us(MICROPROTO_WAITUSREADC);
	}

	//enable command runner from external call
	microproto_disablegetcommandrunner = 0;

	return microproto_getlastcommand(cmd, data, datalength);
}


/**
 * send command
 */
uint8_t microproto_sendcommandcmd(uint16_t cmd, char *data, int datalength, uint8_t waitforack, uint8_t ackcmd) {
	uint16_t i = 0;

	//get length with escape character update
	uint16_t length = datalength;
	for(uint8_t i=0; i<datalength; i++) {
		if(data[i] == MICROPROTO_ETX || data[i] == MICROPROTO_ESC)
			length++;
	}

	//check length
	if(length + 2 > microproto_config[microproto_channelid].datasize)
		return 0;

	//build crc
	uint8_t crcc = 0;
	crcc = microproto_computecrc(crcc, (uint8_t)(cmd>>8));
	crcc = microproto_computecrc(crcc, (uint8_t)(cmd>>0));
	for(i=0; i<datalength; i++) {
		crcc = microproto_computecrc(crcc, (uint8_t)data[i]);
	}
	if(crcc == MICROPROTO_ETX || crcc == MICROPROTO_ESC)
		crcc = 0;

	//send command
	if(ackcmd == MICROPROTO_ACK)
		microproto_config[microproto_channelid].fputc(MICROPROTO_ACK);
	else if(ackcmd == MICROPROTO_NAK)
		microproto_config[microproto_channelid].fputc(MICROPROTO_NAK);
	else
		microproto_config[microproto_channelid].fputc(MICROPROTO_STX);
	microproto_config[microproto_channelid].fputc((uint8_t)(cmd>>8));
	microproto_config[microproto_channelid].fputc((uint8_t)(cmd<<0));
	for(i=0; i<datalength; i++) {
		//add escape character
		if(data[i] == MICROPROTO_ETX || data[i] == MICROPROTO_ESC)
			microproto_config[microproto_channelid].fputc(MICROPROTO_ESC);
		microproto_config[microproto_channelid].fputc(data[i]);
	}
	microproto_config[microproto_channelid].fputc(crcc);
	microproto_config[microproto_channelid].fputc(MICROPROTO_ETX);

	if(waitforack) {
		uint8_t hasack = 0;
		
		uint16_t cmdt = 0;
		int datalengtht = 0;
		char *datat = malloc(microproto_config[microproto_channelid].datasize*sizeof(char));

		for(i=0; i<MINICOM_WAITFORACKUS; i++) {
#if MICROPROTO_WATCHDOGENABLED == 1
			MICROPROTO_RESETWATCHDOG;
#endif
			if(microproto_getcommand(&cmdt, datat, &datalengtht)) {
				if(microproto_config[microproto_channelid].internalstartchar == MICROPROTO_ACK && memcmp(datat, data, datalength) == 0)
					hasack = 1;
				break;
			}
			delay_us(1);
		}

		free(datat);

		//reset input
		memset(data, 0, microproto_config[microproto_channelid].datasize);

		//reset input
		memset(data, 0, microproto_config[microproto_channelid].datasize);

		//check the back ack
		return hasack;
	} else {
		//reset input
		memset(data, 0, microproto_config[microproto_channelid].datasize);

		return 1;
	}

	return 0;
}


/**
 * send command
 */
uint8_t microproto_sendcommand(uint16_t cmd, char *data, int datalength, uint8_t waitforack) {
	return microproto_sendcommandcmd(cmd, data, datalength, waitforack, 0);
}


/**
 * send ack
 */
void microproto_sendack(uint16_t cmd, char *data, int datalength) {
	microproto_sendcommandcmd(cmd, data, datalength, 0, MICROPROTO_ACK);
}


/**
 * send nak
 */
void microproto_sendnak(uint16_t cmd, char *data, int datalength) {
	microproto_sendcommandcmd(cmd, data, datalength, 0, MICROPROTO_NAK);
}


#if MICROPROTO_MODE == MICROPROTO_MODEBLOCK
/**
 * get command delay to prevent the uart port contiuos reading
 */
void microproto_getcommanddelay() {
	delay_ms(MICROPROTO_WAITMSDELAY);
}
#endif


#if MICROPROTO_MODE == MICROPROTO_MODETIMED
/**
 * get command by single char reading
 */
void microproto_getcommandrunner() {
	if(microproto_disablegetcommandrunner)
		return;

	//repeat read if a char is input
	uint8_t readchar = microproto_config[microproto_channelid].continousreadchars;
	while(readchar > 0) {
		if(microproto_getcommandrunnerint())
			readchar--;
		else
			break;
	}
}
#endif
