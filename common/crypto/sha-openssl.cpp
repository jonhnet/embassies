/* ====================================================================
 * Copyright (c) 2011 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.OpenSSL.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    licensing@OpenSSL.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.OpenSSL.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sha-openssl.h"


unsigned int OPENSSL_ia32cap_P[2];

extern "C"{
	void OPENSSL_cpuid_setup()
	{
		unsigned long long OPENSSL_ia32_cpuid();
		unsigned long long vec = OPENSSL_ia32_cpuid();

		OPENSSL_ia32cap_P[0] = vec;
		OPENSSL_ia32cap_P[1] = vec>>32;
	}
}


#define SHA_MAKE_STRING(c,s)   do {  \
  unsigned long ll;   \
  ll=(c)->h0; (void)HOST_l2c(ll,(s)); \
  ll=(c)->h1; (void)HOST_l2c(ll,(s)); \
  ll=(c)->h2; (void)HOST_l2c(ll,(s)); \
  ll=(c)->h3; (void)HOST_l2c(ll,(s)); \
  ll=(c)->h4; (void)HOST_l2c(ll,(s)); \
  } while (0)
# define SHA_BLOCK_DATA_ORDER    sha1_block_data_order

#   define HOST_l2c(l,c)  ({ unsigned int r=(l);      \
           asm ("bswapl %0":"=r"(r):"0"(r));  \
           *((unsigned int *)(c))=r; (c)+=4; r; })


#define INIT_DATA_h0 0x67452301UL
#define INIT_DATA_h1 0xefcdab89UL
#define INIT_DATA_h2 0x98badcfeUL
#define INIT_DATA_h3 0x10325476UL
#define INIT_DATA_h4 0xc3d2e1f0UL


/*
 * Digests section: SHA1
 */

extern "C"{
	void sha1_block_data_order (void *c,const void *p,size_t len);
}

int sha1_init(SHA_CTX *c)
  {
  memset (c,0,sizeof(*c));
  c->h0=INIT_DATA_h0;
  c->h1=INIT_DATA_h1;
  c->h2=INIT_DATA_h2;
  c->h3=INIT_DATA_h3;
  c->h4=INIT_DATA_h4;
  return 1;
  }


int SHA1_Update(SHA_CTX *c, const void *data_, size_t len)
  {
  const unsigned char *data=(const unsigned char *)data_;
  unsigned char *p;
  SHA_LONG l;
  size_t n;

  if (len==0) return 1;

  l=(c->Nl+(((SHA_LONG)len)<<3))&0xffffffffUL;
  /* 95-05-24 eay Fixed a bug with the overflow handling, thanks to
   * Wei Dai <weidai@eskimo.com> for pointing it out. */
  if (l < c->Nl) /* overflow */
    c->Nh++;
  c->Nh+=(SHA_LONG)(len>>29);  /* might cause compiler warning on 16-bit */
  c->Nl=l;

  n = c->num;
  if (n != 0)
    {
    p=(unsigned char *)c->data;

    if (len >= SHA_CBLOCK || len+n >= SHA_CBLOCK)
      {
      memcpy (p+n,data,SHA_CBLOCK-n);
      SHA_BLOCK_DATA_ORDER (c,p,1);
      n      = SHA_CBLOCK-n;
      data  += n;
      len   -= n;
      c->num = 0;
      memset (p,0,SHA_CBLOCK); /* keep it zeroed */
      }
    else
      {
      memcpy (p+n,data,len);
      c->num += (unsigned int)len;
      return 1;
      }
    }

  n = len/SHA_CBLOCK;
  if (n > 0)
    {
    SHA_BLOCK_DATA_ORDER (c,data,n);
    n    *= SHA_CBLOCK;
    data += n;
    len  -= n;
    }

  if (len != 0)
    {
    p = (unsigned char *)c->data;
    c->num = (unsigned int)len;
    memcpy (p,data,len);
    }
  return 1;
  }


int sha1_update(SHA_CTX *c,const void *data,size_t len)
{ 
  const unsigned char *ptr = (const unsigned char *)data;
  size_t res;

  if ((res = c->num)) {
    res = SHA_CBLOCK-res;
    if (len<res) res=len;
    SHA1_Update (c,ptr,res);
    ptr += res;
    len -= res;
  }

  res = len % SHA_CBLOCK;
  len -= res;

  if (len) {
    sha1_block_data_order(c,ptr,len/SHA_CBLOCK);

    ptr += len;
    c->Nh += len>>29;
    c->Nl += len<<=3;
    if (c->Nl<(unsigned int)len) c->Nh++;
  }

  if (res)
    SHA1_Update(c,ptr,res);

  return 1;
}

int SHA1_Final(unsigned char *md, SHA_CTX *c)
  {
  unsigned char *p = (unsigned char *)c->data;
  size_t n = c->num;

  p[n] = 0x80; /* there is always room for one */
  n++;

  if (n > (SHA_CBLOCK-8))
    {
    memset (p+n,0,SHA_CBLOCK-n);
    n=0;
    SHA_BLOCK_DATA_ORDER (c,p,1);
    }
  memset (p+n,0,SHA_CBLOCK-8-n);

  p += SHA_CBLOCK-8;
#if   defined(DATA_ORDER_IS_BIG_ENDIAN)
  (void)HOST_l2c(c->Nh,p);
  (void)HOST_l2c(c->Nl,p);
#elif defined(DATA_ORDER_IS_LITTLE_ENDIAN)
  (void)HOST_l2c(c->Nl,p);
  (void)HOST_l2c(c->Nh,p);
#endif
  p -= SHA_CBLOCK;
  SHA_BLOCK_DATA_ORDER (c,p,1);
  c->num=0;
  memset (p,0,SHA_CBLOCK);

  SHA_MAKE_STRING(c,md);

  return 1;
  }



#define l2c(l,c)  ({ unsigned int r=(l);        \
         asm ("bswapl %0" : "=r"(r) : "0"(r));  \
         *((unsigned int *)(c))=r;      })

int sha1_final(SHA_CTX *c,unsigned char *md)
{ 
  unsigned char final[2*SHA_CBLOCK],*len;
  size_t num = c->num;
  unsigned int *h;

  memset(final,0,sizeof(final));
  if (num) memcpy(final,c->data,num);

  final[num++]=0x80;
  if ((SHA_CBLOCK-num)>=8) {
    len=final+SHA_CBLOCK-8;
    num=1;
  } else {
    len=final+2*SHA_CBLOCK-8;
    num=2;
  }
  l2c(c->Nh,len); l2c(c->Nl,len+4);

  sha1_block_data_order(c,final,num);

  h = (unsigned int *)c;
  l2c(h[0],md), l2c(h[1],md+4), l2c(h[2],md+8), l2c(h[3],md+12), l2c(h[4],md+16);

  return 1;
}

