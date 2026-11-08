all: analyze.c
	gcc analyze.c -o analyze -pthread -time


clean:
	rm analyze
