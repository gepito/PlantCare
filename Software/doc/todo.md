
- software 
	- eliminate 'while (Serial1.available())...' stuff from the main loop (eg. put in setup() section)
		most probably this part is responsible for measurement dropouts

- modify schematics
	- A3 is connected to both sensors (x6-2, x6-4)
	- swap D8/D9, D6/D7
	- add usb-serial module (D0, D1)
	- add SD/Calendar module
	- (D2, D3) > (A4, A5) deal with different pinning of SD/Calendar shield
	
	
- plantcare (the system)
    v - change clock battery
    v - set clock
    v - serial output to D0/D1
    v - serial terminal implementation
    v - empty SD card
	
v - log each pump event (debug overpumping)	
