# Easy-Matrix-Message-Wifi
display messages from web server to led matrix

This is a fork from "http://embedded-lab.com/blog/wifi-enabled-scrolling-led-matrix-display/"
(you will also need https://github.com/markruys/arduino-Max72xxPanel librairies)

Changed : 
- CS pin (GPIO0 -> GPIO15, for compatibility and proximity reason, if you want to change it, in code : "int pinCS = ")
- default speed for rolling text (too slow ^^, in code : "int wait = 80;")

Added : 
- AP mode (when no wifi access found)
- control of animation (anim ; 1 by default, 0: still text, 1: scrolling text, 2: to do ... )
	-> http://ESP-IP/msg?anim=1&msg=message 
- control of intensity (level ; 8 by default, [0-15])
	-> http://ESP-IP/msg?level=0
- control of scrolling speed (speed ; 250 by default, [0-x])
	-> http://ESP-IP/msg?speed=75
- a few commands :
	- showtime : automatic NTP synced display of time with 3 display modes (displaymode=1 2 or 3)
		- by default, at startup, ESP wil display its @IP (rolling on led screen) 4 times then will display time in mode 2 (centered HH:mm:ss)
		- displaymode : (1-3) 1= 'HH:mm' still mode, 2= 'HH:mm:ss' still mode, 3= 'day, HH:mm' rolling mode
		-> http://ESP-IP/msg?cmd=showtime&displaymode=2
	- restart : reboots the ESP (equiv to reset button, usefull when display is not complete or blank after boot up)
		-> http://ESP-IP/msg?cmd=restart
	- forcentp : forces ntp client update
		-> http://ESP-IP/msg?cmd=forcentp
	- off : turn off display
	- resetwifi : reset wifi connexion info and restarts
	- flash : flashes screen 4x (at limited intensity due to probable tension drop)
- list of parameters on the ESP html page (http://ESP-IP/)
- parameter "ntpofset" to change via url gmt offset (usefull for DST), this value is stored to eeprom
	-> http://ESP-IP/msg?ntpoffset=4 (to set time to GMT+4) 
	-> http://ESP-IP/msg?ntpoffset=-6 (to set time to GMT-6) 

Fixed : 
- letter display error (pb of encoding : unicode on the html side / CP437 on the matrix side)
	-> now displays correctly most accents, °, ², ç