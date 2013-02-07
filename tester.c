#define MALLOC_CHECK_ 2

#include <pthread.h>
#include <curl/curl.h>
#include <sys/queue.h>
#include <stdio.h>
#include <unistd.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
LIST_HEAD(listhead, entry) head;

struct entry {
	char *data;
	LIST_ENTRY(entry) entries;
};

void *curl_thread(void *aa)
{
	for (;;) {
		sleep(1);
	}
}

int main(void)
{
	int ret;
	pthread_t thread;
	ret = pthread_create(&thread, NULL, curl_thread, NULL);
	if (ret)
		return -1;
	for (;;) {
		sleep(1);
	}
	return 0;
}
