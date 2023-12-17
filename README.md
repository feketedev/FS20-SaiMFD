# FS20-SaiMFD

A Saitek DirectOutput plugin that connects the good old X52 Pro joystick with Flight Simulator 2020 via the SimConnect API to provide basic information on the joystick's MFD, as well as LED effects that reflect the state of the simulated aircraft.


## Features

### Pages

In their current state the **MFD pages** are output-only, and not tailored for every aircraft kind. (Piston, turboprop, and multi-engine jets are supported in the first place.)

The applicable pages appear in the following order:
* Generic info (G-force, airspeed, altitude etc.)
* Aerodynamic configuration (flaps, spoilers, trim)
* Engine instruments (1..4 engines)
* Autopilot parameters
* Radio frequencies
* Radionavigation (VOR/ADF output)

Within each page the right scroll wheel simply scrolls the page contents. Some gauges jump 2 lines per scoll to help (it's yet to be decided if this is helpful or annoying).


### LEDs
They provide various kinds of feedback about the most important factors during flight. The intention is to follow the common logic that Amber items require some action or interim attention, while Red items call for more acute warnings.

**Toggle switches**  		
* Off-Red-Green: *Landing gear status (quite naturally :))*
* Middle Amber: *Steering is unlocked (taildragger only)*
* Left Amber: *Parking brake set*

**Button E**
* Steady Amber: *Gears extended, Spoilers Unarmed*
* Slow Amber Blink: *Spoilers extended*
* Slow Red Blink: *Overspeed*
* Fast Red Blink: *Stall*

**Fire button**
* Steady by default (read *"SAFE"* in green, provided the cover is closed :))
* Replicates the above Blinks on *Stall* or *Overspeed*

**Buttons A, B**
Flaps status:
* Off: *Retracted*
* 1..2 Green: *Extended >= 1%, >= 25%*
* 1..2 Amber: *Extended >= 50%, >= 75%*
* Lower Amber only: *Fully extended*

**Button I**
Thrust status
* Steady Green: *default - forward thrust*
* Steady Red: *Reverse thrust selected*
* Blinking Red: *Thrust reverser nozzles open (jet only)*
* Blinking Green: *Autopilot TOGA active*

**Button D**
Autothrust state
* Off: *Disengaged*
* Amber: *A/T Armed*
* Green: *A/T Engaged*
* Red: *Autopilot Disengage warning*

**POV-hat 2**  --  Autopilot state
* Off: *Disengaged*
* Green: *A/P Engaged*
* Amber: *Approach, Glideslope Captured*
* Red: *Autopilot Disengage warning*


## Setup

Simply download the latest executable to your preferred location and run it. 
(Visual C++ redistributable is assumed to be already present, given that MSFS 2020 is installed.)

Once the joystick is plugged in, you should be presented with a little bar animation on the MFD waiting for connection to MSFS and then for a flight to load. (Empty gauges on the MFD may flicker a few times due to some false notifications during the game's loading process.)

For Saitek's DirectOutput service to work properly on current Windows versions (on 11 certainly) the latest, signed version of the X52 Pro driver should be installed from Logitech's website.
(I experimented a bit with older versions for their resizeable profile editor... with no luck.)

Should you be in doubt whether DirectOutput is working, check out its provided Test app located by default at "C:\Program Files\Logitech\DirectOutput\SDK\Examples\Test\Test.exe".


## Customization

Currently the only way of customization is by code - but it's aimed to be mostly readable and easily extensible with new simulation variables. 
The whole configuration of Page contents and LED effets is contained within the single file [FSMfd/Configurator.cpp](FSMfd/Configurator.cpp). 
I think it's manageable with very minimal experience in any kind of programming just by following existing patterns -  
albiet the text alignments on MFD are not fully straitforward; and the process requires recompiling this plugin.

To do so you'll need to install the MSFS SDK, accessible from the dev menu in the game.
After that, either re-import their build properties
"\<Your-SDK-Location\>\SimConnect SDK\VS\SimConnectClient.props"  
into the FSMfd project using Visual Studio, or just update its path in [FSMfd/FSMfd.vcxproj](FSMfd/FSMfd.vcxproj) manually.
(For details see their instructions at https://docs.flightsimulator.com)

DirectOutput headers are included in the source-tree, so the driver just needs to be installed on the computer.


## Known issues

I experience some serious bouncing with the MFD's scroll wheels, which is mostly mitigated by code for the custom (right) scroll wheel - currently used only to scroll within page -, but not for the paging wheel (left), which has a more entangled handling in the driver (behind the API).

These ambiguous input events cause errors during paging, which currently trigger a simple restart mechanism in this plugin.  
(This can be a problem in my hardware/driver since even the Saitek example app behaves unstable.)

#### About the code
It was kind of a back-to-programming project - so yes, it does contain some unnecessary extra and exploratory parts, but I think it tends to be structured. :)


## Copyrights

This software is released under GPLv3. See [LICENSE.txt](LICENSE.txt).  
Copyright 2023 Norbert Fekete.

Headers and provided utilities for DirectOutput API are located under [DirectOutputHelper/Saitek](DirectOutputHelper/Saitek/DirectOutput.h).  
Copyright 2008 Saitek

Connection to Microsoft Flight Simulator 2020 is obtained via the SimConnect API, the  
headers of which are provided by its installed SDK and do not form part of this source tree.
