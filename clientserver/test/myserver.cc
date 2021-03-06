/* myserver.cc: sample server program */

#include "server.h"
#include "connection.h"
#include "connectionclosedexception.h"
#include "database.h"
#include "protocol.h"
#include "netutils.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <sstream>

#define INTSIZE sizeof(int)

using namespace protocol;
using namespace std;
using client_server::Server;
using client_server::Connection;
using client_server::ConnectionClosedException;

/*
 * Read an integer from a client.
 */
int readNumber(Connection* conn) {
    unsigned char byte1 = conn->read();
    unsigned char byte2 = conn->read();
    unsigned char byte3 = conn->read();
    unsigned char byte4 = conn->read();
    return (byte1 << 24) | (byte2 << 16) | 
        (byte3 << 8) | byte4;
}

/*
 * Send the string 's' to a client.
 */
void writeString(const string& s, Connection* conn) {
  for (size_t i = 0; i < s.size(); ++i)
    {
      conn->write(s[i]);
    }
}

string readCommand(Connection* conn)
{
  ostringstream iss;
  ostringstream oss;
  oss << conn->read(); //command type
  char tmp = conn->read();
  
  while(tmp != Protocol::COM_END)
    {
      iss << tmp;
      if(tmp == Protocol::PAR_NUM || tmp == Protocol::PAR_STRING)
	{
	  //hack to get around the fact that a number byte might be equal to 8 and
	  //therfore equal to COM_END
	  iss << conn->read();
	  iss << conn->read();
	  iss << conn->read();
	  iss << conn->read();
	}
      tmp = conn->read();
    }
  string input = iss.str();
  for(unsigned i = 0; i < input.size(); ++i)
    {
      if(input[i] == Protocol::PAR_NUM)
	{
	  oss << " " << ntohl(&input[i + 1]);
	  i += INTSIZE;
	}
      else if(input[i] == Protocol::PAR_STRING)
	{
	  string tmp = ntohs(&input[i + 1]);
	  oss << " " << tmp;
	  i += tmp.size();
	}
    }
  return oss.str();
}

string executeCommand(string &input)
{
  istringstream in(input);
  ostringstream response;
  string  result, name, author, text;
  char c;
  int newsID, artID; //identification number
  in >> c;
  switch (c)
    {
    case Protocol::COM_LIST_NG:
      response << Protocol::ANS_LIST_NG;
      response << Database::instance().listNewsgroups();
      break;
      
    case Protocol::COM_CREATE_NG:
      in >> name;
      response << Protocol::ANS_CREATE_NG;
      result =  Database::instance().addNewsgroup(name);
      response << result;
      break;
      
    case Protocol::COM_DELETE_NG:
      in >> newsID;
      response << Protocol::ANS_DELETE_NG;
      response << Database::instance().delNewsgroup(newsID);
      break;

    case Protocol::COM_LIST_ART:
      in >> newsID;
      response << Protocol::ANS_LIST_ART;
      response << Database::instance().listArticles(newsID);
      break;
      
    case Protocol::COM_CREATE_ART:
      in >> newsID;
      in >> name;
      in >> author;
      in >> text;
      response << Protocol::ANS_CREATE_ART;
      response << Database::instance().addArticle(newsID, name, author, text);
      break;

    case Protocol::COM_DELETE_ART:
      in >> newsID;
      in >> artID;
      response << Protocol::ANS_DELETE_ART;
      response << Database::instance().delArticle(newsID, artID);
      break;

    case Protocol::COM_GET_ART:
      in >> newsID;
      in >> artID;
      response << Protocol::ANS_GET_ART;
      response << Database::instance().getArticle(newsID, artID);
      break;

    default:
      cout << "Recived unknown command: " << c << endl;
      break;
    }
  
  response << Protocol::ANS_END;
  return response.str();
}

int main(int argc, char* argv[]){
    if (argc != 2) {
        cerr << "Usage: myserver port-number" << endl;
        exit(1);
    }
    
    Server server(atoi(argv[1]));
    if (! server.isReady()) {
        cerr << "Server initialization error" << endl;
        exit(1);
    }
    
    while (true) {
        Connection* conn = server.waitForActivity();
        if (conn != 0) {
            try {
	      string command = readCommand(conn);
	      string response = executeCommand(command);
 	      writeString(response, conn);
            }
            catch (ConnectionClosedException&) {
                server.deregisterConnection(conn);
                delete conn;
                cout << "Client closed connection" << endl;
            }
        }
        else {
            server.registerConnection(new Connection);
            cout << "New client connects" << endl;
        }
    }
}
