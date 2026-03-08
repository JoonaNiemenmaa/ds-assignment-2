client: src/client/client.c src/server/string.c
	gcc -std=gnu99 -pthread -lncurses -g -Wall -pedantic -fsanitize=address src/client/client.c -o client
server: src/server/server.c src/server/clients.c src/server/string.c
	gcc -std=gnu99 -pthread -g -Wall -pedantic -fsanitize=address src/server/server.c -o server
clean:
	rm client server
