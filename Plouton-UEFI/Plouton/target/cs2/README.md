# CS2 implementation
This is a example implementation of a game cheat fully working in SMM.   
There is no requirement for any interaction with the normal system, thus it is more complicated for an anti-cheat to detect such a cheat.

For the implementation, see [cheatCS2.c](cheatCS2.c).

## Features
Based on the ability to intercept and manipulate all incoming & outgoing USB packets before they can be interpreted by windows, we have implemented the following features:

### Aimbot
A simple configurable aimbot, which allows setting your own aim key, on what weapon types it should be active and what Field of View (FOV) and Smooth value should be used.   
If those factors are fullfilled, it will automatically lock on the nearest player in the configured FOV and change the USB packets to move to the nearest bone of the player.   
For changing settings, see the top of the [cheatCS2.c](cheatCS2.c) .c file.   

### Sound ESP
A simple configurable Sound ESP, which allows configuring your own FOV.   
If an enemy would be within the configured FOV, the Sound ESP would malform the packets of the headset/speaker and thus trigger a sound which indicates there is an enemy in the FOV.    
For changing settings, see the top of the [cheatCS2.c](cheatCS2.c) .c file.   

## How to update
All offsets / signatures used have been moved into the [offsetsCS2.h](offsetsCS2.h) header file. 
The offsets & signatures can both be found online in various forums or in GitHub repositories.

## Recommendations when adding features
It is important to keep in mind that the longer you stay in SMM, the more performance you loose.
Our recommendation is, splitting up operations and using caching to prevent unnecessary repetitive operations.
