/*
 * midi_parser.h
 *
 *  Created on: 23 apr 2026
 *      Author: kzu0
 *
 *  Descrizione:
 *  Parser MIDI
 */

#ifndef MIDI_PARSER_H
#define MIDI_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MIDI message callboack
 *
 * @param status    Status byte (or 0xF0 for SysEx data bytes).
 * @param dat1      First data byte  (meaningful when data_len >= 1).
 * @param dat2      Second data byte (meaningful when data_len == 2).
 * @param data_len  Number of valid data bytes (0, 1, or 2).
 */

typedef void ( *midi_message_cb ) (

    uint8_t     status,
    uint8_t     dat1,
    uint8_t     dat2,
    uint8_t     data_count,
    int64_t     timestamp,
    void*       user );

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

typedef enum
{
    // Data byte ricevuto senza status attivo né running status
    MIDI_ERR_ORPHAN_DATA_BYTE = 0,

    // Data byte in eccesso rispetto a quelli attesi dal messaggio corrente
    MIDI_ERR_UNEXPECTED_DATA_BYTE,

    // 0xF7 ricevuto senza un SysEx aperto
    MIDI_ERR_SYSEX_END_WITHOUT_START,

    // Errore non classificato
    MIDI_ERR_UNKNOWN = 0xFF

} midi_error_t;

/**
 * MIDI error callboack
 *
 * @param error     Midi error.
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
 *      azzerato al completaemtno del messaggio
 *
 * - running status
 *      per il parsing dei channel message senza
 *      status byte
 *
 * - sysex
 *      flag per la ricezione di messaggi sysex
 *
 * - on_message
 *      callback che viene chiamata alla ricezione
 *      di un messaggio midi (non sys ex)
 *
 * - on_sysex
 *      callback che viene chiamata alla ricezione
 *      di dati sys ex, non bufferizza, streaming
 *      interface
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

    midi_message_cb on_message;
    midi_sysex_cb   on_sysex;
    midi_error_cb   on_error;

    void*           user;

} midi_ctx_t;

/**
 * Inizializzazione contesto midi parser
 *
 * @param ctx           puntatore al contesto per il parsing midi
 * @param message_cb    callback per i messaggi non sysex
 * @param sys_cb        callback per i dati sysex
 * @param sys_cb        callback per gli errori
 * @param user          puntatore da passare alla callback
 */
void init_midi_ctx( midi_ctx_t* ctx, midi_message_cb message_cb, midi_sysex_cb sys_cb, midi_error_cb err_cb, void* user );

/**
 * Parsing dato midi
 *
 * @param ctx           puntatore al contesto per il parsing midi
 * @param byte          dato midi
 */
void parse_byte ( midi_ctx_t* ctx, uint8_t byte , int64_t timestamp );

#ifdef __cplusplus
}
#endif

#endif // MIDI_PARSER_H
