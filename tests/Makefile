APP_NAME := test_satcom_modem_interface
ARDUINO_LIBS := AUnit IridiumSBDi2c
include ../EpoxyDuino/EpoxyDuino.mk
# TODO: need shim for WProgram.h for IridiumSBDi2c, and IridiumSBDi2c also requires -fpermissive.
EXTRA_CPPFLAGS=-I. -fpermissive

test_satcom_modem_interface.o: test_satcom_modem_interface.ino ../satcom-modem-interface/timediff.h ../satcom-modem-interface/modem.h

test_satcom_modem_interface.out: test_satcom_modem_interface.o

runtests: test_satcom_modem_interface.out
	./test_satcom_modem_interface.out
