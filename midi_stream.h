/*
 * midi_stream.h
 *
 *  Created on: 23 Apr 2026
 *      Author: kzu0
 *
 *  Description:
 *  Lightweight MIDI 1.0 stream parser designed for embedded systems.
 *
 *  The parser processes incoming MIDI bytes incrementally, handling
 *  Channel Messages, System Common Messages, System Real-Time Messages,
 *  Running Status, and System Exclusive (SysEx) messages according to
 *  the MIDI specification.
 *
 *  SysEx messages can be handled in two different ways:
 *
 *    - Streaming mode:
 *      When a SysEx callback is provided, the parser delivers START,
 *      DATA, END, and ABORT events as bytes are received, allowing
 *      applications to process arbitrarily long SysEx messages without
 *      local buffering.
 *
 *    - Buffered mode:
 *      When no SysEx callback is provided, the parser stores the entire
 *      SysEx message in an internal buffer and delivers it as a complete
 *      message through the standard MIDI message callback once the End
 *      of Exclusive (0xF7) byte is received.
 *
 *  The parser is fully event-driven and does not perform dynamic memory
 *  allocation, making it suitable for resource-constrained embedded
 *  applications.
 */

#ifndef MIDI_STREAM_H
#define MIDI_STREAM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYSEX_BUFFER_SIZE 1024

/**
 * MIDI message callboack
 *
 * @param status        Status byte (or 0xF0 for SysEx data bytes).
 * @param dat1          First data byte  (meaningful when data_len >= 1).
 * @param dat2          Second data byte (meaningful when data_len == 2).
 * @param data_count    Number of valid data bytes (0, 1, or 2).
 * @param sysex_count   Number of sysex bytes
 * @param sysex_buffer  Pointer to sysex buffer
 * @param timestamp     Temporal timestamp.
 * @param user          User data pointer to pass to callbacks.
 */

typedef void ( *midi_message_cb ) (

    uint8_t     status,
    uint8_t     dat1,
    uint8_t     dat2,
    uint8_t     data_count,
    uint32_t    sysex_count,
    uint8_t*    sysex_buffer,
    int64_t     timestamp,
    void*       user );

/**
 * Sys ex State Machine state
 */
typedef enum
{
    MIDI_SYSEX_START,
    MIDI_SYSEX_DATA,
    MIDI_SYSEX_END,
    MIDI_SYSEX_ABORT

} midi_sysex_status_t;

/**
 * MIDI sys ex data callboack
 *
 * @param dat       Status byte (or 0xF0 for SysEx data bytes).
 */
typedef void ( *midi_sysex_cb ) (

    midi_sysex_status_t type,
    uint8_t             dat,
    int64_t             timestamp,
    void*               user );

/**
 * Error codes
 */
typedef enum
{
    // Data byte ricevuto senza status attivo né running status
    MIDI_ERR_ORPHAN_DATA_BYTE = 0,

    // Data byte in eccesso rispetto a quelli attesi dal messaggio corrente
    MIDI_ERR_UNEXPECTED_DATA_BYTE,

    // 0xF7 ricevuto senza un SysEx aperto
    MIDI_ERR_SYSEX_END_WITHOUT_START,

    // Buffer Overflow
    MIDI_ERR_SYSEX_BUFFER_OVERFLOW,

    // Errore non classificato
    MIDI_ERR_UNKNOWN = 0xFF

} midi_error_t;

/**
 * MIDI error callboack
 *
 * @param error     Error code.
 * @param timestamp Temporal timestamp.
 * @param user      User data pointer to pass to callbacks.
 */
typedef void ( *midi_error_cb ) (

    midi_error_t error,
    int64_t      timestamp,
    void*        user );

/**
 * @brief Contesto per parsing midi
 *
 * - status
 *      mantiene l'ultimo status byte ricevuto,
 *      azzerato al completamento del messaggio
 *
 * - running_status
 *      per il parsing dei channel message senza
 *      status byte (vedi running status su
 *      specifica midi)
 *
 * - dat1 & dat2
 *      memorizzano i byte di dato per i messaggi
 *      con uno o due byte di dato
 *
 * - data_count
 *      accumulatore che tiene conto del numero
 *      di byte ricevuti
 *
 * - sysex
 *      flag per la ricezione di messaggi sysex
 *
 * - sysex_count
 *      accumulatore dei dati sysex
 *
 * - sysex_buffer
 *      buffer per messaggi sysex
 *
 * - on_message
 *      callback che viene chiamata alla ricezione
 *      di un messaggio midi
 *
 * - on_sysex
 *      callback che viene chiamata alla ricezione
 *      di dati sysex, non bufferizza, streaming
 *      interface. Se è NULL, i messaggi sysex
 *      vengono elaborati internamente e riportati
 *      da on_message
 *
 * - on_error
 *      callback che viene chiamata quando si
 *      verifica un errore nel parsing di un
 *      byte midi
 *
 * - user
 *      puntatore da passare alle callback
 */
typedef struct {

    uint8_t         status;
    uint8_t         running_status;
    uint8_t         dat1;
    uint8_t         dat2;
    uint8_t         data_count;

    bool            sysex;
    uint32_t        sysex_count;
    uint8_t         sysex_buffer[SYSEX_BUFFER_SIZE];

    midi_message_cb on_message;
    midi_sysex_cb   on_sysex;
    midi_error_cb   on_error;

    void*           user;

} midi_ctx_t;

/**
 * Inizializzazione contesto midi parser
 *
 * @param ctx           puntatore al contesto per il parsing midi
 * @param message_cb    callback per i messaggi
 * @param sys_cb        callback per i dati sysex (data stream, se NULL i messaggi sysex passano per message_cb)
 * @param err_cb        callback per gli errori
 * @param user          puntatore da passare alle callback
 */
void midi_init_ctx( midi_ctx_t* ctx, midi_message_cb message_cb, midi_sysex_cb sys_cb, midi_error_cb err_cb, void* user );

/**
 * Parsing dato midi
 *
 * @param ctx           Puntatore al contesto per il parsing midi
 * @param byte          Dato midi
 * @param timestamp     Temporal timestamp.
 */
void midi_parse_byte ( midi_ctx_t* ctx, uint8_t byte , int64_t timestamp );

#ifdef __cplusplus
}
#endif

#endif // MIDI_STREAM_H
