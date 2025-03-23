# Hermes implementation
This is a example implementation of our previous Hermes project.   
Hermes allows us to communicate with SMM over a shared memory buffer. This way we can send commands to SMM that should be executed with those privileges.   
For the implementation, see [hermes.c](hermes.c).

## Features
We have added all commands of the Hermes project, see the following list:
| Command | Description | Input | Output |
|---|---|---|---|
| gd | Returns the directory base of the requested process  | Process name | Directory Base |
| gmd | Returns essential information of the requested module in a process | Process name & Module name | Module name & Size |
| gm | Returns all module names of the requested process  | Process name | Name of every module in a process |
| vr | Reads the memory at the requested virtual memory address | Source Virtual address, Directory Base & Size  | Memory read at the address |
| vw | Writes the supplied integer to the requested virtual memory address | Destination Virtual address, Directory Base, Size & Value | - |
| pr | Reads the memory at the requested physical memory address | Source Physical address, Directory Base & Size  | Memory read at the address |
| pw | Writes the supplied integer to the requested physical memory address | Destination Physical address, Directory Base, Size & Value | -  |
| vto | Converts a virtual memory address to physical | Source Virtual address & Directory Base | Converted Physical address  |
| dm | Dumps the requested memory area | Source Virtual address, Directory Base, Size & File name | Memory read is written into the file |
| exit | Exits the client process | - | - |
| help | Displays the help about the commands | - | - |

### Client module
If interested in using Hermes, please see the [Hermes project repository](https://github.com/pRain1337/hermes) for how to compile the client binary.

## How to use
Install the Plouton SMM module on your motherboard and boot the system with it.   
Check the serial output to verify if it initialized as expected.    
Start the Hermes client binary with the name "hermes.exe" and verify if communication works by getting the dirbase of Hermes itself.   
For this, enter `gd` and then the process name `hermes.exe`, this should return the directory base of the hermes process itself.   
Afterwards you can use all other commands mentioned in the above chapter.   
