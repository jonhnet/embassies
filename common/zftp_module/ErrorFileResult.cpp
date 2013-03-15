#include "LiteLib.h"
#include "zftp_protocol.h"
#include "ErrorFileResult.h"
#include "zlc_util.h"
#include "ZCachedFile.h"

ErrorFileResult::ErrorFileResult(ZCachedFile *zcf, ZFTPReplyCode code, const char *msg)
	: InternalFileResult(zcf)
{
	ZLC_COMPLAIN(zcf->ze, "_error_reply(0x%02x) \"%s\"\n",, code, msg);
	this->code = code;
	this->msg = msg;
}

uint32_t ErrorFileResult::get_reply_size(ZFileRequest *req)
{
	int str0len = lite_strlen(msg)+1;
	uint32_t reply_size = sizeof(ZFTPErrorPacket)+str0len;
	return reply_size;
}

void ErrorFileResult::fill_reply(Buf* vbuf, ZFileRequest *req)
{
	int str0len = lite_strlen(msg)+1;
	ZFTPErrorPacket *zdp = (ZFTPErrorPacket *) vbuf->data();
	zdp->header.code = z_htong(code, sizeof(zdp->header.code));
	zdp->message_len = z_htong(str0len, sizeof(zdp->message_len));
	zdp->file_hash = zcf->file_hash;
	lite_memcpy((char*) (&zdp[1]), msg, str0len);
}

bool ErrorFileResult::is_error()
{
	return true;
}
