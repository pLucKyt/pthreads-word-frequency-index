#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "param.h"

int cancel_point = false;

struct h_m_entry h_m_words[HASH_MAP_SIZE];
struct scanned_file s_f[MAX_FILES];
char stop_words[HASH_MAP_SIZE][MAX_WORD + 1];
pthread_t threads[MAX_THREADS];

long pow_of_prime[MAX_WORD];

void calculate_pp()
{
	for(int i = 0; i < MAX_WORD; i++)	
		pow_of_prime[i] = 1;
	
	for(int i = 1; i < MAX_WORD; i++)	
		pow_of_prime[i] = pow_of_prime[i - 1] * prime % HASH_MAP_SIZE;
	
	return;
}

void hash_function(char *buffer, long *some_hash)
{
	int len = strlen(buffer);
	for(int i = len - 1; i >= 0; i--)	
		*some_hash = (*some_hash + (buffer[i] - 'a' + 1) * pow_of_prime[len - i - 1]) % HASH_MAP_SIZE;
	
	return;
}

void h_m_init()
{
	for(int i = 0; i < HASH_MAP_SIZE; i++)
	{
		memset(h_m_words[i].word, 0, MAX_WORD + 1);
		pthread_mutex_init(&h_m_words[i].entry_mutex, NULL);
		h_m_words[i].frequency = 0;
	}
	return;
}

void s_f_init()
{
	for(int i = 0; i < MAX_FILES; i++)
	{
		memset(s_f[i].file_name, 0, MAX_FILE_NAME + 1);
		s_f[i].mod_time = 0;			
	}	
	return;
}

void threads_init()
{	
	for(int i = 0; i < MAX_THREADS; i++)
		threads[i] = INVALID_THREAD_ID;
	return;
}

void stop_init()
{
	for(int i = 0; i < HASH_MAP_SIZE; i++)
	{
		for(int j = 0; j < MAX_WORD + 1; j++)		
			stop_words[i][j] = 0;		
	}
	return;
}

void write_ende()
{
	pthread_mutex_lock(&console_mutex);
	printf("The program has finished it's job.\n");
	printf("Here are the informations about the scanned files:\n");
	for(int i = 0; i < MAX_FILES; i++)
	{
		if(strlen(s_f[i].file_name) != 0)
		{
			printf("Name of the file: %s\n", s_f[i].file_name);					
			printf("Time of last modification: %s", ctime(&s_f[i].mod_time)); // ne mora \n jer izgleda ctime vec ima \n
		}		
	}
	printf("ende\n");
	pthread_mutex_unlock(&console_mutex);
	return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void push_back_xdd(char *buffer, char c)
{
	int len = strlen(buffer);
	buffer[len] = c;	
	return;
}

int check_character(char *buffer, char *p)
{
	char c = *p;
	if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
	{
		if(c >= 'A' && c <= 'Z')		
			*p += 32;		
		return true;		
	}
	return false;
}

void print_safe(char *buffer)
{	
	pthread_mutex_lock(&console_mutex);
	printf("%s", buffer);
	pthread_mutex_unlock(&console_mutex);	
	return;
}

void open_stops(const char *path)
{	
	FILE *fp;
	char c;
	char *buffer = malloc(sizeof(char) * (MAX_WORD + 1));
	if((fp = fopen(path, "r")) == NULL)
	{		
		printf("Couldn't find the desired file\n");
		return;
	}	
	int word_counter;
	int truth;
	while((c = fgetc(fp)) != EOF)
	{
		memset(buffer, 0, MAX_WORD + 1);
		word_counter = 0;
		truth = true;
		while(c != ' ' && c != '\n' && c != '\t')
		{
			if(word_counter >= MAX_WORD || !truth)
				goto lul;
			if(check_character(buffer, &c))
				push_back_xdd(buffer, c);		
			else
				truth = false;
lul:
			word_counter++;
			if((c = fgetc(fp)) == EOF)
				goto jumper;
		}
		if(truth)
		{
			long hash_of_word = 0;
			hash_function(buffer, &hash_of_word);
			long i = 0;
			// Ovde radimo shift u desno, linearno naravno
			while(strlen(stop_words[hash_of_word + i]) != 0)
			{
				// Ovde proveravam da nema duplikata slucajno za trenutno posmatranu rec
				if(!strcmp(stop_words[hash_of_word + i], buffer))
					goto lul2;
				i++;
			}
			strcpy(stop_words[hash_of_word + i], buffer);			
		}
lul2:	
		truth = true; // neko smece od koga da bih skocio ovde
	}	
jumper:	
	free(buffer);
	fclose(fp);
	return;
}

int is_present_in_stops(char *buffer, long hash_of_word)
{
	// Zbog kolizije moram da proveravam dok ne dodjem do praznog mesta u stop_words
	// Ovde ne mora lock zato sto thread-ovi ne menjaju stop_words niz
	long i = 0;
	while(strlen(stop_words[hash_of_word + i]) != 0)
	{
		if(!strcmp(stop_words[hash_of_word + i], buffer))
			return true;
		i++;
	}
	return false;
}

void concurrent_addition_to_map(char *buffer)
{
	long hash_of_word = 0;
	hash_function(buffer, &hash_of_word);
	if(is_present_in_stops(buffer, hash_of_word))
		return;
	long i = 0;
	while(strlen(h_m_words[hash_of_word + i].word) != 0)
	{
		// Ovde proveravam da li se nalazim na poziciji gde se mozda ta rec nalazi
		pthread_mutex_lock(&h_m_words[hash_of_word + i].entry_mutex);
		if(!strcmp(h_m_words[hash_of_word + i].word, buffer))
		{
			// Opet ovde mora mutex
			// pthread_mutex_lock(&h_m_words[hash_of_word + i].entry_mutex);			
			h_m_words[hash_of_word + i].frequency += 1;
			pthread_mutex_unlock(&h_m_words[hash_of_word + i].entry_mutex);
			return;
		}
		pthread_mutex_unlock(&h_m_words[hash_of_word + i].entry_mutex);
		i++;
	}
	// U ovom trenutku smo naisli na prazno mesto u h_m_words i sada kada smo nasli adekvatno mesto, moramo da koristimo mutex
	pthread_mutex_lock(&h_m_words[hash_of_word + i].entry_mutex);
	strcpy(h_m_words[hash_of_word + i].word, buffer);
	h_m_words[hash_of_word + i].frequency = 1; // moze i ++ jer je na pocetku 0, ali ovako je malo preglednije
	pthread_mutex_unlock(&h_m_words[hash_of_word + i].entry_mutex);
	return;
}

void concurrent_addition_to_s_f(char *file_name, time_t last_mod_time)
{
	// Posto je scanned file niz relativno mali, mozemo da ga samo lock-ujemo celog
	// Ubacivanje ovde mozemo Round-Robin tehnikom
	pthread_mutex_lock(&s_f_mutex);	
	for(int i = 0; i < MAX_FILES; i++)
	{
		// Prvo provera da li je prazno mesto
		if(strlen(s_f[i].file_name) == 0)
		{
			strcpy(s_f[i].file_name, file_name);
			s_f[i].mod_time = last_mod_time;
			break;
		}
		// Onda provera da li smo mozda pronasli njegovo mesto
		if(!strcmp(s_f[i].file_name, file_name))
		{
			s_f[i].mod_time = last_mod_time;
			break;
		}	
	}	
	pthread_mutex_unlock(&s_f_mutex);
	return;
}

long concurrently_find_frequency(char *buffer)
{
	// Isto kao za dodavanje u h_m moramo da koristimo mutex-e
	long hash_of_word = 0;
	hash_function(buffer, &hash_of_word);
	long i = 0;
	long rez = 0;
	while(strlen(h_m_words[hash_of_word + i].word) != 0)
	{
		pthread_mutex_lock(&h_m_words[hash_of_word + i].entry_mutex);
		if(!strcmp(h_m_words[hash_of_word + i].word, buffer))
		{
			// pthread_mutex_lock(&h_m_words[hash_of_word + i].entry_mutex);
			rez = h_m_words[hash_of_word + i].frequency;
			pthread_mutex_unlock(&h_m_words[hash_of_word + i].entry_mutex);
			return rez;
		}
		pthread_mutex_unlock(&h_m_words[hash_of_word + i].entry_mutex);
		i++;
	}
	return rez;
}

void cleanup_func(void *arg)
{
	struct cleanup_args *klean = (struct cleanup_args *)arg;		
	printf("The thread that analyzed the file %s has successfully terminated\n", klean->path);	
	free(klean->path);
	free(klean->buffer);
	fclose(klean->fp);
	free(klean);	
	return;
}

// Namerno hocu da ostavim gornju implementaciju da bismo zapravo sad ovde videli kako izgleda kompaktnije napisan kod funkcije open_stops
void *analyze_file(void *arg)
{	
	char *path = (char *)arg;
	FILE *fp;	
	if((fp = fopen(path, "r")) == NULL)
	{		
		printf("Couldn't find the desired file\n");
		free(path);
		pthread_exit(NULL);		
	}
	char c;
	char *buffer = malloc(sizeof(char) * (MAX_WORD + 1));
	struct cleanup_args *klean = calloc(1, sizeof(struct cleanup_args));
	klean->path = path;	
	klean->buffer = buffer;
	klean->fp = fp;
	pthread_cleanup_push(cleanup_func, klean);
	
	struct stat st;
	stat(path, &st);
	time_t last_mod_time = st.st_mtime;
	int mastan_counter = 0;
	while(!cancel_point)
	{		
		int word_counter;
		int truth;		
		while((c = fgetc(fp)) != EOF)
		{
			memset(buffer, 0, MAX_WORD + 1);
			word_counter = 0;
			truth = true;
			mastan_counter++;
			while(c != ' ' && c != '\n' && c != '\t')
			{
				// Za registrovanje CTRL+D
				if(cancel_point)
					goto ddx;
				if(word_counter >= MAX_WORD || !truth)
					goto lul;
				if(check_character(buffer, &c))
					push_back_xdd(buffer, c);
				else
					truth = false;
lul:
				word_counter++;
				if((c = fgetc(fp)) == EOF)
					goto jumper;
				mastan_counter++;
			}
			if(truth)	
				concurrent_addition_to_map(buffer);			
		}
jumper:
		// Azuriranje scanned file niza
		concurrent_addition_to_s_f(path, last_mod_time);
		stat(path, &st);
		long current_pos = ftell(fp);				
		
		printf("Position of the EOF: %ld\n", current_pos);
		printf("Mastan counter: %d\n", mastan_counter); // mastan_counter dobro pamti poziciju, i on ce vrv sluziti kao offset, po meni			
		while(last_mod_time == st.st_mtime)
		{
			sleep(5);
			printf("Odspavao sam\n");
			stat(path, &st);
		}		
		printf("Hey it's been modified!!\n");
		last_mod_time = st.st_mtime;
		fclose(fp);
		fp = fopen(path, "r");
		fseek(fp, mastan_counter, SEEK_SET);
	}
ddx:
	pthread_cleanup_pop(1);
	pthread_exit(NULL);
}

void create_thread(const char *path)
{		
	int truth = false;
	int i;
	for(i = 0; i < MAX_THREADS; i++)
	{		
		if(threads[i] == INVALID_THREAD_ID)
		{			
			truth = true;
			break;
		}
	}	
	if(!truth)
	{		
		printf("There is no more space for the new thread\n");
		return;		
	}
	char *path_copy = malloc(sizeof(char) * (strlen(path) + 1));
	strcpy(path_copy, path);	
	pthread_create(&threads[i], NULL, analyze_file, (void *)path_copy);	
	return;
}

void free_memory(char *buffer, char **buffer_array, int it)
{	
	for(int i = 0; i < it; i++)
		free(buffer_array[i]);
	free(buffer_array);
	free(buffer);
	return;
}

int main(int argc, char *argv[])
{	
	h_m_init();
	s_f_init();
	stop_init();
	threads_init();
    calculate_pp();
	if(argc == 2)
		open_stops(*(argv + 1));
	
	while(true)
	{		
		char *buffer = calloc(MAX_INPUT + 1, sizeof(char));
		
		pthread_mutex_lock(&console_mutex);
		int ctrl_d = scanf("%[^\n]", buffer);
		if(ctrl_d != -1)
			getchar();
		pthread_mutex_unlock(&console_mutex);
		char **buffer_array = calloc(MAX_INPUT, sizeof(char*));
		int it = 0;
		char *token = strtok(buffer, " ");
		while(token != NULL)
		{
			buffer_array[it] = malloc(sizeof(char) * (strlen(token) + 1));
			strcpy(buffer_array[it], token);
			it++;
			token = strtok(NULL, " ");
		}
		
		if(ctrl_d == -1)
		{
			cancel_point = true;
			print_safe("Opa opa uhvatio sam CTRL+D\n");
			for(int i = 0; i < MAX_THREADS; i++)
			{				
				if(threads[i] != INVALID_THREAD_ID)
				{
					print_safe("Usao sam ovde : )\n");
					pthread_cancel(threads[i]);
					pthread_join(threads[i], NULL);
					threads[i] = INVALID_THREAD_ID;
				}					
			}
			cancel_point = false;
			print_safe("Opa opa izasao sam iz CTRL+D\n");
		}			
        else if(it == 1)
		{
			if(!strcmp("stop", buffer_array[0]))
			{							
				write_ende();
				free_memory(buffer, buffer_array, it);
				return 0;
			}				
			else
			{					
				pthread_mutex_lock(&console_mutex);
				printf("Frekvencija trazene reci: %ld\n", concurrently_find_frequency(buffer_array[0]));
				pthread_mutex_unlock(&console_mutex);
			}
				
		}
        else if(it == 2)
		{
			if(!strcmp("add", buffer_array[0]))						
				create_thread(buffer_array[1]);				
			else
			{				
				pthread_mutex_lock(&console_mutex);
				printf("Frekvencija trazene reci: %ld\n", concurrently_find_frequency(buffer_array[1]));
				pthread_mutex_unlock(&console_mutex);
			}
				
		}
		else
		{			
			pthread_mutex_lock(&console_mutex);
			printf("Frekvencija trazene reci: %ld\n", concurrently_find_frequency(buffer_array[it - 1]));
			pthread_mutex_unlock(&console_mutex);
		}		
		free_memory(buffer, buffer_array, it);
	}	
	return 0;
}
