/*******************************************************************************
 * ttn-mapper for Adafruit Feather M0 LoRa + Feather Ultimate GPS
 * 
 * Code adapted from the Node Building Workshop using a modified LoraTracker board
 * 
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 * 
 * This code runs on an Adafruit Feather M0 with LoRa, status displayed on 
 * Adafruit OLED FeatherWing
 * 
 * This uses OTAA (Over-the-air activation), in the ttn_secrets.h a DevEUI,
 * a AppEUI and a AppKEY is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 * 
 * To use this sketch, first register your application and device with
 * the things network, to set or generate an AppEUI, DevEUI and AppKey.
 * Multiple devices can use the same AppEUI, but each device has its own
 * DevEUI and AppKey. Do not forget to adjust the payload decoder function.
 * 
 * This sketch will send Battery Voltage (in mV), the location (latitude, 
 * longitude and altitude) and the hdop using the lora-serialization library 
 * matching setttings have to be added to the payload decoder funtion in the
 * The Things Network console/backend.
 * 
 * In the payload function change the decode function, by adding the code from
 * https://github.com/thesolarnomad/lora-serialization/blob/master/src/decoder.js
 * to the function right below the "function Decoder(bytes, port) {" and delete
 * everything below exept the last "}". Right before the last line add this code
 * switch(port) {    
 *   case 1:
 *     loraserial = decode(bytes, [uint16, uint16, latLng, uint16], ['vcc', 'geoAlt', 'geoLoc', 'hdop']);   
 *     values = {         
 *       lat: loraserial["geoLoc"][0],         
 *       lon: loraserial["geoLoc"][1],         
 *       alt: loraserial["geoAlt"],         
 *       hdop: loraserial["hdop"]/1000,         
 *       battery: loraserial['vcc']       
 *     };       
 *     return values;     
 *   default:       
 *     return bytes;
 * and you get a json containing the stats for lat, lon, alt, hdop and battery
 * 
 * Licence:
 * GNU Affero General Public License v3.0
 * 
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *******************************************************************************/
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <LoraEncoder.h>
#include <LoraMessage.h>
#include <NMEAGPS.h>
#include <Adafruit_SleepyDog.h>
#include "ttn_secrets.h"

//#define DEBUG

// GPS variables
static const uint8_t RXPin = 9, TXPin = 8;
static const uint32_t GPSBaud = 9600;
double geoLng = 0;
double geoLat = 0;
int geoAlt = 0;
int geoHdop = 0;
int port = 1;
NMEAGPS  gps;
gps_fix  fix;

// Battery Pin to read Voltage
const uint8_t battery_pin = A7;

// LoRaWAN Sleep / Join variables
int sleepcycles = 1;  // every sleepcycle will last 16 secs, total sleeptime will be sleepcycles * 16 sec
bool joined = false;
bool sleeping = false;

// LoRaWAN keys
static const u1_t app_eui[8]  = SECRET_APP_EUI;
static const u1_t dev_eui[8]  = SECRET_DEV_EUI;
static const u1_t app_key[16] = SECRET_APP_KEY;

// Getters for LMIC
void os_getArtEui (u1_t* buf) 
{
  memcpy(buf, app_eui, 8);
}

void os_getDevEui (u1_t* buf)
{
  memcpy(buf, dev_eui, 8);
}

void os_getDevKey (u1_t* buf)
{
  memcpy(buf, app_key, 16);
}

// Pin mapping
// The Feather M0 LoRa does not map RFM95 DIO1 to an M0 port. LMIC needs this signal 
// in LoRa mode, so you need to bridge IO1 to an available port -- I have bridged it to 
// Digital pin #11
// We do not need DIO2 for LoRa communication.
const lmic_pinmap lmic_pins = {
  .nss = 8,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 4,
  .dio = {3, 11, LMIC_UNUSED_PIN},
};

static osjob_t sendjob;
static osjob_t initjob;

// Init job -- Actual message message loop will be initiated when join completes
void initfunc (osjob_t* j)
{
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  // Allow 1% error margin on clock
  // Note: this might not be necessary, never had clock problem...
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
  // start joining
  LMIC_startJoining();
}

// Reads battery voltage
double readBatteryVoltage()
{
  double battery = analogRead(battery_pin);
  battery *= 2;    // we divided by 2, so multiply back
  battery *= 3.3;  // Multiply by 3.3V, our reference voltage
  battery /= 1024; // convert to voltage
  return battery;
}

void loop_gps() {
  bool geoTest = false;
  while (!geoTest) {
    while (gps.available(Serial1)) {
      geoTest = true;
      fix = gps.read();
      #ifdef DEBUG
        Serial.print(F("Location: "));
      #endif
      if (fix.valid.location) {
        geoLat = fix.latitude();
        geoLng = fix.longitude();
        #ifdef DEBUG
          Serial.print(geoLat, 6);
          Serial.print(F(", "));
          Serial.print(geoLng, 6);
        #endif
      } else {
        geoTest = false;
        #ifdef DEBUG
          Serial.print(F("no fix"));
        #endif
      }
      #ifdef DEBUG
        Serial.print(F(", Altitude: "));
      #endif
      if (fix.valid.altitude) {
        geoAlt = fix.altitude();
        #ifdef DEBUG
          Serial.print(geoAlt);
        #endif
      } else {
        geoTest = false;
        #ifdef DEBUG
          Serial.print(F("no fix"));
        #endif
      }
      #ifdef DEBUG
        Serial.print(F(", HDOP: "));
      #endif
      if (fix.valid.hdop) {
        geoHdop = fix.hdop;
        #ifdef DEBUG
          Serial.print(geoHdop);
        #endif
      } else {
        geoTest = false;
        #ifdef DEBUG
          Serial.print(F("no fix"));
        #endif
      }
      #ifdef DEBUG
        Serial.println();
      #endif
    }
  }
}

// Send job
static void do_send(osjob_t* j)
{
  // get GPS Position
  loop_gps();

  // get Battery Voltage
  int vccValue = readBatteryVoltage() * 1000;

  // compress the data into a few bytes
  LoraMessage message;
  message
    .addUint16(vccValue)
    .addUint16(geoAlt)
    .addLatLng(geoLat, geoLng)
    .addUint16(geoHdop);
    
  // Check if there is not a current TX/RX job running   
  if (LMIC.opmode & OP_TXRXPEND) {
    #ifdef DEBUG
      Serial.println(F("OP_TXRXPEND, not sending"));
    #endif
  } else {
    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(port, message.getBytes(), message.getLength(), 0);
    #ifdef DEBUG
      Serial.println(F("Sending: "));
    #endif
  }
}

// LoRa event handler
// We look at more events than needed, to track potential issues
void onEvent (ev_t ev) {
  switch(ev) {
    case EV_SCAN_TIMEOUT:
      #ifdef DEBUG
        Serial.println(F("EV_SCAN_TIMEOUT"));
      #endif
      break;
    case EV_BEACON_FOUND:
      #ifdef DEBUG
        Serial.println(F("EV_BEACON_FOUND"));
      #endif
      break;
    case EV_BEACON_MISSED:
      #ifdef DEBUG
        Serial.println(F("EV_BEACON_MISSED"));
      #endif
      break;
    case EV_BEACON_TRACKED:
      #ifdef DEBUG
        Serial.println(F("EV_BEACON_TRACKED"));
      #endif
      break;
    case EV_JOINING:
      #ifdef DEBUG
        Serial.println(F("EV_JOINING"));
      #endif
      break;
    case EV_JOINED:
      #ifdef DEBUG
        Serial.println(F("EV_JOINED"));
      #endif
      // Disable link check validation (automatically enabled
      // during join, but not supported by TTN at this time).
      LMIC_setLinkCheckMode(0);
      joined = true;
      LMIC_setDrTxpow(DR_SF7,14);
      break;
    case EV_RFU1:
      #ifdef DEBUG
        Serial.println(F("EV_RFU1"));
      #endif
      break;
    case EV_JOIN_FAILED:
      #ifdef DEBUG
        Serial.println(F("EV_JOIN_FAILED"));
      #endif
      break;
    case EV_REJOIN_FAILED:
      #ifdef DEBUG
        Serial.println(F("EV_REJOIN_FAILED"));
      #endif
      os_setCallback(&initjob, initfunc);
      break;
    case EV_TXCOMPLETE:
      sleeping = true;
      #ifdef DEBUG
        if (LMIC.dataLen) {
          Serial.print(F("Data Received: "));
          Serial.println(LMIC.frame[LMIC.dataBeg],HEX);
        }
        Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      #endif
      break;
    case EV_LOST_TSYNC:
      #ifdef DEBUG
        Serial.println(F("EV_LOST_TSYNC"));
      #endif
      break;
    case EV_RESET:
      #ifdef DEBUG
        Serial.println(F("EV_RESET"));
      #endif
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      #ifdef DEBUG
        Serial.println(F("EV_RXCOMPLETE"));
      #endif
      break;
    case EV_LINK_DEAD:
      #ifdef DEBUG
        Serial.println(F("EV_LINK_DEAD"));
      #endif
      break;
    case EV_LINK_ALIVE:
      #ifdef DEBUG
        Serial.println(F("EV_LINK_ALIVE"));
      #endif
      break;
    default:
      #ifdef DEBUG
        Serial.println(F("Unknown event"));
      #endif
      break;
  }
}

void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
    delay(250);
    Serial.println(F("Starting"));
  #endif
  // initialize the scheduler
  os_init();

  // Initialize radio
  os_setCallback(&initjob, initfunc);
  LMIC_reset();

  // initialize Serial1 for GPS
  Serial1.begin(9600);
}

void loop() {
  if (joined==false) {
    os_runloop_once();
  }
  else {
    do_send(&sendjob);    // Sent sensor values
    while(sleeping == false) {
      os_runloop_once();
    }
    sleeping = false;
    #ifdef DEBUG
      Serial.print(F("Sleeping for "));
      Serial.print(16000*sleepcycles);
      Serial.println(F("ms"));
    #endif
    for (int i=0;i<sleepcycles;i++) {
      int sleepMS = Watchdog.sleep(16000); // sleep for 16 seconds per sleepcycle
    }
  }
}
