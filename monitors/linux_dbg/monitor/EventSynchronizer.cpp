#include <string.h>
#include "EventSynchronizer.h"

EventSynchronizer::EventSynchronizer()
{
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
}

EventSynchronizer::~EventSynchronizer()
{
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}

void EventSynchronizer::update_event(AlarmEnum alarm_num, uint32_t sequence_number)
{
	pthread_mutex_lock(&mutex);
	sequence_numbers[alarm_num] = sequence_number;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

void EventSynchronizer::wait(uint32_t *known_sequence_numbers, uint32_t *out_sequence_numbers)
{
	pthread_mutex_lock(&mutex);
	while (memcmp(known_sequence_numbers, sequence_numbers,
		sizeof(sequence_numbers))==0)
	{
		pthread_cond_wait(&cond, &mutex);
	}
	memcpy(out_sequence_numbers, sequence_numbers,
		sizeof(sequence_numbers));
	pthread_mutex_unlock(&mutex);
}
