#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
int global_val = 0;
pthread_mutex_t thread_mutex;
void *thread1(void *arg){
        while(1){
                pthread_mutex_lock(&thread_mutex);
		global_val = global_val + 1;
                printf("thread1 global_val=%d\n", global_val);
                global_val = global_val + 1;
                usleep(100);
                printf("thread1 global_val=%d\n", global_val);
                usleep(100);
		pthread_mutex_unlock(&thread_mutex);
        }
        return NULL;
}
void *thread2(void *arg){
        while(1){
		pthread_mutex_lock(&thread_mutex);
                global_val = global_val + 1;
                printf("thread2 global_val=%d\n", global_val);
                usleep(100);
                global_val = global_val + 1;
                printf("thread2 global_val=%d\n", global_val);
                usleep(100);
		pthread_mutex_unlock(&thread_mutex);
        }
        return NULL;
}
void *thread3(void *arg){
        while(1){
		pthread_mutex_lock(&thread_mutex);
                global_val = global_val + 1;
                printf("thread3 global_val=%d\n", global_val);
                usleep(100);
                global_val = global_val + 1;
                printf("thread3 global_val=%d\n", global_val);
                usleep(100);
		pthread_mutex_unlock(&thread_mutex);
        }
        return NULL;
}


int main(void){
pthread_t thread_id1 = 0, thread_id2 = 0, thread_id3 = 0;
pthread_mutex_init(&thread_mutex, NULL);
pthread_create(&thread_id1, NULL, thread1, NULL);
pthread_create(&thread_id2, NULL, thread2, NULL);
pthread_create(&thread_id3, NULL, thread3, NULL);
pthread_join(thread_id1, NULL);
pthread_join(thread_id2, NULL);
pthread_join(thread_id3, NULL);

return 0;
}
