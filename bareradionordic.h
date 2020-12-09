#ifndef __NB_BARERADIONORDIC
#define __NB_BARERADIONORDIC

#include "nworkbench.h"
#include "nRFBareRadio/nRFBareRadio.h"

#define RADIO_MODE_TX_ONLY 0
#define RADIO_MODE_RX_ONLY 1
#define RADIO_MODE_BIDIR   2

class nBlock_BareRadioNordic: public nBlockSimpleNode<1> {
public:
    nBlock_BareRadioNordic(
        uint32_t mode,
        uint32_t radioRate,
        uint8_t frequency,
        uint32_t payloadLength,
        uint8_t address0,
        uint8_t address1,
        uint8_t address2,
        uint8_t address3,
        uint8_t address4
    );
    void triggerInput(nBlocks_Message message);
    void endFrame(void);
private:
    uint32_t _mode;
    RadioAddress _address;
    RadioConfig _config;
    BareRadio _radio;
    char _tx_buffer[32];
    char _rx_buffer[32];
    int _tx_updated;
};

#endif
