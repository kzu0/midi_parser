# MIDI Stream Parser

A lightweight, event-driven **MIDI 1.0 stream parser** designed for embedded systems.

The parser processes incoming MIDI bytes incrementally, supporting **Channel Messages**, **System Common Messages**, **System Real-Time Messages**, **Running Status**, and **System Exclusive (SysEx)** messages in compliance with the MIDI 1.0 specification.

The library performs **no dynamic memory allocation**, making it suitable for resource-constrained microcontrollers and real-time applications.

## Usage

The parser provides **two different SysEx processing modes**, allowing applications to choose the most appropriate approach.

### Streaming Mode

If a **SysEx callback** is supplied, SysEx messages are processed as a stream.

The application receives the following events:

- **START** – beginning of the SysEx message (`0xF0`)
- **DATA** – each received data byte
- **END** – End Of Exclusive (`0xF7`)
- **ABORT** – SysEx terminated by another status byte before completion

This mode allows processing of **arbitrarily large SysEx messages** without allocating large buffers, making it ideal for memory-constrained systems.

```c
static void on_message( uint8_t status, uint8_t dat1, uint8_t dat2,
                         uint8_t data_count, uint32_t sysex_count,
                         uint8_t* sysex_buffer, int64_t timestamp, void* user )
{
    // Handle channel message
}

static void on_sysex( midi_sysex_status_t t, uint8_t d, int64_t timestamp, void* u )
{
    // Handle sysex stream
}

int main( void )
{
    midi_ctx_t ctx;

    midi_init_ctx( &ctx, on_message, on_sysex, NULL, NULL );

    uint8_t byte;
    int64_t timestamp;

    while ( read_next_midi_byte( &byte, &timestamp ) )
    {
        midi_parse_byte( &ctx, byte, timestamp );
    }

    return 0;
}
```

### Buffered Mode

If no **SysEx callback** is provided, the parser buffers the entire SysEx message internally.

When the terminating `0xF7` is received, the complete SysEx packet is delivered through the standard message callback.

This mode is convenient for applications that prefer receiving complete SysEx packets instead of processing them byte-by-byte.

```c
static void on_message( uint8_t status, uint8_t dat1, uint8_t dat2,
                         uint8_t data_count, uint32_t sysex_count,
                         uint8_t* sysex_buffer, int64_t timestamp, void* user )
{
    if ( status == 0xF0 )
    {
        // Handle sysex message
    }
    else
    {
        // Handle channel message
    }
}

int main( void )
{
    midi_ctx_t ctx;

    // Pass NULL as sysex callback to use buffered mode
    midi_init_ctx( &ctx, on_message, NULL, NULL, NULL );

    uint8_t byte;
    int64_t timestamp;

    while ( read_next_midi_byte( &byte, &timestamp ) )
    {
        midi_parse_byte( &ctx, byte, timestamp );
    }

    return 0;
}
```
