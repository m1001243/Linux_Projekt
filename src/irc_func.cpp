#ifndef irc_func_cpp__#define irc_func_cpp__//Standard includes#include <iostream>#include <string>#include <cstdlib>#include <cstdio>#include <cstring>#include <fstream>#include <sstream>//Database include#include <sqlite3.h>//Cross platforms configurationa#ifdef WIN32#include <winsock2.h>#else#include <sys/socket.h>#include <sys/types.h>#include <netinet/in.h>#include <arpa/inet.h>#include <netdb.h>#include <unistd.h>#endif#ifdef WIN32SOCKET sockfd;#elseint sockfd;#endif//Include database functions#include "irc_func.h"#include "irc_sqlite3.h"using namespace std;#define MAX_LINE  1024	//Received datasizeint saveLog=0; //switch to on(1) if save log or off(2) if not const char *myName;//Create a connection with PORT and HOSTvoid irc_connect(int port, char* host){//Windows platform need to be start #ifdef WIN32	WSADATA wsa;	if(WSAStartup(MAKEWORD(2,0),&wsa) != 0)		exit(1);#endif		const int PORT = port;	const char *HOST = host;	sockfd = socket(AF_INET, SOCK_STREAM, 0);//TCP connection		if(static_cast<int> (sockfd) < 0){				ofstream f;		f.open ("Bot_Err.txt",std::ofstream::out | std::ofstream::app);		f << "socket()" << endl << endl;		f.close();				irc_disconnect();		exit(1);	}		hostent *hp = gethostbyname(HOST);	if(!hp){				ofstream f;		f.open ("Bot_Err.txt",std::ofstream::out | std::ofstream::app);		f << "gethostbyname()" << endl << endl;		f.close();				irc_disconnect();		exit(1);	}		//Setup the connection	sockaddr_in sin;	memset((char*)&sin, 0, sizeof(sin));	sin.sin_family = AF_INET;	memcpy((char*)&sin.sin_addr, hp->h_addr, hp->h_length);	sin.sin_port =htons(PORT);	memset(&(sin.sin_zero), 0, 8*sizeof(char));		int ret=0;	if((ret = connect(sockfd, (sockaddr*)&sin, sizeof(sin))) < 0){				ofstream f;		f.open ("Bot_Err.txt",std::ofstream::out | std::ofstream::app);		f << ret << endl << "connect()" << endl << endl;		f.close();				irc_disconnect();		exit(ret);	}}//Cross platforms disconnectionvoid irc_disconnect(){	#ifdef WIN32	closesocket(sockfd);	WSACleanup();#else	close(sockfd);#endif	}//Identify the bot for connection on a server void irc_identify(char* name,char* channel){		myName = name;		stringstream ss;		ss << "NICK " << name << "\r\n";	s2u(ss.str().c_str());	// Defines the name in channel		ss.str() = "";	ss << "USER " << name << " 0 0 :" << name << "\r\n";	s2u(ss.str().c_str()); // Defines User data		ss.str() = "";	ss << "PRIVMSG NICKSERV REGISTER password\r\n";	s2u(ss.str().c_str()); // Defines name and password if server support NickServ		ss.str() = "";	ss << "JOIN " << channel << "\r\n";	s2u(ss.str().c_str());	// Defines Channel		ss.str() ="";	ss << "PRIVMSG " << channel << " :Servus Leute\r\n";	string msg = ss.str();		s2u(msg.c_str()); // Give a meesage in channel}//Received until the connection break//Received data, parse and send back if shouldvoid irc_receive(){		for (;;) {		char buffer[MAX_LINE +1] = {0};				if(recv(sockfd, buffer, MAX_LINE * sizeof(char), 0) < 0) {						ofstream f;			f.open ("Bot_Err.txt",std::ofstream::out | std::ofstream::app);			f << "recv()" << endl << endl;			f.close();						irc_disconnect();			exit(1);		}			irc_parse(buffer);	}}//Send a PONG if the server send PINGvoid ping_parse(const string &buffer){		size_t pingPos = buffer.find("PING");		if(pingPos != string::npos){				string pong("PONG" + buffer.substr(pingPos + 4) + "\r\n");				s2u(pong.c_str());	}	}//Check and answer if it needsvoid bot_functions(const string &buffer){		size_t pos = 0;		// if message has => Botname: xxx <= then the answer back is => What! I�m busy. <=	if ((pos = buffer.find("Botname: xxx")) != string::npos) {		string tmp(	"PRIVMSG " + buffer.substr((buffer.find(":")+1),(buffer.find("!")-1)) + " What! I�m busy.\r\n");		s2u(tmp.c_str());		return;	}	// if message has => exit <= then the bot shut down himself	else if(buffer.find("exit") != string::npos){		//s2u("PRIVMSG #channel :bella ciao ciao\r\n");		irc_disconnect();		exit(0);	}	// if message has => name <= then the bot change his nick to given name params <=	else if((pos = buffer.find("name")) != string::npos){		string tmp = "NICK " + buffer.substr(pos + 5, buffer.length() -1) + "\r\n";		s2u(tmp.c_str());		return;	}	// if message has => join <= then the bot join a channel given in params <=	else if((pos = buffer.find("join")) != string::npos){		string tmp = "JOIN " + buffer.substr(pos + 5, buffer.length() -1) + "\r\n";		s2u(tmp.c_str());		return;	}	// if message has => leave <= then the bot part a channel given in params <=	else if((pos = buffer.find("leave")) != string::npos){		string tmp = "PART " + buffer.substr(pos + 5, buffer.length() -1) + "\r\n";		s2u(tmp.c_str());		return;	}	// if message has => savestart <= then the bot start to log in sqlite3 database <=	else if ((pos = buffer.find("savestart")) != string::npos){		saveLog = 1;		return;	}	// if message has => savestop <= then the bot stop to log <=	else if ((pos = buffer.find("savestop")) != string::npos){		saveLog = 0;		return;	}	// if message has => print <= then the bot prints the all datas back <=	else if ((buffer.find("print")) != string::npos){        string s = sql_getchat();                size_t end_pos = s.find("$");		string name = buffer.substr((buffer.find(":")+1),(buffer.find("!")-1));        while (end_pos != string::npos) {           			s2u(("PRIVMSG " +  name + " " + s.substr(0, end_pos) + "\r\n").c_str());           			s.erase(0, end_pos+1);            			end_pos = s.find("$");                }		return;	}	// if message has => lastseen <= then the prints last seen by user from the log <=	else if ((pos = buffer.find("lastseen ")) != string::npos) {		string tmp(	"PRIVMSG " + buffer.substr((buffer.find(":")+1),(buffer.find("!")-1)) + " " + sql_lastseen(buffer.substr(pos + 9, buffer.length() -1).c_str()));				s2u(tmp.c_str());				return;	}	// if message has => delete <= then the bot delete the database <=	else if(buffer.find("delete") != string::npos){		sql_delete();				return;	}				if (saveLog) { //save data in sqlite3 						time_t now = time(0);				time(&now);		tm* localtm = localtime(&now);				//Setup the date and time to a formated string		stringstream t;		t	<< localtm->tm_year+1900	<< "-" << localtm->tm_mon << "-" << localtm->tm_mday << " " 			<< localtm->tm_hour			<< ":" << localtm->tm_min << ":" << localtm->tm_sec;						string name		(buffer.substr(buffer.find(":") +1,		buffer.find("!") -1));	//Get the name		string msg		(buffer.substr(buffer.find(" :") +2,	buffer.length() -1));	//Get the message		string channel	(buffer.substr(buffer.find("!") +1,		buffer.find("@") -1));	//Get the channel name				//Add to database		sql_addchat	(name.c_str(), channel.c_str(), msg.c_str(), t.str().c_str());	}		return;}//Send the message to uplinkvoid s2u(const char* msg){ 		string tmp = msg;		//Add CRLF to send correct message	if(!(tmp.find("\r\n") == (tmp.length() -2)))		tmp += "\r\n";		send(sockfd, tmp.c_str(), strlen(msg), 0); }//Parse the message delete the unneeded characters//and pass throughvoid irc_parse(string buffer){		//Delete CRLF	if(buffer.find("\r\n") == buffer.length() -2)		buffer.erase(buffer.length() -2);		//if Server try to change your nick then force it back..HAHAHAHA	if (buffer.find("NICK ") != string::npos) {		stringstream ss;		ss << "NICK " << myName << "\r\n";		s2u(ss.str().c_str());		return;	}		if (buffer.find("PING") != string::npos) {		ping_parse(buffer);	}	else {		bot_functions(buffer);	}}#endif