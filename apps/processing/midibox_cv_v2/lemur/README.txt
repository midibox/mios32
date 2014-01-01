$Id$

Lemur Setup
===========

A bidirectional connection is required to get this panel working.


1) USB MIDI
-----------

Works via the Lemur Daemon, which is running on your PC or Mac.
It gives Lemur direct access to all MIDI ports of your PC/Mac, accordingly
also to the USB MIDI ports of the MIOS32 core.

In Lemur, select the first MIDI IN/OUT port of MIDIbox CV V2 as MIDI0 target


2) OSC
------

This requires an Ethernet port on your core module (e.g. MBHP_CORE_LPC17) or the
MBHP_ETH interface (e.g. connected to MBHP_CORE_STM32F4)


a) Lemur has to know the IP address and communication ports of your MIDIbox CV.
In order to find out the current IP (which automatically has been assigned via
DHCP), enter the "Netw" page on the SCS and scroll this page to the right.

Alternatively, enter "network" in MIOS Terminal if you haven't built a SCS.
Let's assume, that the IP is 192.168.1.103
Now enter this IP into the Lemur configuration panel for OSC0

In addition, you've to configure the destination port.
Lemur listens to port 8000 by default, so specify 8001 (it must be different)


a) MIDIbox CV V2 has to know the IP address of your iPad, and the communication
ports.

Lemur will display it's IP and local port in the configuration panel.
Let's assume it's "Lemur IP - 192.168.1.123:8000"

Now go to the OSC page of the SCS, and enter:
Remote IP: 192.168.1.123 (the IP of your iPad)
RPort: 8000 (the port to which Lemur is sending)
LPort: 8001 (the port to which MIDIbox CV is sending)

In addition, change the OSC transfer mode to "Text Msg (Integer)"


If you haven't built a SCS, you can enter these details in the MIOS Terminal:
   set osc_remote 1 192.168.1.123
   set osc_remote_port 1 8000
   set osc_local_port 1 8001
   set osc_mode 1 1

Enter
   save default
to store the values on SD Card, so that they will be restored after power-on.


Testing
-------

After the bidirectional communication has been configured, you can test it
the following way:

Select CV1 on the Lemur panel, and move a knob (e.g. Portamento)
Now change to CV2, the Portamento value of the second channel should be displayed.
Change back to CV1: the previously changed value should be displayed.
