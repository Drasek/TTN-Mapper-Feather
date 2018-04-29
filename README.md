# Code for a TTN-Mapper based on the Adafruit Feather M0 LoRa + Ultimate GPS FeatherWing

Some Code adapted from the Node Building Workshop using a modified LoraTracker board
Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman

# Hardware

![Adafruit Feather M0 LoRa](https://raw.githubusercontent.com/Freie-Netzwerker/TTN-Mapper-Feather/master/images/ttn_mapper_feather01.jpg "Adafruit Feather M0 LoRa")
First you take a Adafruit Feather M0 LoRa and solder female header to it.

![Adafruit Feather M0 LoRa](https://raw.githubusercontent.com/Freie-Netzwerker/TTN-Mapper-Feather/master/images/ttn_mapper_feather02.jpg "Adafruit Feather M0 LoRa")
On the backside you connect IO1 with Pin 11, we will need this later in the software for the LMIC library. Add a uFL connector to the Antenna connectors.

![Adafruit Ultimate GPS FeatherWing](https://raw.githubusercontent.com/Freie-Netzwerker/TTN-Mapper-Feather/master/images/ttn_mapper_feather03.jpg "Adafruit Ultimate GPS FeatherWing")
You solder male headers the Ultimate GPS FeatherWing and connect EN (in the middle next to PPS) with Pin 12.

![Adafruit Ultimate GPS FeatherWing](https://raw.githubusercontent.com/Freie-Netzwerker/TTN-Mapper-Feather/master/images/ttn_mapper_feather04.jpg "Adafruit Ultimate GPS FeatherWing")
Please add some wires (ca. 8cm long) to EN (on the outside next to the headers) and Ground, we need those later for a on/off switch.

![Adafruit Feather combined](https://raw.githubusercontent.com/Freie-Netzwerker/TTN-Mapper-Feather/master/images/ttn_mapper_feather05.jpg "Feather combined")
Stack the featherWIng on top of the Feather and add a battery to the FeatherWing.

![Case inside](https://raw.githubusercontent.com/Freie-Netzwerker/TTN-Mapper-Feather/master/images/ttn_mapper_feather06.jpg "Case inside")
Prepare the case with 2 Pigtails from uFL to SMA (RP) for GPS and LoRaWAN, add a on/off switch.

![Case outside](https://raw.githubusercontent.com/Freie-Netzwerker/TTN-Mapper-Feather/master/images/ttn_mapper_feather07.jpg "Case outside")
I have some suckers on the underside of my case.

![Stuff assembled](https://raw.githubusercontent.com/Freie-Netzwerker/TTN-Mapper-Feather/master/images/ttn_mapper_feather08.jpg "Stuff assembled")
Put everything together, connect the Pigtails to the uFL connectors, add a LiPo accu and the 2 wires to the on/off switch and put everything in the case.

![Finished TTN Mapper](https://raw.githubusercontent.com/Freie-Netzwerker/TTN-Mapper-Feather/master/images/ttn_mapper_feather09.jpg "Finished TTN Mapper")
Thats how the TTN-Mapper-Feather looks finished.

# Software
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
