#define SHA_LONG unsigned long
#define SHA_LBLOCK  16
#define SHA_CBLOCK  (SHA_LBLOCK*4)  /* SHA treats input data as a
           * contiguous array of 32 bit
           * wide big-endian values. */


typedef struct SHAstate_st {
  SHA_LONG h0,h1,h2,h3,h4;
  SHA_LONG Nl,Nh;
  SHA_LONG data[SHA_LBLOCK];
  unsigned int num;
} SHA_CTX;

int sha1_init(SHA_CTX *c);

int sha1_update(SHA_CTX *c,const void *data,size_t len);

int sha1_final(SHA_CTX *c,unsigned char *md);
