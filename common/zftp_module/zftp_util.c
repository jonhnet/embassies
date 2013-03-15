#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "debug.h"
#include "LiteLib.h"
#include "zftp_network_state.h"
#include "zftp_util.h"
#include "xax_network_utils.h"


void
zftp_str2hash(const char *string, hash_t *hash)
{
	int i, j;
	lite_memset(hash, 0, sizeof(hash_t));

	for(i = 0, j = 0; i < 2 * sizeof(hash_t); i++)
	{
		uint8_t *b = &((uint8_t *)hash)[j];
		char c = string[i];

		if(c >= '0' && c <= '9')
			*b = (*b << 4) + c - '0';
		else if(c >= 'A' && c <= 'F')
			*b = (*b << 4) + 10 + (c - 'A');
		else if(c >= 'a' && c <= 'f')
			*b = (*b << 4) + 10 + (c - 'a');
		else
		{
			lite_memset(hash, 0, sizeof(hash_t));
			return;
		}

		if(i % 2 != 0)
			j++;
	}
}

bool _is_all_zeros(uint8_t *in_data, uint32_t valid_data_len)
{
	uint32_t i;
	for (i=0; i<valid_data_len; i++)
	{
		if (in_data[i]!=0)
		{
			return false;
		}
	}
	return true;
}

