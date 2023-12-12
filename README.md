# FS20-SaiMFD

A Saitek DirectOutput plugin that connects the X52 Pro joystick with Flight Simulator 2020 via the SimConnect API to provide basic information on the joystick's MFD, as well as LED effects controlled by the simulated aircraft's state.

In the current state the MFD is output-only.

Work in progress.

## Setup

Soon...

## Known issues

I experience some serious bouncing with the MFD's scrollwheels, which is mostly mitigated by code for the custom (right) scrollwheel - which is currently used only to scroll within pages -, but not for the paging wheel (left), which has a handling more entangled with its API.

These ambiguous input events cause errors during paging, which currently triggers a simple restart in this plugin.
(This can be a problem in my hardware/driver, since even the Saitek example app behaves unstable.)

## Copyrights

This software is released under GPLv3. See [LICENSE.txt](LICENSE.txt).
Copyright 2023 Norbert Fekete.

Headers and provided utilities for DirectOutput API are located under [DirectOutputHelper/Saitek](DirectOutputHelper/Saitek/DirectOutput.h).
Copyright 2008 Saitek

Connection to Microsoft Flight Simulator 2020 is obtained via the SimConnect API, the
headers of which are provided by its installed SDK and do not form part of this source tree.