
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

#include <stdio.h>
#include <string.h>

#include "sercomm.h"

static void put_field(unsigned char * dst, uint32_t src, uint8_t len)
{
    switch (len) {
        case 1:
            *(uint8_t *)dst = (uint8_t)src;
            break;
        case 2:
            *(uint16_t *)dst = (uint16_t)src;
            break;
        case 4:
            *(uint32_t *)dst = (uint32_t)src;
            break;
        //default:
            //error
    }
}

static void get_field(uint32_t * dst, unsigned char * src, uint8_t len)
{
    switch (len) {
        case 1:
            *dst = *(uint8_t *)src;
            break;
        case 2:
            *dst = *(uint16_t *)src;
            break;
        case 4:
            *dst = *(uint32_t *)src;
            break;
        //default:
            //error
    }
}

sc_size_t sc_make_message(struct sercomm * sc, sc_cmd_t cmd, sc_cctrl_c cctrl,
        unsigned char * msg, sc_size_t mlen,
        unsigned char * output, sc_size_t olen)
{
    sc_size_t x, sumlen, hashlen;

    sumlen = 
        sc->frame_start_bytes +
        sc->cmd_bytes +
        sc->ts_bytes +
        sc->len_bytes +
        mlen +
        sc->hash_bytes +
        sc->comm_ctrl_bytes;

    if (sumlen > olen)
		return 0;
	if (mlen > 0 && msg == NULL)
        return 0;

    if (sc->frame_start_bytes > 0)
        memcpy(output, sc->frame_start, sc->frame_start_bytes);
    x = sc->frame_start_bytes;
    put_field(&output[x], cmd, sc->cmd_bytes);
    x += sc->cmd_bytes;
    if (sc->ts_bytes > 0 && sc->ts != NULL)
        sc->ts(&output[x]);
    x += sc->ts_bytes;
    put_field(&output[x], mlen, sc->len_bytes);
    x += sc->len_bytes;
    memcpy(&output[x], msg, mlen);
    x += mlen;
    if (sc->hash_bytes > 0)
        memset(&output[x], 0, sc->hash_bytes);
    x += sc->hash_bytes;
	if (sc->comm_ctrl_bytes > 0)
	    put_field(&output[x], cctrl, sc->comm_ctrl_bytes);
    x -= sc->hash_bytes;
    hashlen = 
        sc->cmd_bytes +
        sc->ts_bytes +
        sc->len_bytes +
        mlen;
    if (sc->hash_bytes > 0 && sc->hash != NULL)
        sc->hash(&output[x], &output[sc->frame_start_bytes], hashlen); 

    return sumlen;
}

static void shift_message(struct sercomm * sc, uint8_t offset, uint8_t amount)
{
    int i;

    for (i = offset; i < amount + offset; i++) {
        sc->buffer[i - offset] = sc->buffer[i];
    }
    sc->buffer_len -= offset;
}

void sc_get_message(struct sercomm * sc, struct sercomm_msg * sm, 
        unsigned char byte)
{
	sc_size_t x, sum1, sum2;
    sc_cmd_t cmd = 0;
	sc_cctrl_t cc = 0;

    sc->buffer[sc->buffer_len++] = byte;

    if (sc->reset_bytes != SERCOMM_OMIT_RESET) {
        if (byte == sc->reset_byte)
            sc->buffer_reset_bytes++;
        else
            sc->buffer_reset_bytes = 0;
        if (sc->reset_bytes == sc->buffer_reset_bytes) {
            if (sc->reset != NULL)
                sc->reset();
            sc->buffer_len = 0;
            return;
        }
    }

    sum1 = 
        sc->frame_start_bytes + 
        sc->cmd_bytes + 
        sc->ts_bytes + 
        sc->len_bytes;
    sum2 =
        sc->frame_start_bytes +
        sc->cmd_bytes +
        sc->ts_bytes +
        sc->len_bytes +
        sc->message_len +
        sc->hash_bytes +
        sc->comm_ctrl_bytes;

    if (sc->buffer_len == sc->frame_start_bytes) {
        if (memcmp(sc->buffer, sc->frame_start, sc->frame_start_bytes)) {
            //If not match, drop it!
            shift_message(sc, 1, sc->frame_start_bytes - 1);
        } 
    } else if (sc->buffer_len == sum1) {
        get_field(&sc->message_len, &sc->buffer[sum1 - sc->len_bytes], sc->len_bytes);
		if (sc->message_valid_len != SERCOMM_IGNORE_MSG_VALID_LENGTH) {
			if (sc->message_len != sc->message_valid_len) {
				//If not match, drop it!
				sc->buffer_len = 0;
			}
		} else {
			if (sc->message_len > sc->message_max_len) {
				//If not match, drop it!
				sc->buffer_len = 0;
			}
		}
    } else if (sc->buffer_len == sum2) {
        if (sc->hash != NULL) {
            sum2 = 
                sc->cmd_bytes +
                sc->ts_bytes +
                sc->len_bytes +
                sc->message_len;
            sc->hash(&sc->buffer[sc->buffer_len], &sc->buffer[sc->frame_start_bytes], sum2);
            sum2 += sc->frame_start_bytes;
            if (memcmp(&sc->buffer[sc->buffer_len], &sc->buffer[sum2], sc->hash_bytes)) {
                //If not match, drop it!
                sc->buffer_len = 0;
            } else {
                get_field(&cmd, &sc->buffer[sc->frame_start_bytes], sc->cmd_bytes);
				if (sc->comm_ctrl_bytes > 0)
	                get_field(&cc, &sc->buffer[sc->buffer_len - sc->comm_ctrl_bytes], sc->comm_ctrl_bytes);
				else
					cc = 0;
                for (x = 0; sm[x].fn != NULL; x++) {
                    if (sm[x].cmd == cmd) {
                        sm[x].fn(&sc->buffer[sc->frame_start_bytes + sc->cmd_bytes],
                                sc->message_len, &sc->buffer[sum1], cc, sc->priv);
                        break;
                    }
                }
                sc->buffer_len = 0;
            }
        }
    }
}

