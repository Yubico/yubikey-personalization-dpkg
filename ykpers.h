/* -*- mode:C; c-file-style: "bsd" -*- */
/*
 * Copyright (c) 2008, 2009, Yubico AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	__YKPERS_H_INCLUDED__
#define	__YKPERS_H_INCLUDED__

#include <stddef.h>
#include <stdbool.h>

#include <ykcore.h>

YK_CONFIG *ykp_create_config(void);
int ykp_free_config(YK_CONFIG *cfg);

int ykp_AES_key_from_hex(YK_CONFIG *cfg, const char *hexkey);
int ykp_AES_key_from_passphrase(YK_CONFIG *cfg, const char *passphrase,
				const char *salt);

int ykp_set_access_code(YK_CONFIG *cfg, unsigned char *access_code, size_t len);
int ykp_set_fixed(YK_CONFIG *cfg, unsigned char *fixed, size_t len);
int ykp_set_uid(YK_CONFIG *cfg, unsigned char *uid, size_t len);

int ykp_set_tktflag_TAB_FIRST(YK_CONFIG *cfg, bool state);
int ykp_set_tktflag_APPEND_TAB1(YK_CONFIG *cfg, bool state);
int ykp_set_tktflag_APPEND_TAB2(YK_CONFIG *cfg, bool state);
int ykp_set_tktflag_APPEND_DELAY1(YK_CONFIG *cfg, bool state);
int ykp_set_tktflag_APPEND_DELAY2(YK_CONFIG *cfg, bool state);
int ykp_set_tktflag_APPEND_CR(YK_CONFIG *cfg, bool state);

int ykp_set_cfgflag_SEND_REF(YK_CONFIG *cfg, bool state);
int ykp_set_cfgflag_TICKET_FIRST(YK_CONFIG *cfg, bool state);
int ykp_set_cfgflag_PACING_10MS(YK_CONFIG *cfg, bool state);
int ykp_set_cfgflag_PACING_20MS(YK_CONFIG *cfg, bool state);
int ykp_set_cfgflag_ALLOW_HIDTRIG(YK_CONFIG *cfg, bool state);
int ykp_set_cfgflag_STATIC_TICKET(YK_CONFIG *cfg, bool state);

int ykp_write_config(const YK_CONFIG *cfg,
		     int (*writer)(const char *buf, size_t count,
				   void *userdata),
		     void *userdata);
int ykp_read_config(YK_CONFIG *cfg,
		    int (*reader)(char *buf, size_t count,
				  void *userdata),
		    void *userdata);

extern int * const _ykp_errno_location(void);
#define ykp_errno (*_ykp_errno_location())
const char *ykp_strerror(int errnum);

#define YKP_ENOTYETIMPL	0x01
#define YKP_ENOCFG	0x02

#endif	// __YKPERS_H_INCLUDED__
