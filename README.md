# Multithreaded word frequency index

Counts how often each word appears across a set of text files, using POSIX threads.
Written for the operating systems course.

The hash table has about a million slots and a separate mutex inside every slot
instead of one lock for the whole table, so two threads touching different words do
not wait on each other. It uses open addressing, and a thread locks only the slot it
is probing at that moment.

A thread can be cancelled while it is part way through a file, so each one installs
`pthread_cleanup` handlers that free its buffer and close its file on the way out.

Test corpora are not in the repo. Any large plain text file works.

## Building and running

    make
    ./analyze stop_words.txt

Then type commands: `add <file>` to index a file, a bare word to ask for its
frequency, `stop` to quit.
