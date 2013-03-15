
unsigned int getCurrentTime() {
	//lite_assert(false); // PAL currently doesn't know what time it is!
	volatile int i = 1;
	i--;
	return i;
}
