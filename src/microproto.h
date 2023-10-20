/*
microproto 0.0.2

copyright (c) Davide Gironi, 2020

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#if defined(ARDUINO)
#include <Arduino.h>

extern "C" {
    #include "microprotoinc.h"
}

class MicroProto {
    public :
        MicroProto();
        ~MicroProto() = default;

        /**
         * use Serial to put a char
         */
        static void defaultserialputc(uint8_t c) {
            Serial.write(c);
        }

        /**
         * use Serial to get a char
         */
        static uint16_t defaultserialgetc() {
            if(Serial.available() > 0)
                return Serial.read();
            else
                return MICROPROTO_INVALIDC;	
        }

        /**
         * initialize the micro protocol
         */
        MicroProto(microprotoconfig_t *config, uint8_t configsize) {
            microproto_init(config, configsize);
        }

        /**
         * set the active channel
         */
        void setchannel(uint8_t id) {
            microproto_setchannel(id);
        }
        
        /**
         * get command
         */
        uint8_t getcommand(uint16_t *cmd, char *data, int *datalength) {
            return microproto_getcommand(cmd, data, datalength);
        }

        #if MICROPROTO_MODE == MICROPROTO_MODETIMED
        /**
         * get last command readed
         */
        uint8_t getlastcommand(uint16_t *cmd, char *data, int *datalength) {
            return microproto_getlastcommand(cmd, data, datalength);
        }
        #endif

        /**
         * send command
         */
        uint8_t sendcommand(uint16_t cmd, char *data, int datalength, uint8_t waitforack) {
            return microproto_sendcommand(cmd, data, datalength, waitforack);
        }

        /**
         * send ack
         */
        void sendack(uint16_t cmd, char *data, int datalength) {
            microproto_sendack(cmd, data, datalength);
        }

        /**
         * send nak
         */
        void sendnak(uint16_t cmd, char *data, int datalength) {
            microproto_sendnak(cmd, data, datalength);
        }

        #if MICROPROTO_MODE == MICROPROTO_MODEBLOCK
        /**
         * get command delay to prevent the uart port contiuos reading
         */
        void getcommanddelay() {
            microproto_getcommanddelay();
        }
        #endif
        
        #if MICROPROTO_MODE == MICROPROTO_MODETIMED
        /**
         * get command by single char reading
         */
        void getcommandrunner() {
            microproto_getcommandrunner();
        }
        #endif

    private:
};

#else

#include "microprotoinc.h"

#endif
