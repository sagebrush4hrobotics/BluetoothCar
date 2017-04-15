http://www.martyncurrey.com/connecting-2-arduinos-by-bluetooth-using-a-hc-05-and-a-hc-06-pair-bind-and-link/

Connect HC-05 and HC-06 to 5V and GND (May need to press or hold button on HC-05 during power on)
Connect HC-05 RX to Pin 3 and TX to Pin 2
Leave HC-06 RX and TX disconnected
Connect to HC-05 at 38400 Baud using BTSerial-AT-Mode.ino
Open Serial Monitor, set to Both NL&CR, 9600 Baud
Type the following commands:

AT+RMAAD		clears any previously paired devices.
AT+ROLE=1		puts the HC-05 in Master Mode
AT+RESET		reset the HC-05. This is sometimes needed after changing roles.
AT+CMODE=0		allows the HC-05 to connect to any device
AT+INQM=0,5,9		set inquiry to search for up to 5 devices for 9 seconds
AT+INIT			initiates the SPP profile. If SPP is already active you will get an error(17) which you can ignore.
AT+INQ			searches for other Bluetooth devices.

Sample output from AT+INQ:
+INQ:A854:B2:3FB035,8043C,7FFF
+INQ:3014:10:171179,1F00,7FFF

To ensure we are connecting to the HC-06, use this format:
AT+RNAME?A854,B2,3FB035

Output should look like this:
+RNAME:HC-06_1

If not, try next address

To pair and bind, sample entry:
AT+PAIR=3014,10,171179,9
AT+BIND=3014,10,171179
AT+CMODE=1
AT+LINK=3014,10,171179

The LED on the HC-05 should have 2 quick blinks every 2 seconds (or so).
The LED on the HC-06 should be on (not blinking).
You have now connected the HC-05 and the HC-06