/*
 * Writer Client program being used by Jolie Nazor and Kristi Yost for the CS 450 Chatroom 
 * Project.  This program is originally taken from the text book used for the CS 365
 * Networking class, and has been slightly modified in the main while loop.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;

#define SERVER_PORT 5432
#define MAX_LINE 256

int readn(int sd,char *ptr,int size);
int writen(int sd,char *ptr,int size);

int main(int argc, char * argv[]) {
	FILE * fp;
	struct hostent * hp;
	struct sockaddr_in sin;
	char * host;
	char buf[MAX_LINE];
	int s, new_s;
	int len;
	char msg[MAX_LINE];
	short buflen, msglen;
	char userName[MAX_LINE];
	short uNameLen = 0;

	memset(msg, '\0', MAX_LINE);
	memset(buf, '\0', MAX_LINE);

	//printf("Size of argc: %d", argc);

	if (argc == 2) {
		host = argv[1];
	} else {
		fprintf(stderr, "usage: simplex-talk host\n");
		exit(1);
	}

	/* translate host name into peer's IP address */
	hp = gethostbyname(host);
	if (!hp) {
		fprintf(stderr, "simplex-talk: unknown host: %s\n", host);
		exit(1);
	}

	/* build address data structure */
	bzero((char *) & sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *) & sin.sin_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);

	/* active open */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("simplex-talk: socket");
		exit(1);
	}
	if (connect(s, (struct sockaddr *) & sin, sizeof(sin)) < 0) {
		perror("simplex-talk: connect");
		close(s);
		exit(1);
	}

	char command[5] = {'w', 'r', 'i', 't', '\0'};
	short cmdlen = strlen(command);
	cmdlen = htons(cmdlen);
	writen(s, (char *) &cmdlen, sizeof(short));
	writen(s, command, strlen(command));

	cout << "Enter username: ";
	cin >> userName;
	cout << endl;

	uNameLen = strlen(userName);
	userName[uNameLen] = '\0';
	uNameLen = htons(uNameLen);

	writen(s, (char *) &uNameLen, sizeof(short));
	writen(s, userName, strlen(userName));

	/* main loop: get and send lines of text */
	while (fgets(buf, sizeof(buf), stdin)) {
		
		buflen = strlen(buf);
		buf[buflen] = '\0';
		buflen = htons(buflen);
		//fputs(buf, stdout);

		writen(s, (char *) &buflen, sizeof(short));
		writen(s, buf, strlen(buf));
		memset(buf, '\0', MAX_LINE);
	}
	close(s);
	return 0;
}



int readn(int sd,char *ptr,int size)
{         
    int no_left,no_read;
    no_left = size;
    while (no_left > 0) { 
        no_read = recv(sd,ptr,no_left, 0);
        if(no_read <0)  
		return(no_read);
        if (no_read == 0) 
		break;
        no_left -= no_read;
        ptr += no_read;
    }
    return(size - no_left);
}




int writen(int sd,char *ptr,int size)
{         
    int no_left,no_written;
    no_left = size;
    while (no_left > 0) { 
        no_written = send(sd,ptr,no_left, 0);
        if(no_written <=0)  
		return(no_written);
        no_left -= no_written;
        ptr += no_written;
    }
    return(size - no_left);
}

