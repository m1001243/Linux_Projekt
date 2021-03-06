#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

using namespace std;

const unsigned int MAX_LINE = 1024; //Größe des Empfangpuffers
const int PORT = 6667;
const char *HOST = "irc.insiderZ.DE";

#ifdef WIN32
SOCKET sockfd;
#else
int sockfd;
#endif

void irc_connect(){
	
#ifdef WIN32
	//Windows-Socket initialisieren
	WSADATA wsa;
	if( WSAStartup( MAKEWORD( 2, 0 ), &wsa ) != 0)
		exit(1);
#endif
	
	sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	
	//Cast nur in Windows nötig
	if (static_cast<int> (sockfd) < 0) {
		perror("socket()");
		irc_disconnect();
		exit(1);
	}
	
	//Adressermittlung
	hostent *hp = gethostbyname(HOST);
	if(!hp){
		cerr << "gethostbyname()" << endl;
		irc_disconnect();
		exit(1);
	}
	
	sockaddr_in sin;
	memset( (char*)&sin, 0, sizeof(sin) );
	sin.sin_family = AF_INET;
	memcpy( (char*)&sin.sin_addr, hp->h_addr, hp->h_lenght );
	sin.sin_port = htons(PORT);
	memset(&(sin.sin_zero), 0, 8*sizeof(char));
	
	//Verbindungsaufbau
	if (connect(sockfd, (sockaddr*)&sin, sizeof(sin)) == -1) {
		perror("connect()");
		irc_disconnect();
		exit(1);
	}
}

void irc_disconnect() {
#ifdef WIN32
	//Windows-Socket freigeben
	closesocket(sockfd);
	WSACleanup();
#else
	close(sockfd);
#endif
}

void s2u(const char *msg){ //send to uplink
	send(sockfd, msg, strlen(msg), 0);
}

void ping_parse(const string &buffer){
	size_t pingPos = buffer.find("PING");
	
	if (pingPos != string::npos) {
		string pong("PONG" + buffer.substr(pingPos + 4) + "\r\n");
		cout << pong;
		s2u(pong.c_str());
	}
}

void irc_parse(string buffer){
	
	if (buffer.find("\r\n") == (buffer.length() - 2)) {
		buffer.erase(buffer.length() - 2);
		ping_parse(buffer);
		bot_functions(buffer);
	}
}

int main() {
	irc_connect();
	
	for (;;) {
		char buffer[MAX_LINE+1] = {0};
		
		if (recv(sockfd, buffer, MAX_LINE*sizeof(char), 0) < 0) {
			perror("recv()");
			irc_disconnect();
			exit(1);
		}
		
		cout << buffer;
		irc_parse(buffer);
	}
	
	irc_disconnect();
	
}





