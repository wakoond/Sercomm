
/*
 * Serial message generator and parser for embedded systems
 *
 * Author: Andras Takacs <andras.takacs@emsol.hu>
 *
 * Version 0.4
 *
 * Copyright (c) 2009-2012 Emsol Mernok Iroda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the Emsol Mernok Iroda.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _SERCOMM_H
#define _SERCOMM_H

#include <inttypes.h>
#include <stddef.h>         /* for offsetof */

//#define SERCOMM_USE_TINY_SC

#ifdef SERCOMM_USE_TINY_SC
typedef uint8_t								sc_size_t
typedef uint8_t								sc_cmd_t
typedef uint8_t								sc_cctrl_t
/*! \brief Ignore message validity check. Use in message_max_len in struct sercomm */
#define SERCOMM_IGNORE_MSG_VALID_LENGTH		UINT8_MAX
#else
typedef uint32_t							sc_size_t
typedef uint32_t							sc_cmd_t
typedef uint32_t							sc_cctrl_t
/*! \brief Ignore message validity check. Use in message_max_len in struct sercomm */
#define SERCOMM_IGNORE_MSG_VALID_LENGTH		UINT32_MAX
#endif
/*! \brief Omit reset sequency usage. Use in reset_bytes in struct sercomm */
#define SERCOMM_OMIT_RESET					UINT8_MAX

/*!
 * \brief Sercomm configuration
 *
 * This struct sets the header configuration of the serial messages, and
 * it is used for internal storage, while parsing messages.
 *
 * To use tiny use (smaller struct sercomm size) define SERCOMM_USE_TINY_SC!
 *
 * Example:
 * \code
 * static unsigned char comm_buffer[COMM_BUFFER_SIZE];
 * static struct sercomm sc = {
 *     .frame_start = { 0x00, 0x01, 0x02, 0x03 },
 *     .frame_start_bytes = 4,
 *     .cmd_bytes = 1,
 *     .ts_bytes = 4,
 *     .ts = add_timestamp,
 *     .len_bytes = 1,
 *     .hash_bytes = 32,
 *     .hash = gen_crc_hash,
 *     .comm_ctrl_bytes = 1,
 *     .message_max_len = 8,
 *     .message_valid_len = 8,
 *     .reset_byte = 0xFF,
 *     .reset_bytes = 51,
 *     .reset = do_reset,
 *     .buffer = comm_buffer,
 *     .buffer_size = COMM_BUFFER_SIZE,
 * };
 * \endcode
 *
 * <b>Message format:</b>
 * \code
 * +----------------+--------------+
 * |                |              |
 * | Sercomm header | Message body |
 * |                |              |
 * +----------------+--------------+
 * \endcode
 * The deatils of the Sercomm header are below.
 *
 * <b>Sercomm header format:</b>
 * \code
 * +-------------+---------+-----------+----------------+----------------+------+
 * |             |         |           |                |                |      |
 * | Frame start | Command | Timestamp | Message length | Comm. controll | Hash |
 * |             |         |           |                |                |      |
 * +-------------+---------+-----------+----------------+----------------+------+
 * \endcode
 * - Frame start: Start sequency
 * - Command: Message type / Command code
 * - Timestamp: Message timestamp or sequence number (optional)
 * - Message length: The length of the message without the header
 * - Communication controll: Additional comm. controll command, i.e., Close connection (optional)
 * - Hash: Hash or integrity check, i.e., CRC
 *
 * Before the hash generation, the hash field is be zero!
 *
 * <b>Minimal Sercomm header format with only the required fields:</b>
 * \code
 * +-------------+---------+----------------+----------------+
 * |             |         |                |                |
 * | Frame start | Command | Message length | Comm. controll |
 * |             |         |                |                |
 * +-------------+---------+----------------+----------------+
 * \endcode 
 */
struct sercomm {
	/* The length of the Command field (number of bytes). */
	uint8_t			cmd_bytes;
	/*! The length of the Timestamp field (number of bytes). Zero to omit. */
	uint8_t			ts_bytes;
	/*! Timestamp callback: ts arguments points to the beginning of the Timestamp field. */
	void            (* ts)(void * ts);
	/*! The length of the Message length field (number of bytes). */
	uint8_t			len_bytes;
	/*! The length of the Hash field (number of bytes). Zero to omit. */
	uint8_t			hash_bytes;
	/*! Hash callback: hashptr points to the beginning of the Hash field, msg and mlen are the message and its length */
    void            (* hash)(unsigned char * hashptr, unsigned char * msg, int mlen);
	/*! The length of the communication controll field (number of bytes). Zero to omit. */
    uint8_t         comm_ctrl_bytes;
	/*! The length of the frame start field (number of bytes). */
	uint8_t 		frame_start_bytes;
	/*! The reset byte. A sequence of reset_bytes number of it will be call the reset function */
    unsigned char   reset_byte;
	/*! The number of the reset_byte byte. A sequence of reset_bytes number of reset_byte will be call the reset function */
    uint8_t         reset_bytes;
	/*! Reset callback. It will be called if a reset sequence received. It could be use to reset any message processing mechanisms */
	void            (* reset)(void);
	/*! Buffer. It should to be an enogh big array. Use an array which could store at least two messages */
    unsigned char * buffer;
	/*! The size of the Buffer */
    sc_size_t       buffer_size;
	/*! For message validition: If all of the messages have he same size, use it instead of message_max_len. To ommit this check: SERCOMM_IGNORE_MSG_VALID_LENGTH */
    sc_size_t		message_valid_len;
	/*! For message validation: The maximum length of a message (without the header). */
	sc_size_t		message_max_len;
	/*! Last priv argument of command callback (fn) in struct sercomm_msg */
    void *          priv;
	/*! Internal usage: The current number of bytes in the buffer */
    sc_size_t       buffer_len;
	/*! Internal usge: The message (body) length of the currently parsed message */
    sc_size_t       message_len;
	/*! Internal usage: The number of the received reset bytes */
	uint8_t         buffer_reset_bytes;
	/*! Array of the frame start bytes. See the example */
	unsigned char   frame_start[];
};

/*! 
 * \brief The size of the struct sercomm with the dynamic frame_start part 
 *
 * \param frame_start_length The length of the frame_start array 
 */
#define SIZEOF_SC(frame_start_length) \
    ( offsetof(struct sercomm, frame_start) + (frame_start_length) * sizeof ((struct sercomm *)0)->frame_start[0] )

/*!
 * \brief Sercomm message and command definition
 *
 * Use this struct to define each message commands and their parser functions.
 * If a message successfully received and validated the fn callbacck will be called for further processing.
 *
 * The last entry of the array should to be {0, NULL}!
 *
 * Example usage:
 * \code
 * static struct sercomm_msg sms[] = {
 *		{ MSG_COMMAND_PRESENT,		cmd_present },
 *		{ MSG_COMMAND_ALARM,		cmd_alarm },
 *		{ MSG_COMMAND_CALIBRATE,	cmd_calibrate },
 *		{ MSG_COMMAND_ALARM_CLEAR,	cmd_alarm_clear },
 *		{ MSG_COMMAND_VALIDATE_COMM,cmd_validate_comm },
 *		{ MSG_COMMAND_BEEP,			cmd_do_beep },
 *		{ MSG_COMMAND_ACK1,			cmd_ack1 },
 *		{ MSG_COMMAND_ACK2,			cmd_ack2 },
 *		{ 0,        NULL }
 * };
 * \endcode
 */
struct sercomm_msg {
	/*! Message command value */
	sc_cmd_t 		cmd;
	/*! 
	 * Processing callback. The first argument is the beginning of the timestamp field; the second is the message length;
	 * the third is the beginning of the message; the fourth is the value of the comm. control field;
	 * and the last is the priv field of struct sercomm
	 */
    void            (* fn)(unsigned char * ts, sc_size_t mlen, unsigned char * msg, sc_cctrl_t comm_ctrl, void * priv);
};

/*!
 * \brief Create a message with sercomm header
 *
 * This function cretates a message with proper header configuration.
 * After return the message is ready for sending.
 *
 * Example usage:
 * \code
 * uint8_t tmp;
 * tmp = sc_make_message(&sc, MSG_COMMAND_ALARM, MSG_CCTRL_NONE, message_body, message_body_len, comm_buffer, COMM_BUFFER_SIZE);
 * uart_send_message(comm_buffer, tmp);
 * \endcode
 *
 * \param sercomm The main struct sercom
 * \param cmd Message command value
 * \param cctrl The value of the comm. control field
 * \param msg Message body
 * \param mlen The length of the message body
 * \param output The output buffer. The message with the header will be generated here.
 * \param olen The size of the output buffer
 *
 * \return The length of the message with the header, or zero if error occured
 */
sc_size_t sc_make_message(struct sercomm * sc, sc_cmd_t cmd, sc_cctrl_t cctrl,
        unsigned char * msg, sc_size_t mlen,
        unsigned char * output, sc_size_t olen);

/*!
 * \brief Get and parse a message
 *
 * This function gets the message byte to byte. It build the entire message from the received bytes.
 * If the message is valid, it calls the callback of the command.
 *
 * It searches the beginng of the message. It shoudl to be the frame start sequence.
 * The first validation will be proceeded after the receiving of the message hader. 
 * The next, after the receiving of the full message.
 *
 * Example:
 * \code
 * static void main_get_message(uint8_t byte)
 * {
 *		sc_get_message(&sc, sms, byte);
 * }
 * \endcode
 *
 * \param sc The main struct sercomm
 * \param sm The struct sercomm_msg array
 * \param byte The received byte
 */
void sc_get_message(struct sercomm * sc, struct sercomm_msg * sm, 
        unsigned char byte);

#endif
