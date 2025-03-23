# Plouton
<p align="center">
  <img src="/images/logo_plouton.jpg" alt="Picture of Plouton" width="600">
</p>

*Plouton was the master of the underworld, and thus he was able to command the riches of the earth and the souls of the dead.*

Plouton is a System Management Mode (SMM) (ring-2, *"underworld"*) PC game cheat framework.    
This repository and code were created as proof-of-concept and released as open source, we do not take any responsibility for the further usage of this project.   

# Supported platforms
Plouton supports only Intel-based systems, for AMD-based systems some importants features (e.g. XHCI generating SMIs on USB Events) are not available). The core functionality and memory code would still be applicable, and could be reused.   
The core has been tested on Intel generations from Series 200 (Skylake, Kaby Lake) up to Series 700 (Alder Lake, Raptor Lake).   
According to the offsets in the Intel Chipset datasheet for the Series 800, it should also be supported but has not been tested.   

# Building

See [Plouton-UEFI](Plouton-UEFI)

# Extending

To extend Plouton to support your game of choice, see [targets](Plouton-UEFI/Plouton/target/)

To extend Plouton to support your hardware (mouse, audio devices), see [hardware](Plouton-UEFI/Plouton/hardware/)

To extend for other OS than Windows, sorry, this is not currently possible :-)

Contributions are welcome!
