#include "zftp_protocol.h"
#include "ClientCompressionContext.h"

ClientCompressionContext::ClientCompressionContext()
	: seq(0)
{
}

uint32_t ClientCompressionContext::get_state()
{
	return 7;
}

uint32_t ClientCompressionContext::get_seq()
{
	uint32_t result = seq++;
	return result;
}
