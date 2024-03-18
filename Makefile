CFLAGS = -Wall -g

all: server subscriber

server: -lm server.c struct.c

subscriber: -lm subscriber.c struct.c

.PHONY: clean run_server run_subscriber

run_server:
	./server ${PORT}

run_subscriber:
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT_SERVER}

clean:
	rm -f server subscriber
