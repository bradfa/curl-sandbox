#define MALLOC_CHECK_ 2

#include <pthread.h>
#include <curl/curl.h>
#include <sys/queue.h>
#include <stdio.h>
#include <unistd.h>

#define CURL_SIMUL	128
#define LIST_SIZE	1024

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
LIST_HEAD(listhead, entry) head;
int list_len;

struct entry {
	char *data;
	char *hostname;
	LIST_ENTRY(entry) entries;
};

void *curl_thread(void *aa)
{
	int count, ret, still_left;
	CURLM *multi_handle;
	CURL *easy_handle;
	CURLMsg *msg;
	struct entry *entry;

	multi_handle = curl_multi_init();
	for (;;) {
		count = ret = 0;
		pthread_mutex_lock(&mutex);
		while (!head.lh_first)
			pthread_cond_wait(&cond, &mutex);
		while (head.lh_first && (count < CURL_SIMUL)) {
			entry = head.lh_first;
			LIST_REMOVE(entry, entries);
			easy_handle = curl_easy_init();
			curl_easy_setopt(easy_handle, CURLOPT_VERBOSE, 0);
			curl_easy_setopt(easy_handle, CURLOPT_NOSIGNAL, 1);
			curl_easy_setopt(easy_handle, CURLOPT_FAILONERROR, 1);
			curl_easy_setopt(easy_handle, CURLOPT_NOPROGRESS, 1);
			curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT, 1);
			curl_easy_setopt(easy_handle, CURLOPT_POST, 1);
			curl_easy_setopt(easy_handle, CURLOPT_COPYPOSTFIELDS,
					 entry->data);
			curl_easy_setopt(easy_handle, CURLOPT_URL,
					 entry->hostname);
			ret = curl_multi_add_handle(multi_handle, easy_handle);
		}
		pthread_mutex_unlock(&mutex);
		/* TODO: Send the curls! */
		msg = curl_multi_info_read(multi_handle, &still_left);
		while (msg) {
			curl_multi_remove_handle(multi_handle,
						 msg->easy_handle);
			curl_easy_cleanup(msg->easy_handle);
			msg = curl_multi_info_read(multi_handle, &still_left);
		}
	}
	curl_multi_cleanup(multi_handle);
}

int main(void)
{
	int ret;
	struct entry *entry;
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
