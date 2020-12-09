#include "nRFBareRadio.h"

/*
#ifdef TARGET_NRF52
#define RADIO_RECEIVE_EVENT   EVENTS_CRCOK
#else
#define RADIO_RECEIVE_EVENT   EVENTS_END
#endif*/

RadioConfig::RadioConfig() {
    frequency = 2;                      
    rate = RADIO_RATE_2M;
    tx_power = RADIO_TX_0dBm;
    data_length = 32;                   // compatible to nRF24 (range 1-32)
    address_length = 5;                 // compatible to nRF24 (range 3-5)
    use_whitening = RADIO_NO_WHITENING; // compatible to nRF24 (no change)
    endianness = RADIO_BIGENDIAN;       // compatible to nRF24 (no change)
    crc_poly = 0x1021;                  // compatible to nRF24 (no change)
    crc_init = 0xFFFF;                  // compatible to nRF24 (no change)
}



// Partially based on a version by Manuel Caballero
// https://os.mbed.com/users/mcm/

BareRadio::BareRadio() {
    data_len = 32;
    for (int i=0; i<data_len; i++) packet[i] = 0;
    ConfigClock();
    LastError = 0;
}

int BareRadio::ConfigClock() {
    int i;
    
    NRF_CLOCK->EVENTS_HFCLKSTARTED  =   0; // flag
    NRF_CLOCK->TASKS_HFCLKSTART     =   1; // start task

    for (i=0; i<10000; i++) {
        if (NRF_CLOCK->EVENTS_HFCLKSTARTED) return 1;
        wait_us(10);
    }
    return 0; // could not configure timer in 100ms
}

void BareRadio::Setup(int mode, RadioAddress address, RadioConfig& config) {
    // According to datasheet, power off and on causes a reset on all registers
    NRF_RADIO->POWER = ( RADIO_POWER_POWER_Disabled << RADIO_POWER_POWER_Pos ); // POWER OFF
    wait_us(10);
    NRF_RADIO->POWER = ( RADIO_POWER_POWER_Enabled  << RADIO_POWER_POWER_Pos ); // POWER ON

    // calculate the BASE0 length based on the total address length
    // total address length must be in the range [3,5]
    int BASE0_len = config.address_length - 1;
    if (BASE0_len < 2) BASE0_len = 2;
    if (BASE0_len > 4) BASE0_len = 4;
    
    data_len = config.data_length;

    // Configure data rate (0=1M, 1=2M, 2=250K, 3=BLE1M)
    NRF_RADIO->MODE  = ( config.rate << RADIO_MODE_MODE_Pos );

    // Configure packet: NO S0,S1 or Length fields & 8-bit preamble. 
    // Compatible to nRF24 without dynamic payload
    NRF_RADIO->PCNF0 = ( 0                            << RADIO_PCNF0_LFLEN_Pos  ) |
                       ( 0                            << RADIO_PCNF0_S0LEN_Pos  ) |
                       ( 0                            << RADIO_PCNF0_S1LEN_Pos  )
    #ifdef TARGET_NRF52
                     | ( RADIO_PCNF0_S1INCL_Automatic << RADIO_PCNF0_S1INCL_Pos )
                     | ( RADIO_PCNF0_PLEN_8bit        << RADIO_PCNF0_PLEN_Pos   )
    #endif
                     ;

    // Configure static payload length 
    NRF_RADIO->PCNF1 = ( config.data_length           << RADIO_PCNF1_MAXLEN_Pos) |  // this is to truncate the packet
                       ( config.data_length           << RADIO_PCNF1_STATLEN_Pos) | // this is to configure the radio
                       ( BASE0_len                    << RADIO_PCNF1_BALEN_Pos) |
                       ( config.endianness            << RADIO_PCNF1_ENDIAN_Pos) |
                       ( config.use_whitening         << RADIO_PCNF1_WHITEEN_Pos);

    // Whitening value
    NRF_RADIO->DATAWHITEIV = ( ( 0x55 & RADIO_DATAWHITEIV_DATAWHITEIV_Msk ) << RADIO_DATAWHITEIV_DATAWHITEIV_Pos );

    // Configure address Prefix0 and Base0
    unsigned int addr[5] = {address.A0, address.A1, address.A2, address.A3, address.A4};
    unsigned int base_address = 0;
    int i = 0;
    while (i<BASE0_len) {
        base_address |= (addr[i] << (i*8));
        i++;
    }
    NRF_RADIO->BASE0   = base_address;
    NRF_RADIO->PREFIX0 = ( ( addr[i] & RADIO_PREFIX0_AP0_Msk ) << RADIO_PREFIX0_AP0_Pos );

    // Initialize CRC ( two bytes )
    // Includes address field for nRF24 compatibility
    NRF_RADIO->CRCCNF    =   (RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos) |
                           (RADIO_CRCCNF_SKIPADDR_Include << RADIO_CRCCNF_SKIPADDR_Pos);

    NRF_RADIO->CRCPOLY   =   ( config.crc_poly     << RADIO_CRCPOLY_CRCPOLY_Pos );
    NRF_RADIO->CRCINIT   =   ( config.crc_init     << RADIO_CRCINIT_CRCINIT_Pos );

    #ifdef TARGET_NRF52
    // Enable fast rampup, new in nRF52
    NRF_RADIO->MODECNF0  =   ( RADIO_MODECNF0_DTX_B0  << RADIO_MODECNF0_DTX_Pos ) |
                           ( RADIO_MODECNF0_RU_Fast << RADIO_MODECNF0_RU_Pos  );
    #endif

    NRF_RADIO->FREQUENCY =   ( ( config.frequency & RADIO_FREQUENCY_FREQUENCY_Msk ) << RADIO_FREQUENCY_FREQUENCY_Pos );      // Frequency = 2400 + FREQUENCY (MHz).


    // Configure address of the packet and logic address to use
    NRF_RADIO->PACKETPTR =   (unsigned int)&packet[0]; // Almost the same as (unsigned int)packet,
                                                     // but sizeof() will return normal pointer size

    // Transmit to address 0
    NRF_RADIO->TXADDRESS = ( 0 << RADIO_TXADDRESS_TXADDRESS_Pos);

    // Output Power: 0dBm  @2400MHz
    NRF_RADIO->TXPOWER   =   ( config.tx_power << RADIO_TXPOWER_TXPOWER_Pos );
                           
  // TX
  if (mode == RADIO_MODE_TX) {
        
        // Disable receiving address
        NRF_RADIO->RXADDRESSES = ( RADIO_RXADDRESSES_ADDR0_Disabled << RADIO_RXADDRESSES_ADDR0_Pos );

        // Configure shortcuts.
        NRF_RADIO->SHORTS    =   ( RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos ) |
                                 ( RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos );
  }
  
    // RX
    if (mode == RADIO_MODE_RX) {
        // Use logical address 0 (BASE0 + PREFIX0 byte 0)
        NRF_RADIO->RXADDRESSES = ( RADIO_RXADDRESSES_ADDR0_Enabled << RADIO_RXADDRESSES_ADDR0_Pos );

        // Configure shortcuts.
        NRF_RADIO->SHORTS    =   ( RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos ) |
                                 ( RADIO_SHORTS_END_START_Enabled << RADIO_SHORTS_END_START_Pos );
                                 
        NRF_RADIO->TASKS_RXEN = 1; // Enable receiver
    }                           
}


void BareRadio::Transmit(char * data) {
    Transmit((unsigned char *)data);
}
void BareRadio::Transmit(unsigned char * data) {
    // data must be an array of (unsigned) char at least data_len elements wide
    for (int i=0; i<data_len; i++) packet[i] = data[i];
    
    // Transmits one packet
    NRF_RADIO->TASKS_TXEN                   =    1; // Enable transmitter
    while ( ( NRF_RADIO->EVENTS_DISABLED )  == 0 ); // Wait until transmission complete
    NRF_RADIO->EVENTS_DISABLED              =    0; // Clear flag
    // Time on air: 150 us      

}

int BareRadio::Receive(char * data) {
    return Receive((unsigned char *) data);
}
int BareRadio::Receive(unsigned char * data) {
    // returns true if a packet has arrived and data was replaced
    if ( NRF_RADIO->EVENTS_END == 1 ) {
        NRF_RADIO->EVENTS_END = 0;
        if (NRF_RADIO->CRCSTATUS) {
            for (int i=0; i<data_len; i++) data[i] = packet[i];
            return 1;
        }
        else LastError = 1;
    }
    return 0;
    
}
