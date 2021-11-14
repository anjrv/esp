STEPS:

1. Change the setting to match you configuration: main/main.c
--- 
#define  WIFI_SSID "TOL103M"
#define  WIFI_PASS "InternetOfThings"
#define  MY_LATITUDE   641360  // = round(10000*lat)
#define  MY_LONGITUDE -219478
---
  a) WIFI settings according to your own network
  b) Lat/Lon information you can easily get, e.g., using Google map.
     For privacy reasons, you can choose some location nearby, or simply lie :)

2. Compile the binary:  idf.py build

3. Attach the DHT22 sensor as instructed (see the photos posted to Ed)
   - +3.3V and GND can be borrowed from the board (2 pins near the "boot")
   - Signal is (by default) via GPIO D18, that is located in the same row as Vcc & Gnd.
     Skip 6 pins from the Gnd up, plug in, and there should be another 6 pins
     free on the other side. This "rule" applies to the "Black" device.

4. Upload the binary:     idf.py -p /dev/ttyUSB0 flash
5. Monitor its execution: idf.py -p /dev/ttyUSB0 monitor

6. Check if your device appears to http://iot.rhi.hi.is:8020/nov/
