# Easy-Matrix-Message-Wifi
display messages from web server to led matrix

This is a fork from "http://embedded-lab.com/blog/wifi-enabled-scrolling-led-matrix-display/"
(you will also need https://github.com/markruys/arduino-Max72xxPanel librairies)

Added : 
- control of animation (stopanim ; 0 by default, 1 )
	-> http://ESP-IP/msg?stopanim=1&msg=message 
- control of intensity (level ; 8 by default, [0-15])
	-> http://ESP-IP/msg?level=0&msg=message
	