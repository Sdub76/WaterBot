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
NOTE: The original intent was to excite the tree level sensors with a digital output pin of the controller, so that the "on" time would be minimized, thus prolonging the life of the sensor probes.  The sensor probes electrolyze the water and erode the contacts when they're energized... thus you should only turn them on for a brief period of you're only taking periodic readings.  Unfortunately, I didn't have any spare GPIO pins on the ESP that weren't also used for bootloader control, so I commented out this code.  I suppose the sensors will work fine for a few weeks during the holiday season :)

![Control Board](/images/control_board.png)
![Tree Simulation](/images/tree_simulation.png)

## Shopping List
* 1	Adafruit Feather HUZZAH ESP8266	variant variant 1; part # Adafruit #2821
* 1	Adafruit OLED FeatherWing	variant variant 1; part # Adafruit #2900
* 2	FC-28 Soil Hygrometer Module	chip LM393; variant variant 3 (http://a.co/d/3ILjnXG)
* 2	FC-28 Soil Hygrometer Module - Probe	variant variant 1 (http://a.co/d/3ILjnXG)
* 1	Blue (430nm) LED
* 1	2-Channel 5v Relay Shield	package relay-2ch5v10a
* 1	12VDC Diaphram pump	(http://a.co/d/7KirTrF)
* 1	Logic Level Converter	
* 1	12VDC Power Supply(http://a.co/d/5liJUTH)
* 1	1MΩ Resistor	package THT; resistance 1MΩ; pin spacing 400 mil; tolerance ±5%; bands 4
* 1	390kΩ Resistor	package THT; resistance 390kΩ; pin spacing 400 mil; tolerance ±5%; bands 4
* 1 220Ω Resitor for LED
