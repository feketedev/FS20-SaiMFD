# FS20-SaiMFD

A Saitek DirectOutput plugin that connects the good old X52 Pro joystick with Flight Simulator 2020 via the SimConnect API to provide basic information on the joystick's MFD, as well as LED effects that reflect the state of the simulated aircraft.


## Features

### Pages

In their current state the **MFD pages** are output-only, and not tailored for every aircraft kind. (Piston, turboprop, and multi-engine jets are supported in the first place.)

The applicable pages appear in the following order:
* Generic info (G-force, airspeed, altitude etc.)
* Aerodynamic configuration (flaps, spoilers, trim)
* Engine instruments (1..4 engines) &emsp;&emsp;&emsp;&emsp;&emsp;&#129144; Default at startup
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

Once the joystick is plugged in, you should be presented with a little bar animation on the MFD waiting for connection to MSFS and then for a flight to load. (Idling gauges may show up a few times during the game's loading process.)

For Saitek's DirectOutput service to work properly on current Windows versions (on 11 certainly) the latest, signed version of the X52 Pro driver should be installed from Logitech's website.
(I experimented a bit with older versions for their resizeable profile editor... with no luck.)

Should you be in doubt whether DirectOutput is working, you can check its provided Test app located by default at "C:\Program Files\Logitech\DirectOutput\SDK\Examples\Test\Test.exe".


## Customization

Currently the only way of customization is by code - but it's aimed to be mostly readable and easily extensible with new simulation variables. 
The whole configuration of Page contents and LED effets is contained within the single file [FSMfd/Configurator.cpp](FSMfd/Configurator.cpp). 
I think it's manageable with very minimal experience in any kind of programming just by following existing patterns -  
albiet the text alignments on MFD are not fully straitforward; also the process requires recompiling this plugin.

To do so you'll need to install the MSFS SDK, accessible from the dev menu in the game.
After that, either re-import their build properties
"\<Your-SDK-Location\>\SimConnect SDK\VS\SimConnectClient.props"  
into the FSMfd project using Visual Studio, or just update its path in [FSMfd/FSMfd.vcxproj](FSMfd/FSMfd.vcxproj) manually.
(For details see their instructions at https://docs.flightsimulator.com)

DirectOutput headers are included in the source-tree, so the driver just needs to be installed on the computer.


## Known issues

1. I experienced serious bouncing with the MFD's scroll wheels, even leading to inconsistent DirectOutput events about page changes - however:
	* The cause seem to be some hardware or driver issue related to my current PC.  
	  Lately I've tested DirectOutput on the previous one (using the same updated Logitech driver) and that isn't affected.
	* Since v0.86 these potential problems are largely mitigated by code too.  
	  In the worst case (a page activation event is omitted) the MFD can pause blank, but any further scroll/press should recover it.

2. Some LEDs tend to flash (reveal their default colors for a moment) while turning Pages.
	* As a partial workaround it is suggested to addjust their default colors in USB Game Controller settings to resemble the usual  
	   (i.e. warning-less) in-game colors or simply to Off (which is less annoying for the eyes as "flash" color).

3. If started while FS is in the main menu, the plugin shows idling gauges instead of the wait animation.
	* This is a limitation of available events from the game.

#### About the code
It was kind of a back-to-programming project - so yes, it does contain some unnecessary extra and exploratory parts, but I think it tends to be structured. :)


## Copyrights

This software is released under GPLv3. See [LICENSE.txt](LICENSE.txt).  
Copyright 2023 Norbert Fekete.

Headers and provided utilities for DirectOutput API are located under [DirectOutputHelper/Saitek](DirectOutputHelper/Saitek/DirectOutput.h).  
Copyright 2008 Saitek

Connection to Microsoft Flight Simulator 2020 is obtained via the SimConnect API, the  
headers of which are provided by its installed SDK and do not form part of this source tree.
