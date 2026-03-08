# Distributed Systems Assignment 2

My submission for assignment 2 on the distributed systems course at LUT.

I apologize in advance for the long video (~9 minutes). I do feel it was necessary to properly demo the program and explain its internals.

## Explanation of how transparency, scalability and failure handling is implemented

Transparency is all about hiding the fact that the system is distributed. In this project this is realised by abstracting away network communication with an
ncurses TUI-application. From the client's perspective they're inputting messages into the application and they're appearing on the screen alongside others' messages.
The only times transparency isn't achieved is when connecting to the server for instance since that requires you to input the domain name or ip address for
the server you desire to connect to. In addition, if the server suffers a segmentation fault (a common C programming mistake), the server shuts down removing
the clients' ability to access the service.

Scalability, more specifically size scalability has been implemented by having the serverside handle each client connection on its own thread. However, I have not
tested the efficacy of this solution and there might be a better way to implement it. One way could be by polling each open connection and spawning threads to handle
requests only when there is data has been sent by the client.

The choice of C as the development language for this project was probably a bad idea from the perspective of failure handling as it introduces memory bugs alongside
its memory control. There is a chance that the server might crash because of my poor programming decisions, but in those scenarios both the server and the client are
equipped to detect whenever a failure has happened. If the server crashes the clients are able to detect that the socket has been closed and can provide the user with
an informative message. If a client suddenly dies the server can also detect this and proceed with proper cleanup operations.
