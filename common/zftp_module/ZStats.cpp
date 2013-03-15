#include "ZStats.h"
#include "zemit.h"

ZStats::ZStats(ZLCEmit *ze)
	: ze(ze)
{
	zero();
}

void ZStats::zero()
{
	received_merkle_records = 0;
	received_data_bytes = 0;
}

void ZStats::install_reply(ZFileReplyFromServer *reply)
{
	received_merkle_records += reply->get_num_merkle_records();
	received_data_bytes += (reply->get_data_end() - reply->get_data_start());
	remark();
}

void ZStats::remark()
{
	ZLC_CHATTY(ze,
		"%d hashes (%dMB); %d data bytes (%dMB)\n",,
		received_merkle_records,
		(received_merkle_records*sizeof(ZFTPMerkleRecord))>>20,
		received_data_bytes,
		received_data_bytes>>20);
}

void ZStats::reset()
{
	remark();
	ZLC_CHATTY(ze, "...Reset.\n");
	zero();
}
