# EtherRail DCC Base Station for PackTrack
The DCC basestation recieves speed permits from a PackTrack director and sends them out via DCC.
Our optimized DCC implementation allows for up to 9999 active trains on a layout.

## Hardware
We picked the ESP32-P4 with Ethernet and PoE as the main controller.
This device has quite some power and multiple cores, which allows us to keep the networking tasks and DCC generation separate.
Using Ethernet allows for a stable connection, while PoE eliminates the need for an extra power adapter.

A L298N H-Bridge powers the layout directly, with our special version providing up to 12A.
Multiple can be used, by sending the IN pins to other L298Ns of the same type.

INA226 current and voltage sensors constantly update the controller on the current power state of the layout.
The alarm function triggers an interrupt on the ESP, instantly shutting of the layout power.
This is on top of the L298Ns protections features.

A couple MAX7219 controlled 7-Segment displays in combination with some illuminated EAO 18/26 buttons allow for easy control.
The whole device is mounted in a 19" rack.

## Controls
The panel is split up into multiple sections.
A 3 digit 7 segment display is paired with two buttons below.

### Group 1: Voltage & current
Power for the tracks must be stable and in the correct range.
The controls allow the user to see and manage power.
Some times, the current limit can be an issue, the user should be able to manually overwrite this safety feature.

```
4 digits: voltage, constantly updated from INA226 sensor.
3 digits: current, updated constantly too.
```

```
Button 1: TBD
Button 2: TBD
Button 3: TBD
Button 4:
	Light: Current Limit enabled
	Press: Disable Current Limit (INA226 limit, L298N will still protect itself)
```

### Group 2: Connectivity
The director should constantly update our speed permits.
If this does not work, the controls allow the user to reconnect.

```
3 digits: age of oldest active speed permit - in 10milliseconds
```

```
Button 1:
	Light: Connected
	Press: Force Reconnect
Button 2: TBD
```

### Group 3: DCC Signal
The third control group shows the current load on the DCC signal.
Smart Packing enables running a lot more locomotives than normal DCC allows.
Some features are not nescessary for operations, such as lights (FX) - they can be disabled here manually, when the system is under load.

```
3 digits: queue/active items length
```

```
Button 1:
	Light: Smart Packing enabled
	Press: Toggle Smart Packing
Button 2:
	Light: FX enabled
	Press: Toggle FX sending
```

## Smart Packing
DCC is a old protocol.
The signals are not very fast and thus not too efficient.

Sending a single bit takes between 0.116ms and 0.2ms.
A speed update contains 34 bits, so around 5ms.
Setting a function takes another 5ms.
And every turnout takes 5ms too.

So 60 trains (600ms) and 70 turnouts (350ms) and you're at almost a second.
That is a second delay betwen cranking the throttle and the train moving.
Let's not even get started on extended functions, signals, ...

Our smart packing algorithm makes sure that important updates are dispatched quickly,
running trains get fed new updates regularly and functions get updated.

We bundle the most important stuff into a frame which can contain up to 40 messages, about 200ms.

At full load, the packing algorithm assigns:
- 5 Changing train updates (TC)
- 10 Running train updates (TR)
- 5 Sleeping train updates (TS)
- 3 Updated function updates (FC)
- 2 Function updates (FA)
- 10 Changing accessory updates (AC)
- 5 Sleeping accessory updates (AS)

Let's assume that just one train got a speed update (within the last 200ms), while there are 12 trains running.
The packer would automatically send 2 train updates, then send all 12 running train updates.
