# Pico-LoRa
## Features
Set-up and configuration of nodes

Fixed transmission between two nodes

Broadcast transmission to nodes on same channel

Message display on OLED


## Wiring / Pins
| Pin No. | Pin item | Pin direction | Pin application                                          |
| ------- | -------- | ------------- | -------------------------------------------------------- |
| 1       | M0       | Input         | Work with M1 to set the four operating modes             |
| 2       | M1       | Input         | Work with M0 to set the four operating modes             |
| 3       | RXD      | Input         | TTL UART inputs. Connect to external TXD output pin      |
| 4       | TXD      | Output        | TTL UART outputs, Connect to external RXD input pin      |
| 5       | AUX      | Output        | To indicate module’s working status & wakes up the external MCU. When buffer is empty, outputs high. When buffer not empty, outputs low |
| 6       | VCC      | Power supply  | 2.3V~5.5V DC                                             |
| 7       | GND      | Ground        | Connect to ground                                        |


## Operating Modes
| No  | Mode         | M1 | M0 | Description                                                                                                     |
| --- | ------------ | -- | -- | --------------------------------------------------------------------------------------------------------------- |
| 0   | Normal       | 0  | 0  | Can transmit and receive.                                                                                       |
| 1   | Wake Up      | 0  | 1  | Same as normal but preamble code is added to transmitted data to wake up the receiver.                          |
| 2   | Power Save   | 1  | 0  | UART is off, LoRa radio is on WOR(wake on radio) mode which means the device will turn on when there is data to be received. Transmission is not allowed. |
| 3   | Sleep        | 1  | 1  | Can configure LoRa. Is used to get/set module parameters or to reset the module.                                 |


## Configuration
| No | Command        | Description                                                |
| -- | -------------- | ---------------------------------------------------------- |
| 1  | C0 + config*  | Set the configuration persistently, config saved on power-down. Send C0 + 5 configuration bytes in hex format. |
| 2  | C1+C1+C1*     | Get the module configuration. Send 3x C1 in hex format.    |
| 3  | C2 + config*  | Config not saved on power-down. Send C2 + 5 configuration bytes in hex format. |
| 4  | C3+C3+C3**    | Get the module version information. Send 3x C3 in hex format. |
| 5  | C4+C4+C4      | Reset the module                                            |


## Transmission, Addresses & Channels
**Message / Address Format:**

Address High | Address Low | Channel | Message Content

00 01 04 Hello world!

**Transparent Transmission:**

All nodes have save address and channel. All nodes can send/receive to/from each other. Message do not need to be prefixed with address and channel bytes.

**Fixed Transmission:**

Ensure that module is configured to Fixed Transmission Mode in Options. E.g. 0xC4 (11000100)
Nodes can have different addresses and channels. Messages sent must be prefixed with address and channel of the destination node. 
E.g. 

_NODE A -> NODE B_

NODE A ADDRESS: 00 01 06 {Message Content}

NODE A's MESSAGE: 00 03 04

NODE B ADDRESS: 00 03 04

**Broadcast Transmission:**

A singular node can broadcast message to all nodes on the same channel. Broadcast message must be prefixed with address FFFF and a channel. Additionally, nodes with FFFF address can receive messages from any nodes of any address and the same channel.
E.g.

_NODE A -> NODE B + NODE C_

NODE A ADDRESS: 00 01 04

NODE A's MESSAGE: FF FF 04

NODE B ADDRESS: 00 02 04

NODE C ADDRESS: 00 03 04
--------------------------

_NODE B -> NODE C + NODE A_

NODE B ADDRESS: 00 02 04

NODE B's MESSAGE: 00 03 04 {Message Content}

NODE C ADDRESS: 00 03 04

NODE A ADDRESS: FF FF 04



## Block Diagram
![Block Diagram](docs/Pico-LoRA%20Block%20Diagram.png)

## Flowchart
![Flowchart](docs/Pico-LoRa%20Flowchart.png)
