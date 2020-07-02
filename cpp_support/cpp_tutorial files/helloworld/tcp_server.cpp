#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <sstream>

using namespace std;
 
int main()
{
    // Create a socket
    int listening = socket(AF_INET, SOCK_STREAM, 0);
//     if (listening == -1){cerr << "Can't create a socket! Quitting" << endl;return -1;}
    
    // Bind the ip address and port to a socket
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(5000);
    inet_aton("127.0.0.1", &hint.sin_addr);
    bind(listening, (sockaddr*)&hint, sizeof(hint));

    listen(listening, SOMAXCONN);// Tell Winsock the socket is for listening
  
    
    sockaddr_in client; // Wait for a connection
    socklen_t clientSize = sizeof(client);
    
    int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
    
    char host[NI_MAXHOST];      // Client's remote name
    char service[NI_MAXSERV];   // Service (i.e. port) the client is connect on
    memset(host, 0, NI_MAXHOST); // same as memset(host, 0, NI_MAXHOST);
    memset(service, 0, NI_MAXSERV);
   
//     if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
//     {
//         cout << host << " connected on port " << service << endl;
//     }
//     else
//     {
//         inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
//         cout << host << " connected on port " << ntohs(client.sin_port) << endl;
//     }

    cout << host << " connected on port " << service << endl;
    
    close(listening);   // Close listening socket
    char buf[4096]; // While loop: accept and echo message back to client
 
    while (true)
    {
        memset(buf, 0, 4096);
        
        int bytesReceived = recv(clientSocket, buf, 4096, 0); // Wait for client to send data
        if (bytesReceived == -1){cerr << "Error in recv(). Quitting" << endl;break;}
        if (bytesReceived == 0){ cout << "Client disconnected " << endl; break;}

	std::string text = string(buf, 0, bytesReceived) ;
	stringstream ss; float fl;
	ss << text; // sends the contents of str to the stringstream ss
	ss >> fl;
	cout << fl << endl;
	
//      cout << string(buf, 0, bytesReceived) << endl;
	
//      send(clientSocket, buf, bytesReceived + 1, 0); // Echo message back to client
    }
    close(clientSocket); // Close the socket
 
    return 0;
}