/*
microproto 0.0.2

copyright (c) Davide Gironi, 2020

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#ifndef MICROPROTO_H_
#define MICROPROTO_H_

#if defined(__arm__)
//uncomment line below to enable STM32
#define STM32
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if defined(ARDUINO)
#include <Arduino.h>
#include <stdint.h>
#endif
#if defined(__AVR)
#include <stdint.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#define delay_us(us) _delay_us(us)
#define delay_ms(ms) _delay_ms(ms)
#elif defined(ESP32) || defined(ESP8266)
#define delay_us(us) delayMicroseconds(us)
#define delay_ms(ms) _delay_ms(ms)
#elif defined(STM32)
#define delayUS_ASM(us) do {\
	asm volatile (	"MOV R0,%[loops]\n\t"\
			"1: \n\t"\
			"SUB R0, #1\n\t"\
			"CMP R0, #0\n\t"\
			"BNE 1b \n\t" : : [loops] "r" (16*us) : "memory"\
		      );\
} while(0)
#include "stm32f1xx_hal.h"
#define delay_us(us) delayUS_ASM(us)
#define delay_ms(ms) HAL_Delay(ms)
#endif

//protocol running mode
#define MICROPROTO_MODEBLOCK 0 //get command in function
#define MICROPROTO_MODETIMED 1 //get command in timer
#define MICROPROTO_MODE MICROPROTO_MODETIMED

//set number of times the running function has to be call while idle in order to perform a read
#define MICROPROTO_READINGCOUNTERTHRESHOLD 30
//deley time to wait for an ack
#define MINICOM_WAITFORACKUS 1000
//enable the crc check on get command
#define MICROPROTO_ENABLECRCCHECKONGETCOMMAND 1

#if MICROPROTO_MODE == MICROPROTO_MODEBLOCK
//max number of tryes in read
#define MICROPROTO_MAXTRYREAD 50
//wait between char read in us
#define MICROPROTO_WAITUSNOTREADC 10
//wait after a char is read in us
#define MICROPROTO_WAITUSREADC 10
//delay after each get command used in get command delay
#define MICROPROTO_WAITMSDELAY 500
//check if watchdog is enabled
#define MICROPROTO_WATCHDOGENABLED 0
#if MICROPROTO_WATCHDOGENABLED == 1
//watchdog reset if a watchdog is enabled
#if defined(__AVR) || defined(ARDUINO) || defined(ESP32) || defined(ESP8266)
#include <avr/wdt.h>
#define MICROPROTO_RESETWATCHDOG wdt_reset();
#elif defined(STM32)
#include "main.h"
//define a function in your code to reset the watchdog
#define MICROPROTO_RESETWATCHDOG ResetWatchDog();
#endif
#endif
#endif

#if MICROPROTO_MODE == MICROPROTO_MODETIMED
//max number of tryes in read
#define MICROPROTO_MAXTRYREAD 50
//wait between char read in us
#define MICROPROTO_WAITUSNOTREADC 10
//wait after a char is read in us
#define MICROPROTO_WAITUSREADC 10
#endif

//max char to read if one is read in runner
#define MICROPROTO_DEFAULTCONTINOUSREADCHARS 10

//commands
#define MICROPROTO_STX 0x02
#define MICROPROTO_ETX 0x03
#define MICROPROTO_ACK 0x06
#define MICROPROTO_NAK 0x15
#define MICROPROTO_ESC 0x1B

//invalid char
#define MICROPROTO_INVALIDC 0x9999

//config struct
typedef struct {
	//getc function, read char and return a valid char or MICROPROTO_INVALIDC if not valid
	uint16_t (*fgetc)(void);
	//putc function, write a char
	void (*fputc)(uint8_t);
	//size for readed data, consider 2 bytes for the command
	uint16_t datasize;
	//max char to read if one is read in runner, higher number may slow down reading func
	uint8_t continousreadchars;
	//internal use - data array data
	char *internaldata;
	//internal use - actual data length
	uint16_t internaldatalength;
	//internal use - set if a command exists
	uint8_t internalhascommand;
	//internal use - set the start char
	uint8_t internalstartchar;
	//internal use - command reac
	uint16_t internalcmd;
	//internal use - last read is valid char
	uint8_t internallastreadcharisvalid;
	//internal use - runner reading count for timeout
	uint16_t internalrunnerreadingcounter;
	//internal use - runner is reading
	uint8_t internalrunnerreading;
	//internal use - runner reading command index
	uint8_t internalrunnerreadingcmdindex;
	//internal use - runner previous char readed
	uint8_t internalrunnerpreviuoschar;
} microprotoconfig_t;

//functions
extern void microproto_init(microprotoconfig_t *config, uint8_t configsize);
extern void microproto_setchannel(uint8_t id);
extern uint8_t microproto_getcommand(uint16_t *cmd, char *data, int *datalength);
#if MICROPROTO_MODE == MICROPROTO_MODETIMED
extern uint8_t microproto_getlastcommand(uint16_t *cmd, char *data,
		int *datalength);
#endif
extern uint8_t microproto_sendcommand(uint16_t cmd, char *data, int datalength,
		uint8_t waitforack);
extern void microproto_sendack(uint16_t cmd, char *data, int datalength);
extern void microproto_sendnak(uint16_t cmd, char *data, int datalength);
#if MICROPROTO_MODE == MICROPROTO_MODEBLOCK
extern void microproto_getcommanddelay();
#endif
#if MICROPROTO_MODE == MICROPROTO_MODETIMED
extern void microproto_getcommandrunner();
#endif

#endif
