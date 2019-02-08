flash: flash.o
	c++ flash.o -o flash

flash.o: flash.c
	c++ -x c -c flash.c
