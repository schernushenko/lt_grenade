rem avrdude -C c:\lazertag\conf\avrdude.conf -p m8 -c usbasp -P usb -e -U hfuse:r:hfuse.txt:h -U lfuse:r:lfuse.txt:h
avrdude -C c:\lazertag\conf\avrdude.conf -p m8 -c usbasp -P usb -e -Ulfuse:w:0xe4:m -Uhfuse:w:0xd9:m
rem avrdude -C c:\lazertag\conf\avrdude.conf -p m8 -c usbasp -P usb -e -Ulfuse:w:0xe4:m 