CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lrt -lpthread -lm

SRCS = pipeline.c publisher.c inverter.c bmp.c blurrer.c edger.c
OBJS = $(SRCS:.c=.o)
EXECS = pipeline publisher inverter blurrer edger

all: $(EXECS)

pipeline: pipeline.o bmp.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

publisher: publisher.o bmp.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

inverter: inverter.o bmp.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

blurrer: blurrer.o bmp.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

edger: edger.o bmp.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(EXECS)
	find . -type f -name '*_processed.bmp' -delete
	find . -type f -name '*_blurred.bmp' -delete
	find . -type f -name '*_edges.bmp' -delete
	find . -type f -name '*_filtered.bmp' -delete
	make

.PHONY: all clean