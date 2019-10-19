# Easy-Matrix-Message-Wifi
display messages from web server to led matrix

This is a fork from "http://embedded-lab.com/blog/wifi-enabled-scrolling-led-matrix-display/"
(you will also need https://github.com/markruys/arduino-Max72xxPanel librairies)

Changed : 
- CS pin (GPIO0 -> GPIO15, for compatibility and proximity reason, if you want to change it, in code : "int pinCS = ")
- default speed for rolling text (too slow ^^, in code : "int wait = 80;")

Added : 
- control of animation (stopanim ; 0 by default, 1 )
	-> http://ESP-IP/msg?stopanim=1&msg=message 
- control of intensity (level ; 8 by default, [0-15])
	-> http://ESP-IP/msg?level=0
- control of scrolling speed (speed ; 250 by default, [0-x])
	-> http://ESP-IP/msg?speed=75
- a few commands :
	- showtime : automatic NTP synced display of time with 3 display modes (displaymode=1 2 or 3)
	   - displaymode : (1-3) 1= 'HH:mm' still mode, 2= 'HH:mm:ss' still mode, 3= 'day, HH:mm' rolling mode
		-> http://ESP-IP/msg?cmd=showtime&displaymode=2
	   	*by default, at startup, ESP wil display its @IP (rolling on led screen) 4 times then will display time in mode 2 (centered HH:mm:ss)
	- restart : reboots the ESP (equiv to reset button, usefull when display is not complete or blank after boot up)
		-> http://ESP-IP/msg?cmd=restart
	- forcentp : forces ntp client update
		-> http://ESP-IP/msg?cmd=forcentp
- list of parameters on the ESP html page (http://ESP-IP/)
