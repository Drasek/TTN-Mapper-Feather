# Code for a TTN-Mapper based on the Adafruit Feather M0 LoRa + Ultimate GPS FeatherWing

Some Code adapted from the Node Building Workshop using a modified LoraTracker board
Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman

This uses OTAA (Over-the-air activation), in the ttn_secrets.h a DevEUI, a AppEUI and a AppKEY is configured, which are used in an over-the-air activation procedure where a DevAddr and session keys are assigned/generated for use with all further communication.

To use this sketch, first register your application and device with the things network, to set or generate an AppEUI, DevEUI and AppKey. Multiple devices can use the same AppEUI, but each device has its own DevEUI and AppKey. Do not forget to adjust the payload decoder function.

This sketch will send Battery Voltage (in mV), the location (latitude, longitude and altitude) and the hdop using the lora-serialization library matching setttings have to be added to the payload decoder funtion in the The Things Network console/backend.

In the payload function change the decode function, by adding the code from https://github.com/thesolarnomad/lora-serialization/blob/master/src/decoder.js to the function right below the "function Decoder(bytes, port) {" and delete everything below exept the last "}". Right before the last line add this code
```
switch(port) {    
  case 1:
    loraserial = decode(bytes, [uint16, uint16, latLng, uint16], ['vcc', 'geoAlt', 'geoLoc', 'hdop']);   
    values = {         
      lat: loraserial["geoLoc"][0],         
      lon: loraserial["geoLoc"][1],         
      alt: loraserial["geoAlt"],         
      hdop: loraserial["hdop"]/1000,         
      battery: loraserial['vcc']       
    };       
    return values;     
  default:       
    return bytes;
```
and you get a json containing the stats for lat, lon, alt, hdop and battery

# Licence:
GNU Affero General Public License v3.0

# NO WARRANTY OF ANY KIND IS PROVIDED.
