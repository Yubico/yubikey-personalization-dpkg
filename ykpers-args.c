/* -*- mode:C; c-file-style: "bsd" -*- */
/*
 * Copyright (c) 2008, 2009, 2010, 2011, Yubico AB
 * Copyright (c) 2010  Tollef Fog Heen <tfheen@err.no>
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

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <ykpers.h>
#include <yubikey.h> /* To get yubikey_modhex_encode and yubikey_hex_encode */
#include <ykdef.h>

const char *usage =
"Usage: ykpersonalize [options]\n"
"-1        change the first configuration.  This is the default and\n"
"          is normally used for true OTP generation.\n"
"          In this configuration, TKTFLAG_APPEND_CR is set by default.\n"
"-2        change the second configuration.  This is for Yubikey II only\n"
"          and is then normally used for static key generation.\n"
"          In this configuration, TKTFLAG_APPEND_CR, CFGFLAG_STATIC_TICKET,\n"
"          CFGFLAG_STRONG_PW1, CFGFLAG_STRONG_PW2 and CFGFLAG_MAN_UPDATE\n"
"          are set by default.\n"
"-sFILE    save configuration to FILE instead of key.\n"
"          (if FILE is -, send to stdout)\n"
"-iFILE    read configuration from FILE.\n"
"          (if FILE is -, read from stdin)\n"
"-aXXX..   The AES secret key as a 32 (or 40 for OATH-HOTP/HMAC CHAL-RESP)\n"
"          char hex value (not modhex)\n"
"-cXXX..   A 12 char hex value (not modhex) to use as access code for programming\n"
"          (this does NOT SET the access code, that's done with -oaccess=)\n"
"-oOPTION  change configuration option.  Possible OPTION arguments are:\n"
"          salt=ssssssss       Salt to be used when deriving key from a\n"
"                              password.  If none is given, a unique random\n"
"                              one will be generated.\n"
"          fixed=xxxxxxxxxxx   The public identity of key, in MODHEX.\n"
"                              This is 0-16 characters long.\n"
"          uid=xxxxxx          The uid part of the generated ticket, in HEX.\n"
"                              MUST be 12 characters long.\n"
"          access=xxxxxxxxxxx  New access code to set, in HEX.\n"
"                              MUST be 12 characters long.\n"
"\n"
"          Ticket flags for all firmware versions:\n"
"          [-]tab-first           set/clear TAB_FIRST\n"
"          [-]append-tab1         set/clear APPEND_TAB1\n"
"          [-]append-tab2         set/clear APPEND_TAB2\n"
"          [-]append-delay1       set/clear APPEND_DELAY1\n"
"          [-]append-delay2       set/clear APPEND_DELAY2\n"
"          [-]append-cr           set/clear APPEND_CR\n"
"\n"
"          Ticket flags for firmware version 2.0 and above:\n"
"          [-]protect-cfg2        set/clear PROTECT_CFG2\n"
"\n"
"          Ticket flags for firmware version 2.1 and above:\n"
"          [-]oath-hotp           set/clear OATH_HOTP\n"
"\n"
"          Ticket flags for firmware version 2.2 and above:\n"
"          [-]chal-resp           set/clear CHAL_RESP\n"
"\n"
"          Configuration flags for all firmware versions:\n"
"          [-]send-ref            set/clear SEND_REF\n"
"          [-]pacing-10ms         set/clear PACING_10MS\n"
"          [-]pacing-20ms         set/clear PACING_20MS\n"
"          [-]static-ticket       set/clear STATIC_TICKET\n"
"\n"
"          Configuration flags for firmware version 1.x only:\n"
"          [-]ticket-first        set/clear TICKET_FIRST\n"
"          [-]allow-hidtrig       set/clear ALLOW_HIDTRIG\n"
"\n"
"          Configuration flags for firmware version 2.0 and above:\n"
"          [-]short-ticket        set/clear SHORT_TICKET\n"
"          [-]strong-pw1          set/clear STRONG_PW1\n"
"          [-]strong-pw2          set/clear STRONG_PW2\n"
"          [-]man-update          set/clear MAN_UPDATE\n"
"\n"
"          Configuration flags for firmware version 2.1 and above:\n"
"          [-]oath-hotp8          set/clear OATH_HOTP8\n"
"          [-]oath-fixed-modhex1  set/clear OATH_FIXED_MODHEX1\n"
"          [-]oath-fixed-modhex2  set/clear OATH_FIXED_MODHEX2\n"
"          [-]oath-fixed-modhex   set/clear OATH_MODHEX\n"
"\n"
"          Configuration flags for firmware version 2.2 and above:\n"
"          [-]chal-yubico         set/clear CHAL_YUBICO\n"
"          [-]chal-hmac           set/clear CHAL_HMAC\n"
"          [-]hmac-lt64           set/clear HMAC_LT64\n"
"          [-]chal-btn-trig       set/clear CHAL_BTN_TRIG\n"
"          oath-imf=IMF           set OATH Initial Moving Factor\n"
"\n"
"          Extended flags for firmware version 2.2 and above:\n"
"          [-]serial-btn-visible  set/clear SERIAL_BTN_VISIBLE\n"
"          [-]serial-usb-visible  set/clear SERIAL_USB_VISIBLE\n"
"          [-]serial-api-visible  set/clear SERIAL_API_VISIBLE\n"
"\n"
"-y        always commit (do not prompt)\n"
"\n"
"-v        verbose\n"
"-h        help (this text)\n"
;
const char *optstring = "12a:c:hi:o:s:vy";

static int hex_modhex_decode(unsigned char *result, size_t *resultlen,
			     const char *str, size_t strl,
			     size_t minsize, size_t maxsize,
			     bool primarily_modhex)
{
	if (strl >= 2) {
		if (strncmp(str, "m:", 2) == 0
		    || strncmp(str, "M:", 2) == 0) {
			str += 2;
			strl -= 2;
			primarily_modhex = true;
		} else if (strncmp(str, "h:", 2) == 0
			   || strncmp(str, "H:", 2) == 0) {
			str += 2;
			strl -= 2;
			primarily_modhex = false;
		}
	}

	if ((strl % 2 != 0) || (strl < minsize) || (strl > maxsize)) {
		return -1;
	}

	*resultlen = strl / 2;
	if (primarily_modhex) {
		if (yubikey_modhex_p(str)) {
			yubikey_modhex_decode((char *)result, str, strl);
			return 1;
		}
	} else {
		if (yubikey_hex_p(str)) {
			yubikey_hex_decode((char *)result, str, strl);
			return 1;
		}
	}

	return 0;
}

void report_yk_error()
{
	if (ykp_errno)
		fprintf(stderr, "Yubikey personalization error: %s\n",
			ykp_strerror(ykp_errno));
	if (yk_errno) {
		if (yk_errno == YK_EUSBERR) {
			fprintf(stderr, "USB error: %s\n",
				yk_usb_strerror());
		} else {
			fprintf(stderr, "Yubikey core error: %s\n",
				yk_strerror(yk_errno));
		}
	}
}

/*
 * Parse all arguments supplied to this program and turn it into mainly
 * a YKP_CONFIG (but return some other parameters as well, like
 * access_code, verbose etc.).
 *
 * Done in this way to be testable (see tests/test_args_to_config.c).
 */
int args_to_config(int argc, char **argv, YKP_CONFIG *cfg,
		   const char **infname, const char **outfname,
		   bool *autocommit, char *salt,
		   YK_STATUS *st, bool *verbose,
		   unsigned char *access_code, bool *use_access_code,
		   bool *aesviahash,
		   int *exit_code)
{
	char c;
	const char *aeshash = NULL;
	bool new_access_code = false;
	bool slot_chosen = false;
	bool mode_chosen = false;
	bool option_seen = false;

	struct config_st *ycfg = (struct config_st *) ykp_core_config(cfg);

	while((c = getopt(argc, argv, optstring)) != -1) {
		if (c == 'o') {
			if (strcmp(optarg, "oath-hotp") == 0 ||
			    strcmp(optarg, "chal-resp") == 0) {
				if (mode_chosen) {
					fprintf(stderr, "You may only choose mode (-ooath-hotp / "
						"-ochal-resp) once.\n");
					*exit_code = 1;
					return 0;
				}

				if (option_seen) {
					fprintf(stderr, "Mode choosing flags (oath-hotp / chal-resp) "
						"must be set prior to any other options (-o).\n");
					*exit_code = 1;
					return 0;
				}

				/* The default flags (particularly for slot 2) does not apply to
				 * these new modes of operation found in Yubikey >= 2.1. Therefor,
				 * we reset them here and, as a consequence of that, require the
				 * mode choosing options to be specified before any other.
				 */
				ycfg->tktFlags = 0;
				ycfg->cfgFlags = 0;
				ycfg->extFlags = 0;

				mode_chosen = 1;
			}

			option_seen = true;
		}

		switch (c) {
		case '1':
			if (slot_chosen) {
				fprintf(stderr, "You may only choose slot (-1 / -2) once.\n");
				*exit_code = 1;
				return 0;
			}
			if (option_seen) {
				fprintf(stderr, "You must choose slot before any options (-o).\n");
				*exit_code = 1;
				return 0;
			}
			if (!ykp_configure_for(cfg, 1, st))
				return 0;
			slot_chosen = true;
			break;
		case '2':
			if (slot_chosen) {
				fprintf(stderr, "You may only choose slot (-1 / -2) once.\n");
				*exit_code = 1;
				return 0;
			}
			if (option_seen) {
				fprintf(stderr, "You must choose slot before any options (-o).\n");
				*exit_code = 1;
				return 0;
			}
			if (!ykp_configure_for(cfg, 2, st))
				return 0;
			slot_chosen = true;
			break;
		case 'i':
			*infname = optarg;
			break;
		case 's':
			*outfname = optarg;
			break;
		case 'a':
			*aesviahash = true;
			aeshash = optarg;
			break;
		case 'c': {
			size_t access_code_len = 0;
			int rc = hex_modhex_decode(access_code, &access_code_len,
						   optarg, strlen(optarg),
						   12, 12, false);
			if (rc <= 0) {
				fprintf(stderr,
					"Invalid access code string: %s\n",
					optarg);
				*exit_code = 1;
				return 0;
			}
			if (!new_access_code)
				ykp_set_access_code(cfg,
						    access_code,
						    access_code_len);
			*use_access_code = true;
			break;
		}
		case 'o':
			if (strncmp(optarg, "salt=", 5) == 0)
				salt = strdup(optarg+5);
			else if (strncmp(optarg, "fixed=", 6) == 0) {
				const char *fixed = optarg+6;
				size_t fixedlen = strlen (fixed);
				unsigned char fixedbin[256];
				size_t fixedbinlen = 0;
				int rc = hex_modhex_decode(fixedbin, &fixedbinlen,
							   fixed, fixedlen,
							   0, 16, true);
				if (rc <= 0) {
					fprintf(stderr,
						"Invalid fixed string: %s\n",
						fixed);
					*exit_code = 1;
					return 0;
				}
				ykp_set_fixed(cfg, fixedbin, fixedbinlen);
			}
			else if (strncmp(optarg, "uid=", 4) == 0) {
				const char *uid = optarg+4;
				size_t uidlen = strlen (uid);
				unsigned char uidbin[256];
				size_t uidbinlen = 0;
				int rc = hex_modhex_decode(uidbin, &uidbinlen,
							   uid, uidlen,
							   12, 12, false);
				if (rc <= 0) {
					fprintf(stderr,
						"Invalid uid string: %s\n",
						uid);
					*exit_code = 1;
					return 0;
				}
				/* for OATH-HOTP and CHAL-RESP, uid is not applicable */
				if ((ycfg->tktFlags & TKTFLAG_OATH_HOTP) == TKTFLAG_OATH_HOTP ||
				    (ycfg->tktFlags & TKTFLAG_CHAL_RESP) == TKTFLAG_CHAL_RESP) {
					fprintf(stderr,
						"Option uid= not valid with -ooath-hotp or -ochal-resp.\n"
						);
					*exit_code = 1;
					return 0;
				}
				ykp_set_uid(cfg, uidbin, uidbinlen);
			}
			else if (strncmp(optarg, "access=", 7) == 0) {
				const char *acc = optarg+7;
				size_t acclen = strlen (acc);
				unsigned char accbin[256];
				size_t accbinlen = 0;
				int rc = hex_modhex_decode (accbin, &accbinlen,
							    acc, acclen,
							    12, 12, false);
				if (rc <= 0) {
					fprintf(stderr,
						"Invalid access code string: %s\n",
						acc);
					*exit_code = 1;
					return 0;
				}
				ykp_set_access_code(cfg, accbin, accbinlen);
				new_access_code = true;
			}
#define TKTFLAG(o, f)							\
			else if (strcmp(optarg, o) == 0) {		\
				if (!ykp_set_tktflag_##f(cfg, true)) {	\
					*exit_code = 1;			\
					return 0;		\
				}					\
			} else if (strcmp(optarg, "-" o) == 0) {	\
				if (! ykp_set_tktflag_##f(cfg, false)) { \
					*exit_code = 1;			\
					return 0;		\
				}					\
			}
			TKTFLAG("tab-first", TAB_FIRST)
			TKTFLAG("append-tab1", APPEND_TAB1)
			TKTFLAG("append-tab2", APPEND_TAB2)
			TKTFLAG("append-delay1", APPEND_DELAY1)
			TKTFLAG("append-delay2", APPEND_DELAY2)
			TKTFLAG("append-cr", APPEND_CR)
			TKTFLAG("protect-cfg2", PROTECT_CFG2)
			TKTFLAG("oath-hotp", OATH_HOTP)
			TKTFLAG("chal-resp", CHAL_RESP)
#undef TKTFLAG

#define CFGFLAG(o, f)							\
			else if (strcmp(optarg, o) == 0) {		\
				if (! ykp_set_cfgflag_##f(cfg, true)) {	\
					*exit_code = 1;			\
					return 0;			\
				}					\
			} else if (strcmp(optarg, "-" o) == 0) {	\
				if (! ykp_set_cfgflag_##f(cfg, false)) { \
					*exit_code = 1;			\
					return 0;			\
				}					\
			}
			CFGFLAG("send-ref", SEND_REF)
			CFGFLAG("ticket-first", TICKET_FIRST)
			CFGFLAG("pacing-10ms", PACING_10MS)
			CFGFLAG("pacing-20ms", PACING_20MS)
			CFGFLAG("allow-hidtrig", ALLOW_HIDTRIG)
			CFGFLAG("static-ticket", STATIC_TICKET)
			CFGFLAG("short-ticket", SHORT_TICKET)
			CFGFLAG("strong-pw1", STRONG_PW1)
			CFGFLAG("strong-pw2", STRONG_PW2)
			CFGFLAG("man-update", MAN_UPDATE)
			CFGFLAG("oath-hotp8", OATH_HOTP8)
			CFGFLAG("oath-fixed-modhex1", OATH_FIXED_MODHEX1)
			CFGFLAG("oath-fixed-modhex2", OATH_FIXED_MODHEX2)
			CFGFLAG("oath-fixed-modhex", OATH_FIXED_MODHEX)
			CFGFLAG("chal-yubico", CHAL_YUBICO)
			CFGFLAG("chal-hmac", CHAL_HMAC)
			CFGFLAG("hmac-lt64", HMAC_LT64)
			CFGFLAG("chal-btn-trig", CHAL_BTN_TRIG)
#undef CFGFLAG
			else if (strncmp(optarg, "oath-imf=", 9) == 0) {
				unsigned long imf;

				if (!(ycfg->tktFlags & TKTFLAG_OATH_HOTP) == TKTFLAG_OATH_HOTP) {
					fprintf(stderr,
						"Option oath-imf= only valid with -ooath-hotp or -ooath-hotp8.\n"
						);
					*exit_code = 1;
					return 0;
				}

				if (sscanf(optarg+9, "%lu", &imf) != 1 ||
				    /* yubikey limitations */
				    imf > 65535*16 || imf % 16 != 0) {
					fprintf(stderr,
						"Invalid value %s for oath-imf=.\n", optarg+9
						);
					*exit_code = 1;
					return 0;
				}
				if (! ykp_set_oath_imf(cfg, imf)) {
					*exit_code = 1;
					return 0;
				}
			}

#define EXTFLAG(o, f)							\
			else if (strcmp(optarg, o) == 0) {		\
				if (! ykp_set_extflag_##f(cfg, true)) {	\
					*exit_code = 1;			\
					return 0;			\
				}					\
			} else if (strcmp(optarg, "-" o) == 0) {	\
				if (! ykp_set_extflag_##f(cfg, false)) { \
					*exit_code = 1;			\
					return 0;			\
				}					\
			}
			EXTFLAG("serial-btn-visible", SERIAL_BTN_VISIBLE)
			EXTFLAG("serial-usb-visible", SERIAL_USB_VISIBLE)
			EXTFLAG("serial-api-visible", SERIAL_API_VISIBLE)
#undef EXTFLAG
			else {
				fprintf(stderr, "Unknown option '%s'\n",
					optarg);
				fputs(usage, stderr);
				*exit_code = 1;
				return 0;
			}
			break;
		case 'v':
			*verbose = true;
			break;
		case 'y':
			*autocommit = true;
			break;
		case 'h':
		default:
			fputs(usage, stderr);
			*exit_code = 0;
			return 0;
		}
	}

	if (*aesviahash) {
		bool long_key_valid = false;
		int res = 0;

		/* for OATH-HOTP, 160 bits key is also valid */
		if ((ycfg->tktFlags & TKTFLAG_OATH_HOTP) == TKTFLAG_OATH_HOTP)
			long_key_valid = true;

		/* for HMAC (not Yubico) challenge-response, 160 bits key is also valid */
		if ((ycfg->tktFlags & TKTFLAG_CHAL_RESP) == TKTFLAG_CHAL_RESP &&
		    (ycfg->cfgFlags & CFGFLAG_CHAL_HMAC) == CFGFLAG_CHAL_HMAC) {
			long_key_valid = true;
		}

		if (long_key_valid && strlen(aeshash) == 40) {
			res = ykp_HMAC_key_from_hex(cfg, aeshash);
		} else {
			res = ykp_AES_key_from_hex(cfg, aeshash);
		}

		if (res) {
			fprintf(stderr, "Bad %s key: %s\n", long_key_valid ? "HMAC":"AES", aeshash);
			fflush(stderr);
			return 0;
		}
	}

	return 1;
}
