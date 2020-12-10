#include "nrf5xradio.h"

nBlock_nRF5xRadio::nBlock_nRF5xRadio (        
        uint32_t mode, uint32_t radioRate, uint8_t frequency, uint32_t payloadLength,
        uint8_t address0, uint8_t address1, uint8_t address2, uint8_t address3, uint8_t address4):
            _config(), _radio() {
    
    _address.A0 = address0;
    _address.A1 = address1;
    _address.A2 = address2;
    _address.A3 = address3;
    _address.A4 = address4;
    _config.frequency = frequency;
    _config.rate = radioRate;
    _config.data_length = payloadLength;
    
    _mode = mode;
    uint32_t radio_mode = (mode == RADIO_MODE_TX_ONLY)? RADIO_MODE_TX : RADIO_MODE_RX;
    _radio.Setup(radio_mode, _address, _config);
    
    _tx_updated = 0;
    
    outputType[0] = OUTPUT_TYPE_ARRAY;
    for (int i=0; i<32; i++) {
        _tx_buffer[i] = 0;
        _rx_buffer[i] = 0;
    }
}
void nBlock_nRF5xRadio::triggerInput(nBlocks_Message message) {
    // Ignore inputs if we are RX only
    if (_mode == RADIO_MODE_RX_ONLY) return;
    
    switch (message.dataType) {
        case OUTPUT_TYPE_INT:
        case OUTPUT_TYPE_FLOAT:
            break;

        case OUTPUT_TYPE_STRING:
            strcpy(_tx_buffer, message.stringValue);
            _tx_updated = 1;
            break;

        case OUTPUT_TYPE_ARRAY:
            char * data_array = (char *)(message.pointerValue);
            for (uint32_t i=0; i<message.dataLength; i++) {
                _tx_buffer[i] = data_array[i];
            }
            _tx_updated = 1;
            break;
            
    }
}
void nBlock_nRF5xRadio::endFrame(void) {
    // If we are a receiver, we check for incoming data first
    // so it is not lost by becoming transmitter
    if (_radio.Receive(_rx_buffer)) {
        output[0] = (uint32_t)(&_rx_buffer);
        available[0] = _config.data_length;
    }
    
    // If we have to transmit, the buffer is currently holding the data
    if (_tx_updated) {
        _tx_updated = 0;
        _radio.Transmit(_tx_buffer);
    }
    

    return;
}

