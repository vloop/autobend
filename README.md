## Summary

Apply pitch bend depending on last note.

This allows microtuning on mono synths that do not provide it.
This works properly only in mono mode: because pitch bend is a channel message, if a chord is played the same pitch bend will be applied to all notes.

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
gcc autobend.c -o autobend -lfltk -lasound -lpthread -lstdc++
```
- Installing:
```
sudo cp autobend /usr/local/bin/
```
## Usage
```
autobend [file]
```
File save and load not implemented yet.
