# ESP MODEM

This component is used to communicate with modems in command and data modes. 
It abstracts the UART I/O processing into a DTE (data terminal equipment) entity which is configured and setup separately.
On top of the DTE, the command and data processing is performed in a DCE (data communication equipment) unit.
```
 esp-modem
  
  - start-ppp              +-------+    +-------+
  - stop-ppp               | reset |    | retry |
  - events                 | helper|    | helper|
                           +-------+    +-------+     
   +-----+     +-----+         .            .
   | DTE |--+->| DCE |.......................
   +-----+  |  +-----+
            |
            |  +-------------+
+-----+     +->| modem-netif |
| PPP |------->|             |
+-----+        +-------------+
```

## Start-up sequence

To initialize the modem we typically:
* create DTE with desired UART parameters
* create DCE with desired command palette and attach it to the DTE
* create PPP netif and attach it to the DTE
* configure event handlers for the modem and the netif

Then we can start and stop PPP mode using esp-modem API, as well as receive events from the `ESP_MODEM` base
as well as from netif (IP events).

### DTE

Responsibilities of the DTE unit are
* sending/receiving commands to/from UART 
* sending/receiving data to/from UART 
* changing data/command mode on physical layer

### DCE

Responsibilities of the DCE unit are
* definition of available commands
* cooperate with DTE on changing data/command mode

### PPP-netif

The modem-netif attaches the network interface (which was created outside of esp-modem) to the DTE and
serves as a glue layer between esp-netif and esp-modem.

### Additional units
ESP-MODEM provides these two modules, that could be used in addition to with other units to make 
modem communication more robust:
* reset-helper is a very simple GPIO pulse generator, which could be typically used to reset or power up/down
the module
* retry-strategy is a command executor abstraction, that helps with retrying sending commands in case of a failure or a timeout.


## Modification of existing DCE

In order to support an arbitrary modem, device or introduce a new command we typically have to either modify the DCE,
adding a new or altering an existing command or creating a new "subclass" of the existing DCE variants.

## Internal design

