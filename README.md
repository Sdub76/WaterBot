# WaterBot
ESP8266-based Tree Water Monitor
Tree Watering Robot created to keep the Christmas tree wet without having to refill it everyday.  

![Fritizing Image](/images/WaterBot.png)


## Features
1.  Monitor tree water level
2.  Monitor supply water level
3.  Pump water when tree water is low
4.  Give visual indication (via light on tree) that the pump is running
5.  Give visual indication (via light on tree) that the supply is empty
6.  Give visual indication (via light on tree) that there's a hardware fault
7.  (Disabled) Only turn on tree sensors when periodic readings are taking place to avoid sensor degradation via electrolysis
8.  Provide local debugging via OLED screen 
9.  (TBD) Log data via MQTT (Wi-fi) to see if things are working


NOTE: The original intent was to excite the tree level sensors with a digital output pin of the controller, so that the "on" time would be minimized, thus prolonging the life of the sensor probes.  The sensor probes electrolyze the water and erode the contacts when they're energized... thus you should only turn them on for a brief period if you're only taking periodic readings.  Unfortunately, I didn't have any spare GPIO pins on the ESP that weren't also used for bootloader control, so I commented out this code.  I suppose the sensors will work fine for a few weeks during the holiday season :)

![Control Board](/images/control_board.jpg)
![Tree Simulation](/images/tree_simulation.jpg)

## Shopping List
* 1	Adafruit Feather HUZZAH ESP8266	part # [Adafruit #2821](https://www.adafruit.com/product/2821)
* 1	Adafruit OLED FeatherWing	part # [Adafruit #2900](https://www.adafruit.com/product/2900)
* 2	FC-28 Soil Hygrometer Module/Probe (http://a.co/d/3ILjnXG)
* 1	12VDC Diaphram pump	(http://a.co/d/7KirTrF)
* 1	Logic Level Converter	(https://www.microcenter.com/product/476364/velleman-33v-5v-ttl-logic-level-converter-module)
* 1	2-Channel 5v Relay Module (http://a.co/d/bQ4BVRI)
* 1	12VDC Power Supply (http://a.co/d/5liJUTH)
* 1	Blue (430nm) LED
* 1 220Ω Resitor for LED
* 1	1MΩ Resistor for Analog input voltage divider
* 1	390kΩ Resistor for Analog input voltage divider
