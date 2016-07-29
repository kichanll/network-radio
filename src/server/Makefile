CXXFLAGS+=-pthread -I../include -g
LDFLAGS+=-pthread -lstdc++

all: server

server: main.o thr_channel.o thr_list.o medialib.o mytbf.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -rf main.o thr_channel.o thr_list.o edialib.o mytbf.o server
