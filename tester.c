#define MALLOC_CHECK_ 2

#include <pthread.h>
#include <curl/curl.h>
#include <sys/queue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define CURL_SIMUL	32
#define LIST_MAX	100
#define LIST_DELAY	2 /* ms */

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
LIST_HEAD(listhead, entry) head;
int list_len;
const char hostname[] = "http://127.0.0.1/";
const char raw_data[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
		  "<top><middle>something</middle></top>";

struct entry {
	char *data;
	LIST_ENTRY(entry) entries;
};

void *curl_thread(void *aa)
{
	int count, ret, still_left;
	CURLM *multi_handle;
	CURL *easy_handle;
	CURLMsg *msg;
	struct entry *entry;
	struct curl_slist *slist = NULL;

	slist = curl_slist_append(slist, "Content-Type:text/xml");
	multi_handle = curl_multi_init();
	if (!multi_handle) /* Bad bad bad */
		pthread_exit(NULL);
	for (;;) {
		count = ret = 0;
		pthread_mutex_lock(&mutex);
		while (!head.lh_first)
			pthread_cond_wait(&cond, &mutex);
		printf("Thread found %d entries waiting in list\n", list_len);
		while (head.lh_first && (count < CURL_SIMUL)) {
			entry = head.lh_first;
			LIST_REMOVE(entry, entries);
			list_len--;
			count++;
			easy_handle = curl_easy_init();
			curl_easy_setopt(easy_handle, CURLOPT_VERBOSE, 0);
			curl_easy_setopt(easy_handle, CURLOPT_NOSIGNAL, 1);
			curl_easy_setopt(easy_handle, CURLOPT_FAILONERROR, 1);
			curl_easy_setopt(easy_handle, CURLOPT_NOPROGRESS, 1);
			curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT_MS,
					 LIST_DELAY*CURL_SIMUL-10);
			curl_easy_setopt(easy_handle, CURLOPT_POST, 1);
			curl_easy_setopt(easy_handle, CURLOPT_COPYPOSTFIELDS,
					 entry->data);
			free(entry->data);
			free(entry);
			curl_easy_setopt(easy_handle, CURLOPT_URL, hostname);
			curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER,
					 slist);
			ret = curl_multi_add_handle(multi_handle, easy_handle);
		}
		pthread_mutex_unlock(&mutex);
		pthread_cond_signal(&cond);
		do { /* Call multi_perform again as needed to complete */
			struct timeval timeout;
			fd_set fdread, fdwrite, fdexcep;
			int maxfd = -1;
			long curl_timeo = -1;
			FD_ZERO(&fdread);
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdexcep);
			ret = curl_multi_timeout(multi_handle, &curl_timeo);
			if (ret)
				goto curl_error;
			if (curl_timeo >= 0) {
				timeout.tv_sec = curl_timeo / 1000;
				timeout.tv_usec = (curl_timeo % 1000) * 1000;
			} else {
				timeout.tv_sec = 0;
				timeout.tv_usec = 0;
			}
			ret = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);
			if (ret)
				goto curl_error;
			ret = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
			switch (ret) {
			case -1: /* error */
				break;
			case 0: /* timeout */
			default:
				ret = curl_multi_perform(multi_handle, &still_left);
				if (ret == CURLM_CALL_MULTI_PERFORM)
					curl_multi_perform(multi_handle, &still_left);
				else if (ret != CURLM_OK)
					goto curl_error;
				break;
			}
		} while (still_left);
curl_error:
		printf("Thread completed CURL operations\n");
		msg = curl_multi_info_read(multi_handle, &still_left);
		while (msg) {
			curl_multi_remove_handle(multi_handle,
						 msg->easy_handle);
			curl_easy_cleanup(msg->easy_handle);
			msg = curl_multi_info_read(multi_handle, &still_left);
		}
	}
	curl_multi_cleanup(multi_handle);
	curl_slist_free_all(slist);
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
		pthread_mutex_lock(&mutex);
		while (list_len >= LIST_MAX)
			pthread_cond_wait(&cond, &mutex);
		printf("Main found only %d entries in the list\n", list_len);
		while (list_len < LIST_MAX) {
			pthread_mutex_unlock(&mutex);
			entry = malloc(sizeof(struct entry));
			if (!entry)
				return -ENOMEM;
			memset(entry, '\0', sizeof(struct entry));
			entry->data = malloc(strlen(raw_data)+1);
			if (!entry->data)
				return -ENOMEM;
			memset(entry->data, '\0', strlen(raw_data)+1);
			memcpy(entry->data, raw_data, strlen(raw_data)+1);
			pthread_mutex_lock(&mutex);
			LIST_INSERT_HEAD(&head, entry, entries);
			list_len++;
			pthread_mutex_unlock(&mutex);
			usleep(LIST_DELAY * 1000);
		}
		pthread_cond_signal(&cond);
	}
	return 0;
}
