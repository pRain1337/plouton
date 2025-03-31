# Extending Targets

In order to extend targets, you should create a new directory for your new target. See this directory for examples.

Each target should have their own `TargetEntry` structure defined inside the [targets.h](targets.h) file.

The `TargetEntry` structure requires the target process's name (for example `target.exe`) and two function pointers, 
`InitCheat` (`typedef BOOLEAN EFIAPI (*InitCheat)(VOID);`) and `CheatLoop` (`typedef VOID EFIAPI (*CheatLoop)(VOID);`). 
Your cheat implementation must implement these two functions. The init function simply initializes your cheat (i.e., finds
the target process and required modules etc. from it, initializes local variables to your cheat). The loop function is
the function that is then run every SMI.

You also need to edit the [Plouton.inf](../Plouton.inf) and add your implementation's .c files to the list of to-be-compiled objects.

After you have done aforementioned steps, your cheat should compile and be part of the Plouton framework, and Plouton should
be searching and trying to init/execute your target code, if the target process is found.

In your implementation, you can call and use all the code available elsewhere in this project. Please see how the example CS2
cheat leverages xhci and memory functionality to parse game data and move mouse / play sound!

# Example implementations
To showcase the abilities of this project and how it works, we've prepared two example implementations.
## CS2
We have prepared a fully working implementation for Counter-Strike 2 with the following features:
 - Aimbot
 - Sound ESP

For further information, see [cs2](./cs2).

## Hermes
We have implemented our previous project, [Hermes](https://github.com/pRain1337/hermes) into this project.   
This allows you to use the framework to debug processes / access memory using SMM privileges, which can be especially useful to dump memory and analyze them later.   
For further information, see [hermes](Plouton-UEFI/Plouton/target/hermes).
