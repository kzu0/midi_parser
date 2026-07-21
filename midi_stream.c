#include "midi_stream.h"
#include <assert.h>

static inline uint8_t expected_data_count ( uint8_t status )
{
    // System real time
    if ( status >= 0xF8 )
    {
        return 0;
    }

    // Channel messages
    switch( status & 0xf0 )
    {
    case 0x80:	// Note Off
    case 0x90:	// Note On
    case 0xA0:	// Poly key pressure
    case 0xB0:	// Control change
    case 0xE0:	// Pitch bend
        return 2;

    case 0xC0:	// Program change
    case 0xD0:	// Channel pressure
        return 1;
    }

    // System common messages
    switch ( status )
    {
    case 0xF0: // SysEx start
        return UINT8_MAX;

    case 0xf2:	// Song Position Pointer
        return 2;

    case 0xf1:	// MIDI Time Code Quarter Frame
    case 0xf3:	// Song Select
        return 1;

    case 0xf4:  // Undefined
    case 0xf5:  // Undefined
    case 0xf6:	// Tune Request
    case 0xf7:	// EOX (End of Exclusive)
        return 0;
    }

    return 0;
}

static void handle_error ( midi_ctx_t* ctx, midi_error_t err, int64_t timestamp )
{
    ctx->status = 0;

    ctx->running_status = 0;
    ctx->data_count = 0;

    ctx->sysex = false;
    ctx->sysex_count = 0;

    ctx->dat1 = 0;
    ctx->dat2 = 0;

    if ( ctx->on_error )
    {
        ctx->on_error( err, timestamp, ctx->user );
    }
}

static void abort_sysex ( midi_ctx_t* ctx, uint8_t byte, int64_t timestamp )
{
    if ( ctx->sysex && ctx->on_sysex )
    {
        ctx->on_sysex( MIDI_SYSEX_ABORT, byte, timestamp, ctx->user );
    }

    ctx->sysex = false;
    ctx->sysex_count = 0;
}

static void start_sysex ( midi_ctx_t* ctx, uint8_t byte, int64_t timestamp )
{
    ctx->sysex = true;
    ctx->sysex_buffer[0] = 0xF0;
    ctx->sysex_count = 1;

    if ( ctx->on_sysex )
    {
        ctx->on_sysex( MIDI_SYSEX_START, byte, timestamp, ctx->user );
    }
}

static void end_sysex ( midi_ctx_t* ctx, uint8_t byte, int64_t timestamp )
{
    // Chiusura SysEx arrivata prima dello start
    if ( !ctx->sysex )
    {
        handle_error( ctx, MIDI_ERR_SYSEX_END_WITHOUT_START, timestamp );
        return;
    }

    // Buffer overflow guard
    if ( ctx->sysex_count >= SYSEX_BUFFER_SIZE )
    {
        handle_error( ctx, MIDI_ERR_SYSEX_BUFFER_OVERFLOW, timestamp );
        return;
    }

    ctx->sysex_buffer[ctx->sysex_count++] = 0xF7;

    if ( ctx->on_sysex )
    {
        ctx->on_sysex( MIDI_SYSEX_END, byte, timestamp, ctx->user );
    }
    else
    {
        ctx->on_message( 0xF0, 0, 0, 0, ctx->sysex_count, ctx->sysex_buffer, timestamp, ctx->user );
    }

    ctx->sysex = false;
    ctx->sysex_count = 0;
}

static void handle_sysex_data ( midi_ctx_t* ctx, uint8_t byte, int64_t timestamp )
{
    // Buffer overflow guard
    if ( ctx->sysex_count >= SYSEX_BUFFER_SIZE )
    {
        handle_error( ctx, MIDI_ERR_SYSEX_BUFFER_OVERFLOW, timestamp );
        return;
    }

    ctx->sysex_buffer[ctx->sysex_count++] = byte;

    if ( ctx->on_sysex )
    {
        ctx->on_sysex( MIDI_SYSEX_DATA, byte, timestamp, ctx->user );
    }
}

void midi_parse_byte ( midi_ctx_t* ctx, uint8_t byte, int64_t timestamp )
{
    assert( ctx != NULL );

    /* ------------------------------------------------------------------
     * System Real Time (0xF8–0xFF)
     * These are single-byte messages that may appear anywhere in the
     * stream – even inside a SysEx or between data bytes of another
     * message – without disturbing the current parse state.
     * ---------------------------------------------------------------- */
    if ( byte >= 0xF8 )
    {
        if ( ctx->on_message )
        {
            ctx->on_message( byte, 0, 0, 0, 0, NULL, timestamp, ctx->user );
        }

        return;
    }

    /* ------------------------------------------------------------------
     * Status byte (0x80–0xF7)
     * ---------------------------------------------------------------- */
    if ( byte > 0x7f )
    {
        // SysEx Start (0xF0)
        if ( byte == 0xF0 )
        {
            ctx->status          = byte;
            ctx->running_status  = 0;
            ctx->data_count      = 0;

            abort_sysex( ctx, byte, timestamp );

            start_sysex( ctx, byte, timestamp );

            return;
        }

        // SysEx End (0xF7): terminates an open SysEx stream
        if ( byte == 0xF7 )
        {
            ctx->status          = 0;
            ctx->running_status  = 0;
            ctx->data_count      = 0;

            end_sysex( ctx, byte, timestamp );

            return;
        }

        // System Common (0xF1–0xF6): clear running status
        if ( byte >= 0xF1 && byte <= 0xF6 )
        {
            ctx->status         = byte;
            ctx->running_status = 0;
            ctx->data_count     = 0;

            abort_sysex( ctx, byte, timestamp );

            // Tune Request
            if ( byte == 0xF6 )
            {
                ctx->status = 0;

                if ( ctx->on_message )
                {
                    ctx->on_message( byte, 0, 0, 0, 0, NULL, timestamp, ctx->user );
                }
            }

            // Undefined
            if ( byte == 0xF4 || byte == 0xF5 )
            {
                ctx->status = 0;
            }

            return;
        }

        // Channel message (0x80–0xEF): set/update running status
        if ( byte >= 0x80 && byte <= 0xEF )
        {
            ctx->status         = byte;
            ctx->running_status = byte;
            ctx->data_count     = 0;

            abort_sysex( ctx, byte, timestamp );

            return;
        }

        return;
    }

    /* ------------------------------------------------------------------
     * Data byte (0x00–0x7F)
     * ---------------------------------------------------------------- */

    // System exclusive data byte
    if ( ctx->sysex )
    {
        handle_sysex_data( ctx, byte, timestamp );
        return;
    }

    // Running status
    uint8_t curr_status;

    if ( ctx->status >= 0x80 )
    {
        curr_status = ctx->status;
    }
    else if ( ctx->running_status )
    {
        curr_status = ctx->running_status;
    }
    else
    {
        // Data byte orfano
        handle_error( ctx, MIDI_ERR_ORPHAN_DATA_BYTE, timestamp );
        return;
    }

    // Messaggi contenenti dati
    if ( ( curr_status >= 0x80 && curr_status <= 0xEF ) || ( curr_status >= 0xF1 && curr_status <= 0xF3 ) )
    {
        ctx->data_count++;

        // Memorizzo il dato ricevuto
        if ( ctx->data_count == 1 )
        {
            ctx->dat1 = byte;
        }
        else if ( ctx->data_count == 2 )
        {
            ctx->dat2 = byte;
        }
        else
        {
            // Data count non valido
            handle_error( ctx, MIDI_ERR_UNEXPECTED_DATA_BYTE, timestamp );
            return;
        }

        uint32_t data_size = expected_data_count( curr_status );

        // Gestione messaggi completati
        if ( ctx->data_count == data_size )
        {
            // byte 0 = curr_status
            // byte 1 = dat1 (se data_size >= 1)
            // byte 2 = dat2 (se data_size == 2)

            if ( ctx->on_message )
            {
                ctx->on_message( curr_status, ctx->dat1, ctx->dat2, ctx->data_count, 0, NULL, timestamp, ctx->user );
            }

            ctx->status = 0;
            ctx->data_count = 0;

            ctx->dat1 = 0;
            ctx->dat2 = 0;

            return;
        }
    }
}

void midi_init_ctx(midi_ctx_t *ctx, midi_message_cb message_cb, midi_sysex_cb sys_cb, midi_error_cb err_cb, void *user)
{
    assert( ctx != NULL );

    ctx->status = 0;
    ctx->running_status = 0;
    ctx->dat1 = 0;
    ctx->dat2 = 0;
    ctx->data_count = 0;

    ctx->sysex = false;
    ctx->sysex_count = 0;

    ctx->on_message  = message_cb;
    ctx->on_sysex = sys_cb;
    ctx->on_error = err_cb;

    ctx->user = user;
}
