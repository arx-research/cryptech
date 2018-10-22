# coretest #
Test platform for the [Cryptech Open HSM project](https://cryptech.is/).

(_Note:The Cryptech certificate is by choice not from a CA and therefore
not in your brower trust store._)


## Description ##
This platform and hardware design is used to functionally verfiy cores
developed in the Cryptech Open HSM project. The test core itself
contains just enough functionality to be able to verify that the SW in
the host computer can talk to the core in the FPGA by reading and
writing 32 bit data words to given addresses.

This project includes cores in Verilog, a testbench as well as host SW
to talk to the core.


## Architecture ##
The coretest consists of three state machines:

### rx_engine ###
Handles receiving command messages from the host. Awaits SYN signals from
the host interface and reads bytes when SYN is asserted. For each byte
the rx_engine assetts and ACK and waits for the SYN to be asserted. When
a EOC byte has been detected the rx_engine signals the test_engine that
there is a new command available in the rx_buffer.


### tx_engine ###
Handles transmitting response messages to the host. When the test_engine
signals that there is a new response in the tx_buffer the tx_engine will
start transmitting all bytes up to and including the EOR byte it is
expecting in the tx_buffer. The transmission is done by asserting SYN
awaiting ACK, deasserting SYN and moving to the next byte before
asserting SYN again. This process is repeated until all bytes has been
transmitted.


### test_engine ###
Performs the parsing of commands from the host. Known read or write
commands are used to test the core to be tested. The response from the
core is collected and the appropriate response is stored in the
tx_buffer. The test_engine then signals the tx_engine that there is a
new response message to be transmitted.

## Interfaces ##
The host communication interface is a byte wide data interface with
SYN-ACK handshake for each byte.

The interface to the core to be tested is a memory like
interface with chip select and write enable. The data width is 32-bits
and the address is 16-bits.


## Core under test ##
The core under test is expected to have a simple memory like interface
with chip select (cs), write enable (we) signal ports, 16-bit address
port and separate 32-bit data ports for read and write data. The core is
also expected to have an error signal port that informs the master if
any read or write commands given cannot be performed by the core.

***Note:***
The core reset signal is expected to by active high. The
core reset signal should be connected to the coretest core_reset
port, not to system reset.


## Protocol ##
Coretest uses a simple command-response protocol to allow a host to
control the test functionality.

The command messages are sent as a sequence of bytes with a command byte
followed by zero or more arguments. The response consists of a response
code byte followed by zero or more data fields.

The start of a command is signalled by a Start of Command (SOC)
byte. The end of a command is signalled by a End of Command (EOC)
byte. These bytes are:
  - SOC: 0x55
  - EOC: 0xaa

The start of a response is signalled by a Start of Response (SOR)
byte. The end of a response is signalled by a End of Respons (EOC)
byte. These bytes are:
 - SOR: 0xaa
 - EOR: 0x55

***The commands accepted are:***
  - RESET_CMD. Reset the core being tested. Message length is 3 bytes
    including SOC and EOC.
    - SOC
    - 0x01 opcode
    - EOC


  - READ_CMD. Read a 32-bit data word from a given address. Message
    length is 5 bytes including SOC and EOC.
    - SOC
    - 0x10 opcode
    - 16-bit address in MSB format
    - EOC
    

  - WRITE_CMD. Write a given data word to a given address. Message
    length is 9 bytes including SOC and EOC.
    - SOC
    - 0x11 opcode
    - 16-bit address in MSB format
    - 32-bit data in MSB format
    - EOC
    

***The possible responses are:***
  - UNKNOWN. Unknown command received. Message length is 4 bytes
    including SOR and EOR. 
    - SOR
    - 0xfe response code
    - Received command
    - EOR
    

  - ERROR. Known but unsuccessful command as signalled by the
    core. Caused for example by a write command to read only
    register. Message length is 4 bytes including SOR and EOR.
    - SOR
    - 0xfd response code
    - command received
    - EOR


  - READ_OK. Sent after successful read operation. Message length is 9
    bytes including SOR and EOR .
    - SOR
    - 0x7f response code
    - 16-bit address in MSB format
    - 32-bit data in MSB format
    - EOR


  - WRITE_OK. Sent after successful write operation. Message length is 5
    bytes including SOR and EOR 
    - SOR
    - 0x7e response code
    - 16-bit address in MSB format
    - EOR


  - RESET_OK. Sent after successful reset operation. Message length is 3
    bytes including SOR and EOR.
    - SOR
    - 0x7d response code
    - EOR

## Status ##
***(2014-02-11):***

Added information about the architecture and protocols. Updated the
command and response with explicit read and write ok responses. Some
cleanup of the description.

Completed first draft of the RTL for coretest. The RTL is not debugged
and has not been synthesized. We need to add a testbench and a simple
test core.

Added a simple test core.
Adding initial version of UART core that will be used for the host
interface.


***(2014-02-10):***

Initial version of the project. Based on previous cttest project but
renamed and with new (ideas) about the test architecture. Specified
command and response protocol.

