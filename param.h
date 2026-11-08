#define MAX_WORD 63
#define MAX_FILES 15
#define MAX_THREADS 15
#define MAX_INPUT 100
#define HASH_MAP_SIZE 0x100537 // 1048583 je zapravo taj broj sa leve, a ovaj broj je takodje prost
// izgleda da mi ovo ne treba jer bi se mucio sa adjust-ovanjem mapiranja glavne hashmap-e i stop hm #define STOP_SIZE 101 // neka bude prost zbog boljeg izbegavanja kolizije
#define MAX_FILE_NAME 255
#define true 1
#define false 0
#define prime 29
#define INVALID_THREAD_ID ((pthread_t)0xDEADBEEF)

struct h_m_entry
{
	char word[MAX_WORD + 1];
	pthread_mutex_t entry_mutex;
	long frequency;
};

struct scanned_file
{
	char file_name[MAX_FILE_NAME + 1];
	time_t mod_time;
};

struct cleanup_args
{
	char *path;
	char *buffer;
	FILE *fp;
};

pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t s_f_mutex = PTHREAD_MUTEX_INITIALIZER;
