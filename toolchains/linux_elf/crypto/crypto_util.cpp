#include <iostream>
#include <fstream>
#include <assert.h>
#include <stdlib.h>

#include "types.h" 
#include "tests.h"
#include "ZApp.h"
#include "RandomSupply.h"

#include "ambient_malloc.h"
#include "standard_malloc_factory.h"

using namespace std;

int main(int argc, char **argv) {
  ambient_malloc_init(standard_malloc_factory_init());

  // Initialize some strong randomness
  ifstream urand;
  urand.open("/dev/urandom", ios::binary);
  assert(!urand.fail());
  uint8_t buffer[RandomSupply::SEED_SIZE];
  urand.read((char*)buffer, sizeof(buffer));
  urand.close();
	RandomSupply* random_supply = new RandomSupply(buffer);
 
	// Initialize some not-so-strong randomness 
	srand(1);

  ZApp app;

  app.run(random_supply, argc, argv);

//  cout << "Done!\n";

  return 0;
}


// TODO: Replace with a real source of randomness

extern "C" {
void randombytes(unsigned char *buffer,unsigned long long numBytes) {
  for (uint64_t i = 0; i < numBytes; i++) {
    buffer[i] = rand() % 256;
  }
}
}

