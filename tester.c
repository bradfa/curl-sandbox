#define MALLOC_CHECK_ 2

#include <pthread.h>
#include <curl/curl.h>
#include <sys/queue.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
LIST_HEAD(listhead, entry) head;
int list_len;

struct entry {
	char *data;
	LIST_ENTRY(entry) entries;
};

void *curl_thread(void *aa)
{
	int count;
	for (;;) {
		count = 0;
		pthread_mutex_lock(&mutex);
		while (!head.lh_first)
			pthread_cond_wait(&cond, &mutex);
		while (head.lh_first && (count < 128)) {
			/* TODO: Build some curls */
		}
	}
}

int main(void)
{
	int ret;
	struct entry *a;
	pthread_t thread;
	list_len = 0;
	ret = pthread_create(&thread, NULL, curl_thread, NULL);
	if (ret)
		return -1;
	for (;;) {
		sleep(1);
	}
	return 0;
}
