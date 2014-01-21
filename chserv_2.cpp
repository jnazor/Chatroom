/*
 * Group Members:  Jolie Nazor and Kristi Yost
 * Chatroom Project
 * CS 450 - Operating Systems
 * Michael Lyle
 * December 4, 2013
 * Description:  This program acts as a server.  It can connect with multiple client programs.
 * 	The particular client program passes one of three commands:  "read", "writ", or "join".
 *	Also, all three clients pass a username to the server.
 *
 *	join - User can enter various commands to control the user's location and chatrooms.
 *
 *	       'crea' - create a chatroom.  First enter command, press enter, then enter name 
 *			the for chatroom.
 *
 *	       'list' - lists all chatroom names into Command Client window.
 *
 *	       'join' - join a chatroom.  First enter command, press enter, then enter name
 *			of the chatroom to join. If the client is already in another chatroom,
 *			the client is taken out of that chatroom before joining another
 *			chatroom.
 *
 *	       'leav' - leave the current chatroom.  If there are no other clients in the 
 *			chatroom when the client leaves it, then the chatroom is removed.
 *
 *	read - The client can read information written to the server from another user in the
 *	       same chatroom.
 *
 *	writ - The client can write information to the server to be sent to the other users in
 *	       the same chatroom.
 *
 * Known Issues:  The server does not check for users with the same username from joining the
 *		  server.
 *		  Strange characters are displayed after the room names when listing the room
 *		  names to a command client (This was not happening before today.  :( ).
 *	          Not all windows close nicely when the clients are logging off (ctl + c).
 */

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <netinet/tcp.h>
#include <fstream>
#include <map>
#include <iterator>

#define SERVER_PORT 5432
#define MAX_PENDING 5
#define MAX_LINE 256

using namespace std;


// The following four variables are global to all server functions.
bool hasInfoToWrite = false;
pthread_mutex_t readerMutex = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t readerCond = PTHREAD_COND_INITIALIZER;

//Our Stuff
int clientID = 0;
char clientIDchar[2];
int new_s = 0;  //server accept value
char clientMessage[MAX_LINE];
string writeToRoom = "dumbThing";
string writingUser = "newDumbThing";

struct chatRoom {
	vector<string> users;
	string roomName;

	//~chatRoom() {}
};


vector<chatRoom> rooms;

map<int, string> writers;
map<string, int> readers;
map<int, string> cmders;


void *commandProcessThread( void *cFD );
void *readerProcessThread( void *cFD );
void *writerProcessThread( void *cFD );

int readn(int sd,char *ptr,int size);
int writen(int sd,char *ptr,int size);
void printRooms(int clientSD);
int findRoom(string nameST);
void leaveRoom(int roomIndex, string userName, string currentRoom);


//Server Stuff
int main() {

	cout << "Server is up and running!!!" << endl << endl;

	struct sockaddr_in sin;
	char command [5];
	unsigned int len = 0;
	int clientSD = 0;  // create socket from client
	int sock_index = 0;

	string userName = "";
	char uName[MAX_LINE];
	short uNameLen = 0;

	bool newName = true;

	bzero( (char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);

	if ( (clientSD = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		cout << "simplex-talk: socket" << endl;
		exit(1);
	}

	if ( (bind(clientSD, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
		cout << "simplex-talk: bind" << endl;
		exit(1);
	}
	
	listen(clientSD, 5);

	while(true) {

		if( (new_s = accept(clientSD, (struct sockaddr *)&sin, &len)) < 0) {
			cout << "simplex-talk: accept";
			exit(1);
		}

		char command[5];
		short cmdlen = 4;
		readn(new_s, (char *) &cmdlen, sizeof(short));
		readn(new_s, command, 4);
		command[4] = '\0';
		cout << "Command: " << command << endl << endl;

		readn(new_s, (char *) &uNameLen, sizeof(short));
		uNameLen = ntohs(uNameLen);
		readn(new_s, uName, uNameLen);
		uName[uNameLen] = '\0';
		
		for(int q = 0; q <= uNameLen; q++)
			userName = userName + uName[q];

		cout << "UserName: " << userName << endl;

		command[4] = '\0';	// assume we did successfully read 4 bytes. 
		pthread_t tid;
		int *pToClientSD = new int;
		*pToClientSD = new_s;

		if( strcmp( command, "read" ) == 0 ) {
			readers[userName] = new_s;
			pthread_create( &tid, NULL, readerProcessThread, pToClientSD );
		} else if( strcmp( command, "writ" ) == 0 ) {
			writers[new_s] = userName;
			pthread_create( &tid, NULL, writerProcessThread, 
					(void *) pToClientSD );
		} else if( strcmp( command, "join" ) == 0 ) {
			cmders[new_s] = userName;
			pthread_create( &tid, NULL, commandProcessThread, pToClientSD );
		} else
			cout << "Wrong command" << endl;

		//pthread_join(tid, NULL);
		userName = "";
		memset(uName, '\0', MAX_LINE);

	}

	close(new_s);

	return 0;
}


// Need to keep track of all client IDs, Client socket, and what rooms the client is in, and string name for username

// Need IP address and port number  SIN Field - client address

void *commandProcessThread( void *cFD ) {

	int clientSD = *(reinterpret_cast<int *>(cFD)); // Recast to the original datatype
	char cmd[5];
	short cmdlen = 0;
	int n;
	string currentRoom = "bye";
	int roomIndex = -1;

	string userName = cmders[clientSD];

	while(true) {

		readn(clientSD, (char *) &cmdlen, sizeof(short));
		cmdlen = ntohs(cmdlen);
	    	n = readn(clientSD, cmd, cmdlen);
            	cmd[4] = '\0';
		
		//cout << cmd << endl;

		if(strcmp(cmd, "crea") == 0) {
			cout << "create room" << endl;

			string nameRoom = "";
			char name[MAX_LINE];
			short namelen = 0;
			chatRoom temp;

			readn(clientSD, (char *) &namelen, sizeof(short));
			namelen = ntohs(namelen);
			readn(clientSD, name, namelen);
			name[namelen] = '\0';

			for(int j = 0; j <= namelen; j++)
			    nameRoom = nameRoom + name[j];

			temp.roomName = nameRoom;
			if(findRoom(nameRoom) == -1)
			    rooms.push_back(temp);	
			else
			    cout << "Error: chatroom name already exists." 
			         << endl;		
		}
		if(strcmp(cmd, "join") == 0 /*&& roomIndex == -1*/) {
			cout << "join room" << endl;

			//Need info:  get room name
			char name[MAX_LINE];
			short namelen = 0;
			string nameRoom = "";

			readn(clientSD, (char *) &namelen, sizeof(short));
			namelen = ntohs(namelen);
			readn(clientSD, name, namelen);
			name[namelen] = '\0';

			for(int j = 0; j <= namelen; j++)
				nameRoom = nameRoom + name[j];

			if(roomIndex != -1)
				leaveRoom(roomIndex, userName, currentRoom);

			int roomNo;
			if((roomNo = findRoom(nameRoom)) > -1) {
				//Add user to found room
				//User name gets put into array of users in chatroom struct
				rooms[roomNo].users.push_back(userName);
				currentRoom = rooms[roomNo].roomName;
				roomIndex = roomNo;
				//cout << "Found name!" << roomNo << endl;
			} //else
				//cout << "Did not find name.  :(" << endl;
		}
		if(strcmp(cmd, "list") == 0) {
			cout << "Show list of rooms" << endl;
		        printRooms(clientSD);
		}
		if(strcmp(cmd, "leav") == 0) {
			cout << "\nUser leaves room" << endl;

			leaveRoom(roomIndex, userName, currentRoom);

			roomIndex = -1;
			currentRoom = "bye";
		}

		memset(cmd, '\0', 5);
	}
}





void *readerProcessThread( void *cFD ) {

	int clientSD = *(reinterpret_cast<int *>(cFD)); // Recast to the original datatype
	
	char buf[MAX_LINE];
	short buflen = 0;
	int n, len;
	vector<int> all_readers;
	int doNotSend = -1;
	string message = "";
	int index = 0;

	while( true ) {
	    short msgLen = 0;

    	    pthread_mutex_lock( &readerMutex );
	    pthread_cond_wait( &readerCond, &readerMutex );
	    if( hasInfoToWrite ) {

		for(int j = 0; j < rooms.size(); j++)
			if(writeToRoom.compare(rooms[j].roomName) == 0)
				index = j;

		for(int k = 0; k < rooms[index].users.size(); k++)
		    all_readers.push_back(readers[rooms[index].users[k]]);

		doNotSend = readers[writingUser];

		message = writingUser.substr(0, writingUser.length() - 1);
		message += ": ";

		for(int m = 0; m < strlen(clientMessage); m++)
			message += clientMessage[m];

		//cout << "\nMessage: " << message << endl;
		//cout << "Message Length: " << message.length() << endl;

		char sendMe[message.length() + 1];
		short sendlen = 0;

		for(int q = 0; q <= message.length(); q++)
			sendMe[q] = message[q];

		sendlen = strlen(sendMe);
		sendMe[sendlen] = '\0';
		sendlen = htons(sendlen);

		//cout << "\nSendMe: " << sendMe << endl;

	 	for(int i = 0; i < all_readers.size(); i++) {
			if(all_readers[i] != doNotSend) {
				writen( all_readers[i], (char *) &sendlen, sizeof(short));
				writen( all_readers[i], sendMe, strlen(sendMe));
			}
		}
		hasInfoToWrite = false;
		memset(clientMessage, '\0', MAX_LINE);
		writeToRoom = "";
		all_readers.clear();
	    } 
	    pthread_mutex_unlock( &readerMutex );
	}
}




void *writerProcessThread( void *cFD ) {

	int clientSD = *((int *) cFD);
	short buflen = 0;
	char buf[MAX_LINE];
	int n, len;
	int tally = 0;
	string currentRoom = "hi";
	string userName = "bye";

	userName = writers[clientSD];

	//For Writer Client writerProcessThread
        while( true ){

	    short msgLen = 0;
	    readn(clientSD, (char *) &msgLen, sizeof(short));
	    msgLen = ntohs(msgLen);
	    readn(clientSD, clientMessage, msgLen);	

	    for(int j = 0; j < rooms.size(); j++) {
		for(int k = 0; k < rooms[j].users.size(); k++)
		    if(userName.compare(rooms[j].users[k]) == 0)
			currentRoom = rooms[j].roomName;		
	    }

	    clientMessage[msgLen] = '\0';
	    //if(strlen(clientMessage) > 0) {
	    if(rooms.size() != 0) {
	        pthread_mutex_lock( &readerMutex ); 
	        hasInfoToWrite = true; 
		writeToRoom = currentRoom;
		writingUser = userName;
	        pthread_cond_signal( &readerCond ); 
	        pthread_mutex_unlock( &readerMutex );
	    }
	    
        }
}




void leaveRoom(int roomIndex, string userName, string currentRoom) {

	vector<string>::iterator q = rooms[roomIndex].users.begin();
	for(; q != rooms[roomIndex].users.end(); advance(q, 1)) {
		if(userName.compare(*q) == 0)
			break;
	}
			
	rooms[roomIndex].users.erase(q);

	if(rooms[roomIndex].users.size() == 0){
		vector<chatRoom>::iterator p = rooms.begin();
		for(; p != rooms.end(); advance(p, 1)) {
			if(currentRoom.compare(p->roomName) == 0)
				break;
		}
		rooms.erase(p);
	}
}



// Prints a list of chatrooms to the Command Client at the file descriptor value of clientSD.
void printRooms(int clientSD) {
	
	char name[MAX_LINE];
	short namelen = 0;
	short roomSize = rooms.size();

	writen(clientSD, (char *) &roomSize, sizeof(short));
	
	for(int i = 0; i < rooms.size(); i++) {
	    cout << "rooms roomName: " << rooms[i].roomName << endl;
	    int size = rooms[i].roomName.size();

	    for(int j = 0; j < size; j++)
	    	name[j] = rooms[i].roomName.at(j);

	    namelen = strlen(name);
	    name[namelen] = '\0';
	    namelen = htons(namelen);

	    writen(clientSD, (char *) &namelen, sizeof(short));
	    writen(clientSD, name, strlen(name));  
	}

}


// Returns the index value of the chatroom matching nameST found in the rooms vector.  Returns 
// -1 a match chatroom name to nameST was not found in the rooms vector.
// nameST is the name of a chatroom.
int findRoom(string nameST) {

	int index = -1;
	//cout << "Find Room: Size of rooms: " << rooms.size() << endl;
	for(int i = 0; i < rooms.size(); i++) {
		if(nameST.compare(rooms[i].roomName) == 0) {
			index = i;
		}
	}

	//cout << "Find Room: Index: " << index << endl;

	return index;
}




int readn(int sd,char *ptr,int size)
{         
    int no_left,no_read;
    no_left = size;
    while (no_left > 0) { 
        no_read = read(sd,ptr,no_left);
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
        no_written = write(sd,ptr,no_left);
        if(no_written <=0)  
		return(no_written);
        no_left -= no_written;
        ptr += no_written;
    }
    return(size - no_left);
}


