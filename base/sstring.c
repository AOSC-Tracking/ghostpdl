/* Copyright (C) 2001-2023 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  39 Mesa Street, Suite 108A, San Francisco,
   CA 94129, USA, for further information.
*/


/* String and hexstring streams (filters) */
#include "stdio_.h"		/* includes std.h */
#include "memory_.h"
#include "string_.h"
#include "strimpl.h"
#include "sstring.h"
#include "scanchar.h"

/* ------ ASCIIHexEncode ------ */

private_st_AXE_state();

/* Initialize the state */
static int
s_AXE_init(stream_state * st)
{
    stream_AXE_state *const ss = (stream_AXE_state *) st;

    return s_AXE_init_inline(ss);
}

/* Process a buffer */
static int
s_AXE_process(stream_state * st, stream_cursor_read * pr,
              stream_cursor_write * pw, bool last)
{
    stream_AXE_state *const ss = (stream_AXE_state *) st;
    const byte *p = pr->ptr;
    byte *q = pw->ptr;
    int rcount = pr->limit - p;
    int wcount = pw->limit - q;
    int count;
    int pos = ss->count;
    const char *hex_digits = "0123456789ABCDEF";
    int status = 0;

    if (last && ss->EndOfData)
        wcount--;		/* leave room for '>' */
    wcount -= (wcount + pos * 2) / 64; /* leave room for \n */
    wcount >>= 1;		/* 2 chars per input byte */
    count = (wcount < rcount ? (status = 1, wcount) : rcount);
    while (--count >= 0) {
        *++q = hex_digits[*++p >> 4];
        *++q = hex_digits[*p & 0xf];
        if (!(++pos & 31) && (count != 0 || !last))
            *++q = '\n';
    }
    if (last && status == 0 && ss->EndOfData)
        *++q = '>';
    pr->ptr = p;
    pw->ptr = q;
    ss->count = pos & 31;
    return status;
}

/* Stream template */
const stream_template s_AXE_template =
{&st_AXE_state, s_AXE_init, s_AXE_process, 1, 3
};

/* ------ ASCIIHexDecode ------ */

private_st_AXD_state();

/* Initialize the state */
static int
s_AXD_init(stream_state * st)
{
    stream_AXD_state *const ss = (stream_AXD_state *) st;

    return s_AXD_init_inline(ss);
}

/* Process a buffer */
static int
s_AXD_process(stream_state * st, stream_cursor_read * pr,
              stream_cursor_write * pw, bool last)
{
    stream_AXD_state *const ss = (stream_AXD_state *) st;
    int code = s_hex_process(pr, pw, &ss->odd, hex_ignore_whitespace);

    switch (code) {
        case 0:
            if (ss->odd >= 0 && last) {
                if (pw->ptr == pw->limit)
                    return 1;
                *++(pw->ptr) = ss->odd << 4;
                ss->odd = -1;
            }
            /* falls through */
        case 1:
            /* We still need to read ahead and check for EOD. */
            for (; pr->ptr < pr->limit; pr->ptr++)
                if (scan_char_decoder[pr->ptr[1]] != ctype_space) {
                    if (pr->ptr[1] == '>') {
                        pr->ptr++;
                        goto eod;
                    }
                    return 1;
                }
            return 0;		/* still need to scan ahead */
        default:
            return code;
        case ERRC:
            ;
    }
    /*
     * Check for EOD.  ERRC implies at least one more character
     * was read; we must unread it, since the caller might have
     * invoked the filter with exactly the right count to read all
     * the available data, and we might be reading past the end.
     */
    if (*pr->ptr != '>') {	/* EOD */
        --(pr->ptr);
        return ERRC;
    }
  eod:if (ss->odd >= 0) {
        if (pw->ptr == pw->limit)
            return 1;
        *++(pw->ptr) = ss->odd << 4;
    }
    return EOFC;
}

/* Stream template */
const stream_template s_AXD_template =
{&st_AXD_state, s_AXD_init, s_AXD_process, 2, 1
};

/* ------ PSStringEncode ------ */

/* Process a buffer */
static int
s_PSSE_process(stream_state * st, stream_cursor_read * pr,
               stream_cursor_write * pw, bool last)
{
    const byte *p = pr->ptr;
    const byte *rlimit = pr->limit;
    byte *q = pw->ptr;
    byte *wlimit = pw->limit;
    int status = 0;

    /* This doesn't have to be very efficient. */
    while (p < rlimit) {
        int c = *++p;

        if (c < 32 || c >= 127) {
            const char *pesc;
            const char *const esc = "\n\r\t\b\f";

            if (c < 32 && c != 0 && (pesc = strchr(esc, c)) != 0) {
                if (wlimit - q < 2) {
                    --p;
                    status = 1;
                    break;
                }
                *++q = '\\';
                *++q = "nrtbf"[pesc - esc];
                continue;
            }
            if (wlimit - q < 4) {
                --p;
                status = 1;
                break;
            }
            q[1] = '\\';
            q[2] = (c >> 6) + '0';
            q[3] = ((c >> 3) & 7) + '0';
            q[4] = (c & 7) + '0';
            q += 4;
            continue;
        } else if (c == '(' || c == ')' || c == '\\') {
            if (wlimit - q < 2) {
                --p;
                status = 1;
                break;
            }
            *++q = '\\';
        } else {
            if (q == wlimit) {
                --p;
                status = 1;
                break;
            }
        }
        *++q = c;
    }
    if (last && status == 0) {
        if (q == wlimit)
            status = 1;
        else
            *++q = ')';
    }
    pr->ptr = p;
    pw->ptr = q;
    return status;
}

/* Stream template */
const stream_template s_PSSE_template =
{&st_stream_state, NULL, s_PSSE_process, 1, 4
};

/* ------ PSStringDecode ------ */

private_st_PSSD_state();

/* Initialize the state */
int
s_PSSD_init(stream_state * st)
{
    stream_PSSD_state *const ss = (stream_PSSD_state *) st;

    ss->from_string = false;
    return s_PSSD_partially_init_inline(ss);
}

/* Process a buffer */
static int
s_PSSD_process(stream_state * st, stream_cursor_read * pr,
               stream_cursor_write * pw, bool last)
{
    stream_PSSD_state *const ss = (stream_PSSD_state *) st;
    const byte *p = pr->ptr;
    const byte *rlimit = pr->limit;
    byte *q = pw->ptr;
    byte *wlimit = pw->limit;
    int status = 0;
    int c;

#define check_p(n)\
  if ( p == rlimit ) { p -= n; goto out; }
#define check_q(n)\
  if ( q == wlimit ) { p -= n; status = 1; goto out; }
    while (p < rlimit) {
        c = *++p;
        if (c == '\\' && !ss->from_string) {
            check_p(1);
            switch ((c = *++p)) {
                case 'n':
                    c = '\n';
                    goto put;
                case 'r':
                    c = '\r';
                    goto put;
                case 't':
                    c = '\t';
                    goto put;
                case 'b':
                    c = '\b';
                    goto put;
                case 'f':
                    c = '\f';
                    goto put;
                default:	/* ignore the \ */
                  put:check_q(2);
                    *++q = c;
                    continue;
                case char_CR:	/* ignore, check for following \n */
                    check_p(2);
                    if (p[1] == char_EOL)
                        p++;
                    continue;
                case char_EOL:	/* ignore */
                    continue;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                    {
                        int d;

                        check_p(2);
                        d = p[1];
                        c -= '0';
                        if (d >= '0' && d <= '7') {
                            if (p + 1 == rlimit) {
                                p -= 2;
                                goto out;
                            }
                            check_q(2);
                            c = (c << 3) + d - '0';
                            d = p[2];
                            if (d >= '0' && d <= '7') {
                                c = (c << 3) + d - '0';
                                p += 2;
                            } else
                                p++;
                        } else
                            check_q(2);
                        *++q = c;
                        continue;
                    }
            }
        } else
            switch (c) {
                case '(':
                    check_q(1);
                    ss->depth++;
                    break;
                case ')':
                    if (ss->depth == 0) {
                        status = EOFC;
                        goto out;
                    }
                    check_q(1);
                    ss->depth--;
                    break;
                case char_CR:	/* convert to \n */
                    check_p(1);
                    check_q(1);
                    if (p[1] == char_EOL)
                        p++;
                    *++q = '\n';
                    continue;
                case char_EOL:
                    c = '\n';
                    /* fall through */
                default:
                    check_q(1);
                    break;
            }
        *++q = c;
    }
#undef check_p
#undef check_q
  out:pr->ptr = p;
    pw->ptr = q;
    if (last && status == 0 && p != rlimit)
        status = ERRC;
    return status;
}

/* Stream template */
const stream_template s_PSSD_template =
{&st_PSSD_state, s_PSSD_init, s_PSSD_process, 4, 1
};

/* ------ Utilities ------ */

/*
 * Convert hex data to binary.
 * Return 1 if we filled the string,
 *        0 if we ran out of input data before filling the string,
 *        2 if hex_break_on_whitespace is on and we encounrered
 *          a white space.
 *        ERRC on error.
 * The caller must set *odd_digit to -1 before the first call;
 * after each call, if an odd number of hex digits has been read (total),
 * *odd_digit is the odd digit value, otherwise *odd_digit = -1.
 * See strimpl.h for the definition of syntax.
 */
int
s_hex_process(stream_cursor_read * pr, stream_cursor_write * pw,
              int *odd_digit, hex_syntax syntax)
{
    const byte *p = pr->ptr;
    const byte *rlimit = pr->limit;
    byte *q = pw->ptr;
    byte *wlimit = pw->limit;
    byte *q0 = q;
    byte val1 = (byte) * odd_digit;
    byte val2;
    uint rcount;
    byte *flimit;
    const byte *const decoder = scan_char_decoder;
    int code = 0;

    if (q >= wlimit)
        return 1;
    if (val1 <= 0xf)
        goto d2;
    do {
        /* No digits read */
        if ((rcount = (rlimit - p) >> 1) != 0)
        {
            /* Set up a fast end-of-loop check, so we don't have to test */
            /* both p and q against their respective limits. */
            flimit = (rcount < wlimit - q ? q + rcount : wlimit);
            while (1) {
                if ((val1 = decoder[p[1]]) <= 0xf &&
                    (val2 = decoder[p[2]]) <= 0xf) {
                    p += 2;
                    *++q = (val1 << 4) + val2;
                    if (q < flimit)
                        continue;
                    if (q >= wlimit)
                        goto px;
                }
                break;
            }
        }
        /* About to read the first digit */
        while (1) {
            if (p >= rlimit)
                goto end1;
            if ((val1 = decoder[*++p]) > 0xf) {
                if (val1 == ctype_space) {
                    switch (syntax) {
                        case hex_ignore_garbage:
                        case hex_ignore_whitespace:
                            continue;
                        case hex_ignore_leading_whitespace:
                            if (q == q0 && *odd_digit < 0)
                                continue;
                            /* pass through */
                        case hex_break_on_whitespace:
                            --p;
                            code = 2;
                            goto end1;
                    }
                } else if (syntax == hex_ignore_garbage)
                    continue;
                code = ERRC;
                goto end1;
            }
            break;
        }
  d2:
        /* About to read the second hex digit of a pair */
        while (1) {
            if (p >= rlimit) {
                *odd_digit = val1;
                goto ended;
            }
            if ((val2 = decoder[*++p]) > 0xf) {
                if (val2 == ctype_space)
                    switch (syntax) {
                        case hex_ignore_garbage:
                        case hex_ignore_whitespace:
                            continue;
                        case hex_ignore_leading_whitespace:
                            if (q == q0)
                                continue;
                            /* pass through */
                        case hex_break_on_whitespace:
                            --p;
                            *odd_digit = val1;
                            code = 2;
                            goto ended;
                    }
                if (syntax == hex_ignore_garbage)
                    continue;
                *odd_digit = val1;
                code = ERRC;
                goto ended;
            }
            break;
        }
        *++q = (val1 << 4) + val2;
    } while (q < wlimit);
  px:code = 1;
  end1:*odd_digit = -1;
  ended:pr->ptr = p;
    pw->ptr = q;
    return code;
}
