# CRSF Module

CRSF (TBS Crossfire protocol) middleware written in C++ (C++17, no dynamic
memory, no exceptions, no RTTI) for embedded targets. Implemented according to
the TBS specification `crsf.pdf` (2024-11-19) in this folder.

The module covers the flying platform side (flight controller, receiver
peripheral) as well as the RC / TX module side: frames are received and sent in
both directions, the role is given only by the handlers the application
registers.

## Features

| Area                    | Frames                                   | Class / API                       |
|-------------------------|------------------------------------------|-----------------------------------|
| RC channels + failsafe  | 0x16                                     | `AppLayer::RcChannels`            |
| Telemetry reception     | 0x02 0x03 0x06 0x07 0x08 0x09 0x0B 0x10 0x14 0x1C 0x1D 0x1E 0x1F 0x21 0x22 0x3A.0x10 0xAC | `AppLayer::TelemetryRx` (callbacks) |
| Telemetry transmission  | same as above + 0x16, 0x28, 0x32         | `AppLayer::TelemetryTx`           |
| Device parameters       | 0x28 ping, 0x29 info, 0x2B entry (chunks), 0x2C read, 0x2D write | `AppLayer::Params` |
| Direct commands         | 0x32 (command CRC 0xBA, Command ACK)     | `AppLayer::Commands`              |
| Payload encode / decode | all broadcast frames, baro altitude / vario packing, channel us conversion | `Codec` |
| Bridge / routing        | forwarding between two CRSF ports         | `AppLayer::Bridge`                |

## Architecture

```text
 Application (callbacks, parameter table, periodic telemetry)
          |
 +--------v-------------------------------------------------------+
 | AppLayer  AppCore -> FrameHandler (RcChannels, TelemetryRx,     |  Crsf_App*.hpp
 |                                    Params, Commands, custom)    |
 |           TelemetryTx, Codec (PayloadWriter / PayloadReader)    |
 +--------^-------------------------------------------------------+
          | crsf_Frame_t (type, dest, orig, payload)
 +--------v-------------------------------------------------------+
 | TransportLayer  frame search / check (sync, length, CRC 0xD5),  |  Crsf_TL.hpp
 |                 transmit queue, statistics; Crc8                |  Crsf_Crc.hpp
 +--------^-------------------------------------------------------+
          | byte stream
 +--------v-------------------------------------------------------+
 | PhysicalLayer   interface implemented by application            |  Crsf_Phy.hpp
 +----------------------------------------------------------------+
```

* **Physical layer** (`Crsf::PhysicalLayer`) - continuous reception (eg.
  USART + circular DMA), non-blocking `Get_RxData`, transmission of one frame,
  baudrate. Default 416666 Bd full duplex, 400000 Bd single wire half duplex.
  CRSF is a byte stream, the physical layer does not detect frame boundaries.
* **Transport layer** - accepts sync byte `0xC8`, broadcast `0x00` and own
  device address (or any byte in `CRSF_SYNC_ANY` mode for transparent
  bridge). Length field out of 2 - 62 or invalid CRC drops one byte and the
  search continues, so a valid frame following a damaged or truncated one is
  not lost. Incomplete data without a new byte for 5 ms (stall timeout) are
  resynchronized the same way. Frames for transmission are queued
  (`txQueueLen` = 4) and sent one by one; the first frame starts immediately.
* **Application core** (`Crsf::AppLayer::AppCore`) - reads up to 8 frames per
  task, calls the optional frame callback / observer, passes the frame to all handlers
  supporting its type (extended frames only if addressed to own device or
  broadcast) and runs handler tasks. Implements `Crsf::FrameSender` - origin of
  extended frames is own device address.

## Integration (flight controller)

The complete working integration is in [Example](Example) - see
`Example/Crsf_Example.cpp`.

1. **Implement the physical layer** - derive from `Crsf::PhysicalLayer`
   (example: `Example/Crsf_Phy_Usart.cpp` over the C interface
   `Example/Crsf_Hal.h`).
2. **Describe application data** - telemetry callbacks, parameters, commands:

   ```cpp
   static const crsf_DeviceInfo_t deviceInfo = { "EmBi FC", serial, hwId, fwId, 1u };

   static const crsf_ParamConfig_t params[] =      /* item 0 = parameter 1 */
   {
       CRSF_PARAM_FOLDER_ITEM        ( 0u, "Tuning" ),
       CRSF_PARAM_FLOAT_ITEM         ( 1u, "Rate gain", Get_Gain, Set_Gain, 50, 300, 100, 2u, 5, "x" ),
       CRSF_PARAM_TEXT_SELECTION_ITEM( 1u, "Mode", "ACRO;ANGLE;HORIZON", Get_Mode, Set_Mode, 0, "" ),
       CRSF_PARAM_COMMAND_ITEM       ( 0u, "Calibrate", Get_Calib, Set_Calib, 50 ),
   };

   static const crsf_CommandConfig_t commands[] =
   {
       { CRSF_CMDID_FC, CRSF_SUBCMD_FC_FORCE_DISARM, ForceDisarm },
   };
   ```

3. **Create the stack** (static objects):

   ```cpp
   static Crsf::UsartPhysicalLayer       phy;
   static Crsf::TransportLayer           tl( phy );
   static Crsf::AppLayer::RcChannels     rc;                       /* failsafe 1000 ms */
   static Crsf::AppLayer::TelemetryRx    telemetryRx( telemetryCallbacks );
   static Crsf::AppLayer::Params         paramsHandler( deviceInfo, params, 4u );
   static Crsf::AppLayer::Commands       commandsHandler( commands, 1u );

   static Crsf::FrameHandler * const handlers[] = { &rc, &telemetryRx, &paramsHandler, &commandsHandler };

   static Crsf::AppLayer::AppCore        core( tl, handlers, 4u );
   static Crsf::AppLayer::TelemetryTx    telemetryTx( core );
   ```

4. **Set own address** (`core.Set_DeviceAddr( CRSF_ADDR_FLIGHT_CONTROLLER )`,
   default) and baudrate before the first task.
5. **Call the task cyclically** - `core.Task( taskTimeMs )` (1 ms recommended,
   RC frames come every 1 - 20 ms). Send telemetry by `telemetryTx.Send_xxx()`.

### RC channels and failsafe

`RcChannels` stores the last channels (raw 11 bit, center 992 = 1500 us,
`Get_ChannelUs` converts by `(x - 992) * 5 / 8 + 1500`). The RC channels frame
is not sent in failsafe, so the link is reported as `CRSF_LINK_LOST` when no
frame is received within the failsafe timeout (constructor parameter, 1 s by
specification recommendation). Channels keep the last received values; the
application decides the failsafe action. Callbacks: `Set_ChannelsCallback`,
`Set_LinkStateCallback`.

### Device parameters

| Situation                                  | Response                                       |
|--------------------------------------------|------------------------------------------------|
| Ping to own address or broadcast           | 0x29 device information to origin              |
| Read parameter 0                           | Root folder named by device name, children = parameters with parent 0 |
| Read parameter, entry > 56 bytes           | Split into chunks; entry is built for chunk 0 and following chunks are sent from the same (cached) entry |
| Read / write not existing parameter        | Entry of type `OUT_OF_RANGE` (0x7F)            |
| Read not existing chunk                    | No response                                    |
| Write FLOAT out of min/max, TEXT_SELECTION index out of options | Setter not called, actual entry answered |
| Write STRING longer than maximal length    | Truncated to maximal length                    |
| Write COMMAND START / CONFIRM / CANCEL     | Setter called with action, actual entry answered (setter shall update status, eg. PROGRESS) |
| Write COMMAND POLL                         | Actual entry answered                          |
| Write folder, info, parameter without setter | Actual entry answered, nothing changed       |

Getter failure reports the configured default value. Parameter visibility
(hidden bit 7 of data type) can be controlled by `VisibilityGetter` of the
configuration item. Asynchronous status change (eg. COMMAND PROGRESS -> READY)
is pushed by `Params::Request_EntryUpdate( paramNumber, hostAddr )`, sent in
the next task.

Supported data types: FLOAT, TEXT_SELECTION, STRING, FOLDER, INFO, COMMAND.
Deprecated integer types (UINT8 ... INT32) are reported as `OUT_OF_RANGE` - use
FLOAT with decimal point 0 (as recommended by specification).

### Direct commands

Command frame payload: command ID, sub-command ID, data, command CRC
(polynomial 0xBA over type, destination, origin, IDs and data). Frame with
invalid command CRC is ignored (`Get_CrcErrorCnt`). Command addressed directly
to own device is acknowledged by Command ACK `0x32.0xFF` with the action
returned by callback (`CRSF_CMD_ACTION_NONE` for not configured command);
broadcast commands and received ACKs are not acknowledged. ACK can be disabled
by constructor parameter. Commands are sent by `TelemetryTx::Send_Command`.

### Bridge mode (two CRSF ports)

Specification requires a device with more CRSF ports to forward all received
frames to the other ports. `Crsf::AppLayer::Bridge` connects two transport
layers; it is not part of the physical layer, because only valid frames
(sync, length, CRC checked) shall be forwarded and frames for the local device
can be filtered by header. Each port has its own physical layer, baudrate,
duplex mode and transmit queue. Frames are forwarded unchanged (incl. sync byte
and CRC).

**Standalone bridge** (no local device, eg. UART repeater / converter):

```cpp
static Crsf::UsartPhysicalLayer phyRx;          /* eg. receiver side, 416666 Bd */
static Crsf::UsartPhysicalLayer phyTx;          /* eg. other port               */
static Crsf::TransportLayer     tlA( phyRx );
static Crsf::TransportLayer     tlB( phyTx );
static Crsf::AppLayer::Bridge   bridge( tlA, tlB );

tlA.Set_SyncMode( CRSF_SYNC_ANY );              /* transparent - any sync byte */
tlB.Set_SyncMode( CRSF_SYNC_ANY );

bridge.Task( 1u );                              /* cyclically, 1 ms */
```

**Device with two ports** (eg. flight controller forwarding between receiver
and VTX / OSD port): each port has its own `AppCore` with handlers, the bridge
observes both cores, frames are processed locally and forwarded:

```cpp
static Crsf::AppLayer::AppCore coreA( tlA, handlersA, countA );
static Crsf::AppLayer::AppCore coreB( tlB, handlersB, countB );
static Crsf::AppLayer::Bridge  bridge( tlA, tlB );

coreA.Set_FrameObserver( &bridge.Get_PortObserver( CRSF_BRIDGE_PORT_A ) );
coreB.Set_FrameObserver( &bridge.Get_PortObserver( CRSF_BRIDGE_PORT_B ) );
bridge.Set_SkipOwnFrames( true );               /* requests for own device stay local */

coreA.Task( 1u );                               /* bridge.Task() is NOT called */
coreB.Task( 1u );
```

* Frames sent by the local device (responses, telemetry) go out only on the
  port of its application core.
* `Set_Filter( callback )` - application decision per frame and source port
  (eg. do not forward RC channels to an OSD port).
* `Get_Stats( port )` - forwarded, filtered and overflowed frames (transmit
  queue of the other port full, eg. slower baudrate).
* Loops in topology are forbidden by specification (endless forwarding).

### Writing a custom handler

Derive from `Crsf::FrameHandler`, implement `Get_FrameTypeStatus()` and
`Process_Frame()` (optionally `Task()`). `frame.Payload` points behind the
(extended) header, responses are sent by `sender.Send_ExtFrame( type,
frame.OrigAddr, ... )`. Payload helpers: `Crsf::PayloadReader` /
`Crsf::PayloadWriter` (big endian, bounds checked).

## Notes to the specification

* Battery sensor units: specification states LSB 10 uV / 10 uA, common
  implementations (Betaflight, EdgeTX) use 0.1 V / 0.1 A. The module transports
  raw values.
* Vertical speed unpacking formula in the specification has a misplaced
  parenthesis; `Codec::Get_VerticalSpeedCmS` implements the exact inverse
  `(e^(|p| * 0.026) - 1) * 100`.
* Frames 0x34 (Logging), 0xAA (MAVLink envelope) and 0xAC (MAVLink system
  status) have short header although their type is above 0x27
  (`Crsf_Get_HeaderType`).
* Payload longer than expected is accepted (newer protocol version), shorter
  payload is rejected by decoders.

## Build

The component is located in `Middlewares/Core/CRSF` of the project:

```text
Middlewares/
├── CRSF/                    Project handler module - created from Example/,
│                            owned by the project, never overwritten
└── Core/CRSF/               This repository
    ├── CMakeLists.txt       Creates the handler module, adds the stack
    ├── Example/             Handler module template (library Crsf_Lib)
    └── CRSF/                CRSF stack library (CrsfCore_Lib)
        ├── CMakeLists.txt   Library, documentation and unit tests
        ├── AppLayer/
        ├── TransportLayer/
        ├── PhysicalLayer/
        └── Tests/UnitTests/ Host unit tests
```

The stack is built by `CRSF/CMakeLists.txt` as library `CrsfCore_Lib`
(public include directories `AppLayer`, `TransportLayer`, `PhysicalLayer`).

During project configuration the root `CMakeLists.txt` copies `Example/` into
the handler module folder `Middlewares/CRSF` if the folder does not exist, it
is empty or it contains only the unmodified module scaffold of EmBi platform.
Any other content is the project's own handler module and it is left
untouched. The handler module requires implementation of `Crsf_Hal.h`
(USART / DMA driver) by the application. When the component is not located in
`Middlewares/Core` (e.g. standalone unit testing), no handler module is
created.

Footprint (arm-none-eabi-g++ 14.3, Cortex-M33, `-Os`): approx. 9.7 kB flash for
the module, no static RAM (objects are instantiated by the application; the
transport layer object has approx. 500 B, parameters handler approx. 280 B).

## Unit tests

Host unit tests (Unity) are in [CRSF/Tests/UnitTests](CRSF/Tests/UnitTests) and they are
registered by `UnitTesting_AddPath()` in the "UnitTest" build of EmBi platform:

| Test executable     | Scope                                                         |
|---------------------|---------------------------------------------------------------|
| `UT_Crsf_Crc`       | CRC tables compared with tables of specification, check values |
| `UT_Crsf_TL`        | Frame search, resynchronization, stall timeout, sync modes, transmit queue |
| `UT_Crsf_Codec`     | Encoding / decoding of all frames, packing helpers           |
| `UT_Crsf_AppCore`   | Dispatching, address filter, observer, RC channels, telemetry |
| `UT_Crsf_Params`    | Ping, device info, read (all types, chunks), write, updates   |
| `UT_Crsf_Commands`  | Command CRC, dispatching, Command ACK                         |
| `UT_Crsf_Bridge`    | Standalone and device bridge, filters, sync any mode, overflow |

Physical layer is replaced by `UtCrsf::FakePhy`; frames injected by tests use
CRC computed by the lookup tables copied from the specification (independent
of the implementation).

```sh
cmake --preset nucleo-h503rb-unittest
cmake --build --preset nucleo-h503rb-unittest
ctest --preset nucleo-h503rb-unittest -L Crsf
```

## Limitations

* Not implemented (can be added as custom handlers): 0x17 subset RC channels
  (discouraged by specification), MSP over CRSF (0x7A / 0x7B), MAVLink envelope
  (0xAA) chunk reassembly, 0x34 logging, 0x3C game, parameter host side
  (reading parameters of other devices), protocol speed negotiation logic
  (0x32.0x0A.0x70 is available as a command callback, baudrate change via
  `PhysicalLayer::Set_Baudrate` + `PhyInit` is the application's job).
* Half duplex timing (RC sends one frame per period and waits for telemetry)
  is the responsibility of the application / physical layer.
* BST (I2C) transport is not supported (EOL by specification).

## License

This project is licensed under the MIT License.
