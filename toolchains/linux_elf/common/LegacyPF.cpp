#include "LegacyPF.h"

typedef struct {
	LegacyMethods *methods;
	PFHandler *handler;
} LegacyPF_Impl;

typedef MQEntry LegacyMessage_Impl;

ZAS *legacy_pf_get_zas(LegacyPF *lpf)
{
	LegacyPF_Impl *impl = (LegacyPF_Impl *) lpf;
	return &impl->handler->mq.zas;
}

LegacyMessage *legacy_pf_pop(LegacyPF *lpf)
{
	LegacyPF_Impl *impl = (LegacyPF_Impl *) lpf;
	MQEntry *mqe = mq_pop(&impl->handler->mq, 0);
	LegacyMessage_Impl *lmimpl = mqe;
	return (LegacyMessage*) lmimpl;
}

uint8_t *legacy_message_get_data(LegacyMessage *lm)
{
	LegacyMessage_Impl *impl = (LegacyMessage_Impl *) lm;
	return impl->zcb->data();
}

uint32_t legacy_message_get_length(LegacyMessage *lm)
{
	LegacyMessage_Impl *impl = (LegacyMessage_Impl *) lm;
	return impl->info.packet_length;
}

void legacy_message_release(LegacyMessage *lm)
{
	LegacyMessage_Impl *impl = (LegacyMessage_Impl *) lm;
	delete impl->zcb;
	cheesy_free(impl);
}

static LegacyMethods s_methods = {
	legacy_pf_get_zas,
	legacy_pf_pop,
	legacy_message_get_data,
	legacy_message_get_length,
	legacy_message_release
};

LegacyPF *legacy_pf_init(PFHandler *pfh)
{
	LegacyPF_Impl *impl = new LegacyPF_Impl;
	impl->methods = &s_methods;
	impl->handler = pfh;
	return (LegacyPF *) impl;
}
