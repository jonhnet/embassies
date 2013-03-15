#include <iostream>
#include <fstream>
#include <assert.h>
#include <stdlib.h>

#include "types.h"
#include "tests.h"
#include "ZApp.h"
#include "RandomSupply.h"

using namespace std;

// Give it an argument (any argument) to run speed tests
// Otherwise, it runs regression tests

int main(int argc, char **argv) {
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

  runTests(random_supply, argc > 1);

  cout << "Done!\n";
  char foo;
  cin >> foo;

  return 0;
}

