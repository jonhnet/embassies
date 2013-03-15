#include <time.h>
#include "pal_abi/pal_types.h"

uint32_t getCurrentTime() {
  return (uint32_t) time (NULL);
}

