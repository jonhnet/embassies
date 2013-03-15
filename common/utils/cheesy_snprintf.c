#include <stdarg.h>
#include "xax_util.h"
#include "LiteLib.h"
#include "math_util.h"

#include "cheesy_snprintf.h"

//////////////////////////////////////////////////////////////////////////////

char *hexstr(char *buf, int val)
{
	return hexstr_min(buf, val, 1);
}

char *hexstr_08x(char *buf, int val)
{
	return hexstr_min(buf, val, 8);
}

char *hexstr_min(char *buf, int val, int min_digits)
{
	int ni;
	int oi=0;
	bool begun = false;
	for (ni=0; ni<8; ni++)
	{
		int digit_index = 7-ni;
		int nybble_val = (val >> ((digit_index)*4)) & 0x0f;
		if (nybble_val>0 || (digit_index < min_digits))
		{
			begun = true;
		}
		if (begun)
		{
			buf[oi] = NYBBLE_TO_HEX(nybble_val);
			oi++;
		}
	}
	buf[oi] = '\0';
	return buf;
}

void utoa(char *out, uint32_t in_val)
{
	if (in_val==0)
	{
		out[0] = '0';
		out[1] = '\0';
	}
	else
	{
		int digits = 0;
		uint32_t val;
		for (val=in_val; val>0; val/=10)
		{
			digits += 1;
		}

		int total_digits = digits;
		digits = 0;
		for (val=in_val; val>0; val/=10)
		{
			out[total_digits - digits - 1] = '0'+(val%10);
			digits += 1;
		}
		out[total_digits] = '\0';
	}
}

void dtoa(char *out, double val_param, int num_decimals)
{
	int outi = 0;
	double val = val_param;
	if (val<0)
	{
		out[outi++] = '-';
		val = -val;
	}
	uint32_t wholepart = (uint32_t) val;
	utoa(&out[outi], wholepart);
	outi += lite_strlen(&out[outi]);
	val -= ((double) wholepart);

	double epsilon = 1e-8;
	int decimals = 0;
	out[outi++] = '.';

	while (num_decimals>=0
		? (decimals < num_decimals)
		: (val > epsilon))
	{
		val *= 10;
		uint32_t intpart = (uint32_t) val;
		val -= intpart;
		out[outi++] = '0'+intpart;
		epsilon *= 10.0;
		decimals +=1;
	}

	out[outi] = '\0';
}

char *hexbyte(char *buf, uint8_t val)
{
	buf[0] = NYBBLE_TO_HEX((val>>4) & 0x0f);
	buf[1] = NYBBLE_TO_HEX((val>>0) & 0x0f);
	buf[2] = '\0';
	return buf;
}

char *hexlong(char *buf, uint32_t val)
{
	int nybble;
	for (nybble=0; nybble<8; nybble++)
	{
		buf[nybble] = NYBBLE_TO_HEX((val >> ((7-nybble)*4))&0x0f);
	}
	buf[8] = '\0';
	return buf;
}

extern uint32_t str2hex(const char *buf)
{
	uint32_t val = 0;
	const char *p;
	for (p=buf; p[0]!=0; p++)
	{
		if (p[0]>='0' && p[0]<='9')
		{
			val=(val<<4) | (p[0]-'0');
		}
		else if (p[0]>='a' && p[0]<='f')
		{
			val=(val<<4) | (p[0]-'a'+10);
		}
		else if (p[0]>='A' && p[0]<='F')
		{
			val=(val<<4) | (p[0]-'A'+10);
		}
		else
		{
			break;
		}
	}
	return val;
}

extern int str2dec(const char *buf)
{
	const char *p = buf;
	bool neg = false;
	if (p[0]=='-')
	{
		neg = true;
		p++;
	}

	uint32_t val = 0;
	for (; p[0]!=0; p++)
	{
		if (p[0]>='0' && p[0]<='9')
		{
			val=(val*10) | (p[0]-'0');
		}
		else
		{
			break;
		}
	}
	if (neg)
	{
		val *= -1;
	}
	return val;
}

//////////////////////////////////////////////////////////////////////////////

typedef struct {
	const char *ip;
	char format_code;
	int format_len;
	int format_n1;
	int format_n2;
	char *op;
	char *eop;
} CheesySprintfState;

static bool _cs_done(CheesySprintfState *state)
{
	return (state->ip[0]=='\0')
		|| (state->op >= state->eop);
}

static void _cs_out(CheesySprintfState *state, char c)
{
	if (state->op < state->eop)
	{
		state->op[0] = c;
		state->op++;
	}
}

static void _cs_out_str(CheesySprintfState *state, char *s)
{
	int min_space = state->format_n1;
	int text_len = lite_strlen(s);
	if (min_space > 0 && text_len < min_space)
	{
		int i;
		for (i=0; i<min_space-text_len; i++)
		{
			_cs_out(state, ' ');
		}
	}
	int j;
	for (j=0; s[j]!='\0'; j++)
	{
		_cs_out(state, s[j]);
	}
}

int _decode_num(const char *begin, const char *end)
{
	if (begin==NULL)
	{
		return -1;
	}
	lite_assert(end!=NULL);
	lite_assert(end-begin<20);
	char buf[20];
	lite_memcpy(buf, begin, end-begin);
	buf[end-begin]='\0';
	return str2dec(buf);
}

void _cs_parse_format_code(CheesySprintfState *state)
{
	lite_assert(state->ip[0] == '%');
	state->format_n1 = -1;
	state->format_n2 = -1;
	state->format_len = -1;
	state->format_code = -1;

	const char *n1_end = NULL;
	const char *n2 = NULL;
	const char *n2_end = NULL;
	const char *x;
	for (x=state->ip+1; x!='\0'; x++)
	{
		if ((*x)>='0' && (*x)<='9')
		{
			// decoding one of the numbers
			continue;
		}
		else if ((*x)=='.')
		{
			// change numbers
			lite_assert(n2==NULL);
			n2 = x+1;
			n1_end = x;
		}
		else
		{
			// both are done
			if (n2!=NULL)
			{
				n2_end = x;
			}
			else
			{
				n1_end = x;
			}
			state->format_code = *x;
			state->format_len = ((*x=='\0') ? x : x+1) -state->ip;
			break;
		}
	}

	state->format_n1 = _decode_num(state->ip+1, n1_end);
	state->format_n2 = _decode_num(n2, n2_end);
}

int cheesy_snprintf(char *out, int n, const char *fmt, ...)
{
	if (n<=0) { return 0; }

	va_list argp;

	CheesySprintfState state;
	state.op = out;
	state.eop = out+n;

#define OUT(c)	_cs_out(&state, (c))
#define OUTS(s)	_cs_out_str(&state, (s))

	va_start(argp, fmt);
	state.ip=fmt;
	while (!_cs_done(&state))
	{
		if (state.ip[0] != '%')
		{
			OUT(state.ip[0]);
			state.ip++;
			continue;
		}
		else
		{
			_cs_parse_format_code(&state);

			switch (state.format_code)
			{
			case 'x':
			{
				int val = va_arg(argp, int);
				char hexbuf[10];
				hexstr_min(hexbuf, val, max(1, state.format_n1));
				OUTS(hexbuf);
				state.ip+=state.format_len;
				break;
			}
			case 'd':	// sorry, things are unsigned around here. :v)
			case 'u':
			{
				int val = va_arg(argp, int);
				char hexbuf[10];
				utoa(hexbuf, val);
				OUTS(hexbuf);
				state.ip+=state.format_len;
				break;
			}
			case 'L':	// jonh made up symbol for pointer-to-uint64_t
			{
				uint64_t *val = va_arg(argp, uint64_t*);
				char hexbuf[10];
				hexstr_min(hexbuf, (*val)>>32, 1);
				OUTS(hexbuf);
				hexstr_min(hexbuf, (*val)&0xffffffffLL, 8);
				OUTS(hexbuf);
				state.ip+=state.format_len;
				break;
			}
			case 'f':
			{
				double val = va_arg(argp, double);
				char dbuf[32];
				dtoa(dbuf, val, state.format_n2);
				OUTS(dbuf);
				state.ip+=state.format_len;
				break;
			}
			case 's':
			{
				typedef char *charp;
				char *str = va_arg(argp, charp);
				OUTS(str);
				state.ip+=state.format_len;
				break;
			}
			case '%':
			{
				OUT('%');	// just one percent, please, and go parse again.
				state.ip+=state.format_len;
				break;
			}
			default:
				// no understood format. Treat as literal.
				OUT('%');
				state.ip++;
				break;
			}
		}
	}
	int num_written = state.op - out;	// don't count NUL
	OUT('\0');

	out[n-1] = '\0';
	return num_written;
}

