/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold
#define MAXDATASIZE 100 // max number of bytes we can get at once 

void sigchld_handler(int s)
{
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

class Connection {
public:
	int fd;
	int timer;
	Connection(int _fd){
		fd = _fd;
		timer = 0;
	}

	void run() {
		char buf[MAXDATASIZE];
		int numbytes;
		while(1) {
			printf("Tick...\n");
			if (send(fd, "Hello, world!\n", 13, 0) == -1)
				perror("send");
			printf("Tock...\n");
			if ((numbytes = recv(fd, buf, MAXDATASIZE-1, 0)) == -1) {
				perror("recv");
			}
			else{
				buf[numbytes] = '\0';

				printf("server received '%s'\n",buf);
			}
			printf("Tack?...\n");

			if(timer++ > 50){
				closeConnection();
				printf("Closing Connection\n");
			}
		}
	}

	void closeConnection() {
		close(fd);
		exit(0);
	}
};

class Server {
public:
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes;
	char s[INET6_ADDRSTRLEN];
	int rv;
	
	Server(){
		yes=1;

		memset(&hints, 0, sizeof hints);
		// hints.ai_family = AF_UNSPEC;
		// hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE; // use my IP

		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;

		if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			exit(1);
		}

		// loop through all the results and bind to the first we can
		for(p = servinfo; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype,
					p->ai_protocol)) == -1) {
				perror("server: socket");
				continue;
			}

			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
					sizeof(int)) == -1) {
				perror("setsockopt");
				exit(1);
			}

			if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
				close(sockfd);
				perror("server: bind");
				continue;
			}

			break;
		}

		freeaddrinfo(servinfo); // all done with this structure

		if (p == NULL)  {
			fprintf(stderr, "server: failed to bind\n");
			exit(1);
		}

		// if (listen(sockfd, BACKLOG) == -1) {
		// 	perror("listen");
		// 	exit(1);
		// }
		int nonBlocking = 1;
		if ( fcntl( sockfd, 
			F_SETFL, 
			O_NONBLOCK, 
			nonBlocking ) == -1 )
		{
			printf( "failed to set non-blocking\n" );
			exit(1);
		}

		sa.sa_handler = sigchld_handler; // reap all dead processes
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;
		if (sigaction(SIGCHLD, &sa, NULL) == -1) {
			perror("sigaction");
			exit(1);
		}
	}

	void run() {
		printf("server: waiting for connections...\n");

		while(1) {  // main accept() loop
			sin_size = sizeof their_addr;
			new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
			if (new_fd == -1) {
				perror("accept");
				continue;
			}

			inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof s);
			printf("server: got connection from %s\n", s);

			if (!fork()) { // this is the child process
				close(sockfd); // child doesn't need the listener
				Connection c = Connection(new_fd);
				c.run();
				
			}
			close(new_fd);  // parent doesn't need this
		}
	}
};

int main(void)
{
	Server server = Server();
	server.run();
	return 0;
}


