# MIDI Stream Parser

A lightweight, event-driven **MIDI 1.0 stream parser** designed for embedded systems.

The parser processes incoming MIDI bytes incrementally, supporting **Channel Messages**, **System Common Messages**, **System Real-Time Messages**, **Running Status**, and **System Exclusive (SysEx)** messages in compliance with the MIDI 1.0 specification.

The library performs **no dynamic memory allocation**, making it suitable for resource-constrained microcontrollers and real-time applications.

## Features

- MIDI 1.0 compliant stream parser
- Incremental byte-by-byte parsing
- Full Running Status support
- Correct handling of System Real-Time messages
- System Common message support
- Event-driven callback interface
- No dynamic memory allocation
- Suitable for bare-metal and RTOS-based embedded systems

## SysEx Handling

The parser provides **two different SysEx processing modes**, allowing applications to choose the most appropriate approach.

### Streaming Mode

If a **SysEx callback** is supplied, SysEx messages are processed as a stream.

The application receives the following events:

- **START** – beginning of the SysEx message (`0xF0`)
- **DATA** – each received data byte
- **END** – End Of Exclusive (`0xF7`)
- **ABORT** – SysEx terminated by another status byte before completion

This mode allows processing of **arbitrarily large SysEx messages** without allocating large buffers, making it ideal for memory-constrained systems.

### Buffered Mode

If no **SysEx callback** is provided, the parser buffers the entire SysEx message internally.

When the terminating `0xF7` is received, the complete SysEx packet is delivered through the standard message callback.

This mode is convenient for applications that prefer receiving complete SysEx packets instead of processing them byte-by-byte.

## Error Handling

The parser detects several malformed MIDI conditions, including:

- Orphan data bytes
- Unexpected data bytes
- SysEx end without a corresponding start
- SysEx buffer overflow

Errors are reported through an optional callback while the parser automatically resets its internal state to resume parsing subsequent messages.

## Design Goals

- Simple API
- Small memory footprint
- Deterministic execution
- No heap usage
- Easy integration into embedded firmware
- Fully event-driven architecture
