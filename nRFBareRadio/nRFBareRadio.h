/** 
 *  @file nRFBareRadio.h
 *  @brief nRFBareRadio is a library to use the Radio peripheral in a 
 *  nRF51 or nRF52 Nordic microcontroller in "bare" mode transmitting
 *  raw packets, instead of the usual BLE protocols.
 * 
 *  @author Fernando Cosentino
 *  @author Uses some code from a version by Manuel Caballero
 *  (https://os.mbed.com/users/mcm/)
 *
 *  Transmitter example:
 *  @code
 *  #include "mbed.h"
 *  #include "nRFBareRadio.h"
 *
 *  #ifdef TARGET_NRF52
 *  DigitalOut led1(P0_17); // nRF52
 *  #else
 *  DigitalOut led1(P0_21); // nRF51
 *  #endif
 *
 *  int main(void) {
 *      int i = 0;
 *      char buffer[32]; 
 *
 *      // Address object
 *      RadioAddress address = {0xC0, 0x55, 0x42, 0xBB, 0xC2};
 *
 *      // Configuration object
 *      RadioConfig config;
 *      config.frequency = 10; // 2400 + 10
 *      // change other config parameters here if you please
 *   
 *      // Radio object
 *      BareRadio radio;
 *      radio.Setup(RADIO_MODE_TX, address, config); // This is a transmitter
 *
 *      // Main loop
 *      while(1) {
 *          // Put some data in the buffer     
 *          sprintf(buffer, "Value = %d\n", i++);
 *          // Transmit the buffer
 *          radio.Transmit(buffer);
 *          // Toggle the LED and wait a bit
 *          led1 = !led1;
 *          wait(1.0);
 *      }
 *  }
 *  @endcode
 *
 *  Receiver example:
 *  @code
 *  #include "mbed.h"
 *  #include "nRFBareRadio.h"
 *
 *  #ifdef TARGET_NRF52
 *  Serial pc(P0_6, P0_8);  // nRF52
 *  DigitalOut led1(P0_17); // nRF52
 *  #else
 *  Serial pc(P0_9, P0_11); // nRF51
 *  DigitalOut led1(P0_21); // nRF51
 *  #endif
 *
 *  int main(void) {
 *      int i = 0;
 *      char buffer[32]; 
 *
 *      // Address object
 *      RadioAddress address = {0xC0, 0x55, 0x42, 0xBB, 0xC2};
 *
 *      // Configuration object
 *      RadioConfig config;
 *      config.frequency = 10; // 2400 + 10
 *      // change other config parameters here if you please
 *   
 *      // Radio object
 *      BareRadio radio;
 *      radio.Setup(RADIO_MODE_RX, address, config); // This is a receiver
 *
 *      // Main loop
 *      while(1) {
 *          // Did we receive a packet?
 *          if (radio.Receive(buffer)) {
 *              // Print the packet since the transmitter is sending a string
 *              pc.printf(buffer);
 *              // Toggle the LED
 *              led1 = !led1;
 *          }
 *      }
 *  }
 *  @endcode
 */

#ifndef __NRFBARERADIO
#define __NRFBARERADIO

#include "mbed.h"

#define RADIO_MODE_RX 0
#define RADIO_MODE_TX 1

#define RADIO_RATE_1M   RADIO_MODE_MODE_Nrf_1Mbit
#define RADIO_RATE_2M   RADIO_MODE_MODE_Nrf_2Mbit
#define RADIO_RATE_250K RADIO_MODE_MODE_Nrf_250Kbit

#define RADIO_WHITENING    RADIO_PCNF1_WHITEEN_Enabled
#define RADIO_NO_WHITENING RADIO_PCNF1_WHITEEN_Disabled

#define RADIO_LITTLEENDIAN RADIO_PCNF1_ENDIAN_Little
#define RADIO_BIGENDIAN    RADIO_PCNF1_ENDIAN_Big

#define RADIO_TX_0dBm     RADIO_TXPOWER_TXPOWER_0dBm
#define RADIO_TX_N4dBm    RADIO_TXPOWER_TXPOWER_Neg4dBm
#define RADIO_TX_N12dBm   RADIO_TXPOWER_TXPOWER_Neg12dBm
#define RADIO_TX_N20dBm   RADIO_TXPOWER_TXPOWER_Neg20dBm
#define RADIO_TX_N40dBm   RADIO_TXPOWER_TXPOWER_Neg40dBm
#define RADIO_TX_P4dBm    RADIO_TXPOWER_TXPOWER_Pos4dBm // WARNING: 
                             // +4dBm could be against regulations in some areas


/**
 *  RadioAddress is a struct type for storing transmit and receive addresses,
 *  being used as argument to configure a radio.
 */
typedef struct RadioAddress {
    unsigned char A0;
    unsigned char A1;
    unsigned char A2;
    unsigned char A3;
    unsigned char A4;
} RadioAddress;

/**
 *  RadioConfig holds various setup parameters, being used as argument to 
 *  configure a radio. All have default values, so you can get a radio up and 
 *  running without touching anything.
 */
class RadioConfig {
public:
    RadioConfig();
    
    /** Center frequency the radio will operate at in MHz, with 2400MHz offset
     *  (that is, a value of 35 means the radio will operate at 2435MHz).
     *  Must be in range 0-100 and defaults to 2 (2402MHz). */
    int frequency;
    
    /** Data rate (band width around the center frequency) in MHz.
     *  Possible values are RADIO_RATE_1M, RADIO_RATE_2M (default) 
     *  or RADIO_RATE_250K (deprecated). */
    int rate;
    
    /** Packet payload size in bytes, must be in the range 0-32. This library
     *  uses static payload size only. If your payload varies, use the largest
     *  possible length and leave unused bytes in the packet. */
    int data_length;
    
    /** Length of the address field in bytes, must be in the range 3-5. This
     *  library supports one endpoint only (logic address 0). */
    int address_length;
    
    /** Power level used in transmit mode. Possible values are
     *  RADIO_TX_0dBm (default) for 0dBm, 
     *  RADIO_TX_N4dBm for -4dBm,
     *  RADIO_TX_N12dBm for -12 dBm,
     *  RADIO_TX_N20dBm for -20 dBm,
     *  RADIO_TX_N40dBm for -40 dBm,
     *  RADIO_TX_P4dBm for +4dBm,
     *  or any constant from the nRF SDK 
     *  (example: RADIO_TXPOWER_TXPOWER_Neg8dBm). */
    int tx_power;
    
    /** Either to use data whitening or not. Possible values are RADIO_WHITENING
     *  or RADIO_NO_WHITENING (default). Data whitening is not compatible to 
     *  nRF24 chipsets. */
    int use_whitening;
    
    /** Data endianness for both the address and payload. Possible values are
     *  RADIO_LITTLEENDIAN or RADIO_BIGENDIAN (default). For nRF24 compatibility
     *  it must be RADIO_BIGENDIAN. */
    int endianness;
    
    /** CRC polynomial, fixed at a 16bit length in this library. Default 0x1021
     *  (compatible to nRF24). */
    unsigned int crc_poly;
    
    /** CRC initial value, fixed at a 16bit length in this library.
     *  Default 0xFFFF (compatible to nRF24). */
    unsigned int crc_init;
};

/** 
 *  BareRadio represents and controls the RADIO peripheral in a nRF51 or nRF52
 *  Nordic microcontroller. Should be initialised with a RadioAddress and a
 *  RadioConfig objects.
 */
class BareRadio {
public:
    /** Initialises a BareRadio instance. */
    BareRadio();
    
    /** Configures the high speed clock. You don't have to call this manually
     *  since it's called by the constructor on instantiation. 
     *
     *  @returns 1 if the clock was successfully configured, 0 on task timeout.
     */
    int ConfigClock();
    
    /** Configures the radio at either a transmitter or a receiver,
     *  using a supplied address and configuration objects.
     *
     *  @param mode The radio mode, either RADIO_MODE_RX or RADIO_MODE_TX
     *  @param address A RadioAddress object previously filled
     *  @param config A RadioConfig object previously configured
     */
    void Setup(int mode, RadioAddress address, RadioConfig& config);
    
    /** Transmits a packet using data from a supplied buffer, which must be 
     *  at least as long as the data length configured.
     *
     *  @param data The buffer as array of char or array of unsigned char
     */
    void Transmit(char * data);
    void Transmit(unsigned char * data);
    
    /** Checks if a packet was received, and if received fills the supplied
     *  buffer with the corresponding data. If no packet was received,
     *  the buffer is kept unchanged.
     *
     *  @param data The buffer to be filled as array of char or array of
     *  unsigned char
     *  @returns 1 if a packet was received and the buffer was changed, 
     *  0 otherwise
     */
    int Receive(char * data);
    int Receive(unsigned char * data);
    
    int LastError;
    

private:
    unsigned char packet[32];
    int data_len;
};


























#endif