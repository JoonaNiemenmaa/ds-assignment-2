client: src/client/client.c
	gcc -std=gnu99 -g -Wall -pedantic src/client/client.c -o client
server: src/server/server.c
	gcc -std=gnu99 -g -Wall -pedantic src/server/server.c -o server
