#include "VerifyLabelTest.h"
#include "paltest.cert.h"

void verifylabeltest(ZoogDispatchTable_v1 *zdt)
{
	ZCert *cert = new ZCert((uint8_t*) vendor_a_cert, sizeof(vendor_a_cert));
	ZCertChain *zcc = new ZCertChain();
	zcc->addCert(cert);
	(zdt->zoog_verify_label)(zcc);
}
