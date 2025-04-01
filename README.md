# About the project

The project is designed to create an RS-485 listener for the interface between lithium high-power 
 batteries and inverters using the PylonTech protocol and send information to a higher-level controller (MQTT broker).

# How the project appeared

The integrator company installing Chinese solar panel and inverter equipment categorically
 refused to provide an interface for determining battery charge. Due to the fact that it is
 impossible to control the load without knowing the battery charge level, it was necessary
 to develop a method for passively listening to the RS-485 bus and decrypting packets transmitted over the bus.

The general idea is to place an esp32-based probe running FreeRTOS on the RS-485 bus via a
 simple RS-485-TTL adapter. The probe passively listens to the bus and, upon receiving packets
 about the state of the batteries, decodes them and transmits them to the MQTT broker, where
 interested consumers can already use this information.

The developed solution does not transmit any own commands to the bus, does not poll the devices, but
 only collects information that the battery-powered BMS reports in response to requests from the master of the bus.

The probe expects packets with a very specific header type: cid1=0x46, cid2=0x00. That is,
 the BMS response to the status request. Error code is 0x00.

# About the author

If this project was useful to you, I ask you to help me find interesting projects or jobs. 

Ready to move if necessary. Thank you.