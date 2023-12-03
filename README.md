# CAN Man In the Middle (CANMIM)

CANMIM is a DIY project which supports reading from vehicle's CAN bus, filtering and injecting data and sending the CAN data to another CAN bus using ESP32 family controllers. The original project was hosted under https://github.com/zeroomega/canmim. However, to better support revised hardware and add new features, the firmware was rewritten and rehosted here.


# Intended use case

For amateur car racer and track enthusists, it is very popular to use laptimer and data loggers like AIM Solo 2 DL to log GPS and CAN data. However, these comercial data loggers/lap timers usually cannot log any data that are not from the CAN bus. For example, aftermarket oil pressure sensor sends out 0-5V analog signals and there isn't any official accessories that can hook it up to an AIM Solo 2 DL device. This CANMIM device solved this issue, by reading the analog signal from the oil pressure sensor using an ADC, and then send out the ADC readings using custom CAN IDs, combined with CAN data from the vehicle's CAN bus, to data logger's CAN input.

This project is mostly tested with Toyota GR86/Subaru BRZ ZD8. But the implementation is generatic and should work with any CAN 2.0B system.

TODO: Explain how to build the project.

TODO: Add hardware schematics.

# LICENSE

This project is currently licensed under MIT license. However, it currently containing redistributed code from Espressif and Adafruit and they are released under different licenses. License files are included in the corresponding directories.