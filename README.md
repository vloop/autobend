## Summary

1) autobend
Apply pitch bend depending on last note.

This program offers a small subset of the Scala microtonal program functionality.
It focuses on ease of use and light weight.
It allows microtuning on mono synths that do not provide it.
It works properly only in mono mode: because pitch bend is a channel message, if a chord is played the same pitch bend will be applied to all notes.
Scala attempts to work around this by rotating note channels, depending on your polysynth this may work or not.
see also Scala http://www.huygens-fokker.org/scala/

2) autohold
Implement pedals for poly synths that do not support it

Suitable for use with triple pedals (damper, sostenuto, unacorda)
The damper is handled as an on-off switch.

## Installation

No installer provided at the moment

The only prerequisites are alsa and fltk

The following commands were tested under Linux Mint 19.1

- Installing ALSA development files:
```
sudo apt install libasound2-dev
```
- Installing FLTK and its development files:
```
sudo apt install libfltk1.3-dev
```
- Compiling:
```
gcc autobend.cxx -o autobend -lfltk -lasound -lpthread -lstdc++
gcc autohold.cxx -o autohold -lasound -lpthread -lstdc++
```
- Installing:
```
sudo cp autobend /usr/local/bin/
sudo cp po/fr/autobend.mo /usr/share/locale/fr/LC_MESSAGES/
sudo cp autohold /usr/local/bin/
```
## Usage
```
autobend [file]
```
Using the sliders with the mouse gives only coarse control, accuracy requires using the keyboard shortcuts
up arrow or numeric keypad +, down arrow or numeric keypad -, page up and down or using the mouse scroll wheel.
Delete or ., home and end will set the value to 0, -8192 and +8191 respectively.

Autobend has no way of knowing the bender range on your synth,
the pitch bend midi message range is fixed from -8192 to 8191
but the corresponding musical interval may vary, therefore tuning has to be done by ear.

Files default to .conf file type. This is a plain text file, whith a very simple syntax:
each line is of the form "note space offset", for example E -2048.

```
autohold [option]...
```
This is a command-line only tool.
Options include -h which diplays usage specifics.
```
Connections
```
In the normal use case, one needs to connect the master keyboard output to the tool (autobend or autohold) input,
and the tool output to the synth input.
For example, you may use qjackctl or aconnect for that purpose.

### Thanks
Thanks to jmechmech for the original idea and testing of autobend
