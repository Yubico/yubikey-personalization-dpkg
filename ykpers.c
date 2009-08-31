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

#include "ykcore_lcl.h"
#include "ykpbkdf2.h"
#include "yktsd.h"

#include <ykpers.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <yubikey.h>

struct ykp_config_t {
	unsigned int yk_major_version;
	unsigned int yk_minor_version;
	unsigned int configuration_number;

	YK_CONFIG ykcore_config;
};

static const YK_CONFIG default_config1 = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* fixed */
	{ 0, 0, 0, 0, 0, 0 },	/* uid */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* key */
	{ 0, 0, 0, 0, 0, 0 },	/* accCode */
	0,			/* fixedSize */
	0,			/* pgmSeq */
	TKTFLAG_APPEND_CR,	/* tktFlags */
	0,			/* cfgFlags */
	0,			/* ctrOffs */
	0			/* crc */
};

static const YK_CONFIG default_config2 = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* fixed */
	{ 0, 0, 0, 0, 0, 0 },	/* uid */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* key */
	{ 0, 0, 0, 0, 0, 0 },	/* accCode */
	0,			/* fixedSize */
	0,			/* pgmSeq */
	TKTFLAG_APPEND_CR,	/* tktFlags */
	/* cfgFlags */
	CFGFLAG_STATIC_TICKET | CFGFLAG_STRONG_PW1 | CFGFLAG_STRONG_PW2 | CFGFLAG_MAN_UPDATE,
	0,			/* ctrOffs */
	0			/* crc */
};

YKP_CONFIG *ykp_create_config(void)
{
	YKP_CONFIG *cfg = malloc(sizeof(YKP_CONFIG));
	if (cfg) {
		memcpy(&cfg->ykcore_config, &default_config1,
		       sizeof(default_config1));
		cfg->yk_major_version = 1;
		cfg->yk_minor_version = 3;
		cfg->configuration_number = 1;
		return cfg;
	}
	return 0;
}

int ykp_free_config(YKP_CONFIG *cfg)
{
	if (cfg) {
		free(cfg);
		return 1;
	}
	return 0;
}

int ykp_configure_for(YKP_CONFIG *cfg, int confnum, YK_STATUS *st)
{
	cfg->yk_major_version = st->versionMajor;
	cfg->yk_minor_version = st->versionMinor;

	switch(confnum) {
	case 1:
		memcpy(&cfg->ykcore_config, &default_config1,
		       sizeof(default_config1));
		cfg->configuration_number = 1;
		return 1;
	case 2:
		if (cfg->yk_major_version >= 2) {
			memcpy(&cfg->ykcore_config, &default_config2,
			       sizeof(default_config2));
			cfg->configuration_number = 2;
			return 1;
		}
		ykp_errno = YKP_EOLDYUBIKEY;
		break;
	default:
		ykp_errno = YKP_EINVCONFNUM;
		break;
	}
	return 0;
}

int ykp_AES_key_from_hex(YKP_CONFIG *cfg, const char *hexkey) {
	char aesbin[256];
	unsigned long int aeslong;

/* Make sure that the hexkey is exactly 32 characters */
	if (strlen(hexkey) != 32) {
		return 1;  /* Bad AES key */
	}

/* Make sure that the hexkey is made up of only [0-9a-f] */
	int i;
	for (i=0; i < strlen(hexkey); i++) {
		char c = tolower(hexkey[i]);
/* In ASCII, 0-9 == 48-57 and a-f == 97-102 */
		if ( c<48 || (c>57 && c<97) || c>102 ) {
			return 1;
		}
	}

	yubikey_hex_decode(aesbin, hexkey, sizeof(aesbin));
	memcpy(cfg->ykcore_config.key, aesbin, sizeof(cfg->ykcore_config.key));

	return 0;
}

int ykp_AES_key_from_passphrase(YKP_CONFIG *cfg, const char *passphrase,
				const char *salt)
{
	if (cfg) {
		char *random_places[] = {
			"/dev/srandom",
			"/dev/urandom",
			"/dev/random",
			0
		};
		char **random_place;
		uint8_t _salt[8];
		size_t _salt_len = 0;

		if (salt) {
			_salt_len = strlen(salt);
			if (_salt_len > 8)
				_salt_len = 8;
			memcpy(_salt, salt, _salt_len);
		} else {
			for (random_place = random_places;
			     *random_place;
			     random_place++) {
				FILE *random_file = fopen(*random_place, "r");
				if (random_file) {
					size_t read_bytes = 0;

					while (read_bytes < sizeof(_salt)) {
						size_t n = fread(&cfg->ykcore_config.key[read_bytes],
								 1, KEY_SIZE - read_bytes,
								 random_file);
						read_bytes += n;
					}

					fclose(random_file);

					_salt_len = sizeof(_salt);

					break; /* from for loop */
				}
			}
		}
		if (_salt_len == 0) {
			/* There was no randomness files, so create a cheap
			   salt from time */
#                       include <ykpbkdf2.h>

			time_t t = time(NULL);
			uint8_t output[256]; /* 2048 bits is a lot! */

			yk_hmac_sha1.prf_fn(passphrase, strlen(passphrase),
					    (char *)&t, sizeof(t),
					    output, sizeof(output));
			memcpy(_salt, output, sizeof(_salt));
			_salt_len = sizeof(_salt);
		}

		return yk_pbkdf2(passphrase,
				 _salt, _salt_len,
				 1024,
				 cfg->ykcore_config.key, sizeof(cfg->ykcore_config.key),
				 &yk_hmac_sha1);
	}
	return 0;
}

static bool vcheck_all(const YKP_CONFIG *cfg)
{
	return true;
}
static bool vcheck_v1(const YKP_CONFIG *cfg)
{
	return cfg->yk_major_version == 1;
}
static bool vcheck_no_v1(const YKP_CONFIG *cfg)
{
	return cfg->yk_major_version > 1;
}

#define def_set_charfield(fnname,fieldname,size,extra,vcheck)	\
int ykp_set_ ## fnname(YKP_CONFIG *cfg, unsigned char *input, size_t len)	\
{								\
	if (cfg) {						\
		size_t max_chars = len;				\
								\
		if (!vcheck(cfg)) {				\
			ykp_errno = YKP_EYUBIKEYVER;		\
			return 0;				\
		}						\
		if (max_chars > (size))				\
			max_chars = (size);			\
								\
		memcpy(cfg->ykcore_config.fieldname, (input), max_chars);	\
		memset(cfg->ykcore_config.fieldname + max_chars, 0,		\
		       (size) - max_chars);			\
		extra;						\
								\
		return 1;					\
	}							\
	ykp_errno = YKP_ENOCFG;					\
	return 0;						\
}

def_set_charfield(access_code,accCode,ACC_CODE_SIZE,,vcheck_all)
def_set_charfield(fixed,fixed,FIXED_SIZE,cfg->ykcore_config.fixedSize = max_chars,vcheck_all)
def_set_charfield(uid,uid,UID_SIZE,,vcheck_all)

#define def_set_tktflag(type,vcheck)				\
int ykp_set_tktflag_ ## type(YKP_CONFIG *cfg, bool state)	\
{								\
	if (cfg) {						\
		if (!vcheck(cfg)) {				\
			ykp_errno = YKP_EYUBIKEYVER;		\
			return 0;				\
		}						\
		if (state)					\
			cfg->ykcore_config.tktFlags |= TKTFLAG_ ## type;	\
		else						\
			cfg->ykcore_config.tktFlags &= ~TKTFLAG_ ## type;	\
		return 1;					\
	}							\
	ykp_errno = YKP_ENOCFG;					\
	return 0;						\
}

#define def_set_cfgflag(type,vcheck)				\
int ykp_set_cfgflag_ ## type(YKP_CONFIG *cfg, bool state)		\
{								\
	if (cfg) {						\
		if (!vcheck(cfg)) {				\
			ykp_errno = YKP_EYUBIKEYVER;		\
			return 0;				\
		}						\
		if (state)					\
			cfg->ykcore_config.cfgFlags |= CFGFLAG_ ## type;	\
		else						\
			cfg->ykcore_config.cfgFlags &= ~CFGFLAG_ ## type;	\
		return 1;					\
	}							\
	ykp_errno = YKP_ENOCFG;					\
	return 0;						\
}

def_set_tktflag(TAB_FIRST,vcheck_all)
def_set_tktflag(APPEND_TAB1,vcheck_all)
def_set_tktflag(APPEND_TAB2,vcheck_all)
def_set_tktflag(APPEND_DELAY1,vcheck_all)
def_set_tktflag(APPEND_DELAY2,vcheck_all)
def_set_tktflag(APPEND_CR,vcheck_all)
def_set_tktflag(PROTECT_CFG2,vcheck_no_v1)

def_set_cfgflag(SEND_REF,vcheck_all)
def_set_cfgflag(TICKET_FIRST,vcheck_v1)
def_set_cfgflag(PACING_10MS,vcheck_all)
def_set_cfgflag(PACING_20MS,vcheck_all)
def_set_cfgflag(ALLOW_HIDTRIG,vcheck_v1)
def_set_cfgflag(STATIC_TICKET,vcheck_all)
def_set_cfgflag(SHORT_TICKET,vcheck_no_v1)
def_set_cfgflag(STRONG_PW1,vcheck_no_v1)
def_set_cfgflag(STRONG_PW2,vcheck_no_v1)
def_set_cfgflag(MAN_UPDATE,vcheck_no_v1)


const char str_key_value_separator[] = ": ";
const char str_hex_prefix[] = "h:";
const char str_modhex_prefix[] = "m:";
const char str_fixed[] = "fixed";
const char str_uid[] = "uid";
const char str_key[] = "key";
const char str_acc_code[] = "acc_code";

const char str_flags_separator[] = "|";

struct map_st {
	uint8_t flag;
	const char *flag_text;
	bool (*vcheck)(const YKP_CONFIG *cfg);
};

const char str_ticket_flags[] = "ticket_flags";
struct map_st ticket_flags_map[] = {
	{ TKTFLAG_TAB_FIRST, "TAB_FIRST", vcheck_all },
	{ TKTFLAG_APPEND_TAB1, "APPEND_TAB1", vcheck_all },
	{ TKTFLAG_APPEND_TAB2, "APPEND_TAB2", vcheck_all },
	{ TKTFLAG_APPEND_DELAY1, "APPEND_DELAY1", vcheck_all },
	{ TKTFLAG_APPEND_DELAY2, "APPEND_DELAY2", vcheck_all },
	{ TKTFLAG_APPEND_CR, "APPEND_CR", vcheck_all },
	{ TKTFLAG_PROTECT_CFG2, "PROTECT_CFG2", vcheck_no_v1 },
	{ 0, "" }
};

const char str_config_flags[] = "config_flags";
struct map_st config_flags_map[] = {
	{ CFGFLAG_SEND_REF, "SEND_REF", vcheck_all },
	{ CFGFLAG_TICKET_FIRST, "TICKET_FIRST", vcheck_v1 },
	{ CFGFLAG_PACING_10MS, "PACING_10MS", vcheck_all },
	{ CFGFLAG_PACING_20MS, "PACING_20MS", vcheck_all },
	{ CFGFLAG_ALLOW_HIDTRIG, "ALLOW_HIDTRIG", vcheck_v1 },
	{ CFGFLAG_STATIC_TICKET, "STATIC_TICKET", vcheck_all },
	{ CFGFLAG_SHORT_TICKET, "SHORT_TICKET", vcheck_no_v1 },
	{ CFGFLAG_STRONG_PW1, "STRONG_PW1", vcheck_no_v1 },
	{ CFGFLAG_STRONG_PW2, "STRONG_PW2", vcheck_no_v1 },
	{ CFGFLAG_MAN_UPDATE, "MAN_UPDATE", vcheck_no_v1 },
	{ 0, "" }
};

int ykp_write_config(const YKP_CONFIG *cfg,
		     int (*writer)(const char *buf, size_t count,
				   void *userdata),
		     void *userdata)
{
	if (cfg) {
		char buffer[256];
		struct map_st *p;

		writer(str_fixed, strlen(str_fixed), userdata);
		writer(str_key_value_separator,
		       strlen(str_key_value_separator),
		       userdata);
		writer(str_modhex_prefix,
		       strlen(str_key_value_separator),
		       userdata);
		yubikey_modhex_encode(buffer, cfg->ykcore_config.fixed, cfg->ykcore_config.fixedSize);
		writer(buffer, strlen(buffer), userdata);
		writer("\n", 1, userdata);

		writer(str_uid, strlen(str_uid), userdata);
		writer(str_key_value_separator,
		       strlen(str_key_value_separator),
		       userdata);
		writer(str_hex_prefix,
		       strlen(str_key_value_separator),
		       userdata);
		yubikey_hex_encode(buffer, cfg->ykcore_config.uid, UID_SIZE);
		writer(buffer, strlen(buffer), userdata);
		writer("\n", 1, userdata);

		writer(str_key, strlen(str_key), userdata);
		writer(str_key_value_separator,
		       strlen(str_key_value_separator),
		       userdata);
		writer(str_hex_prefix,
		       strlen(str_key_value_separator),
		       userdata);
		yubikey_hex_encode(buffer, cfg->ykcore_config.key, KEY_SIZE);
		writer(buffer, strlen(buffer), userdata);
		writer("\n", 1, userdata);

		writer(str_acc_code, strlen(str_acc_code), userdata);
		writer(str_key_value_separator,
		       strlen(str_key_value_separator),
		       userdata);
		writer(str_hex_prefix,
		       strlen(str_key_value_separator),
		       userdata);
		yubikey_hex_encode(buffer, cfg->ykcore_config.accCode, ACC_CODE_SIZE);
		writer(buffer, strlen(buffer), userdata);
		writer("\n", 1, userdata);

		buffer[0] = '\0';
		for (p = ticket_flags_map; p->flag; p++) {
			if (cfg->ykcore_config.tktFlags & p->flag
			    && p->vcheck(cfg)) {
				if (*buffer) {
					strcat(buffer, str_flags_separator);
					strcat(buffer, p->flag_text);
				} else {
					strcpy(buffer, p->flag_text);
				}
			}
		}
		writer(str_ticket_flags, strlen(str_ticket_flags), userdata);
		writer(str_key_value_separator,
		       strlen(str_key_value_separator),
		       userdata);
		writer(buffer, strlen(buffer), userdata);
		writer("\n", 1, userdata);

		buffer[0] = '\0';
		for (p = config_flags_map; p->flag; p++) {
			if (cfg->ykcore_config.cfgFlags & p->flag
			    && p->vcheck(cfg)) {
				if (*buffer) {
					strcat(buffer, str_flags_separator);
					strcat(buffer, p->flag_text);
				} else {
					strcpy(buffer, p->flag_text);
				}
			}
		}
		writer(str_config_flags, strlen(str_config_flags), userdata);
		writer(str_key_value_separator,
		       strlen(str_key_value_separator),
		       userdata);
		writer(buffer, strlen(buffer), userdata);
		writer("\n", 1, userdata);

		return 1;
	}
	return 0;
}
int ykp_read_config(YKP_CONFIG *cfg,
		    int (*reader)(char *buf, size_t count,
				  void *userdata),
		    void *userdata)
{
	ykp_errno = YKP_ENOTYETIMPL;
	return 0;
}

YK_CONFIG *ykp_core_config(YKP_CONFIG *cfg)
{
	if (cfg)
		return &cfg->ykcore_config;
	ykp_errno = YKP_ENOCFG;
	return 0;
}

int ykp_config_num(YKP_CONFIG *cfg)
{
	if (cfg)
		return cfg->configuration_number;
	ykp_errno = YKP_ENOCFG;
	return 0;
}

int * const _ykp_errno_location(void)
{
	static int tsd_init = 0;
	static int nothread_errno = 0;
	YK_DEFINE_TSD_METADATA(errno_key);
	int rc = 0;

	if (tsd_init == 0) {
		if ((rc = YK_TSD_INIT(errno_key, free)) == 0) {
			YK_TSD_SET(errno_key, calloc(1, sizeof(int)));
			tsd_init = 1;
		} else {
			tsd_init = -1;
		}
	}
	if (tsd_init == 1) {
		return YK_TSD_GET(int *, errno_key);
	}
	return &nothread_errno;
}

static const char *errtext[] = {
	"",
	"not yet implemented",
	"no configuration structure given",
	"option not available for this Yubikey version",
	"too old yubikey for this operation",
	"invalid configuration number (this is a programming error)",
};
const char *ykp_strerror(int errnum)
{
	if (errnum < sizeof(errtext)/sizeof(errtext[0]))
		return errtext[errnum];
	return NULL;
}
