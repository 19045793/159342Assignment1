//=======================================================================================================================
// Course: 159.342
// Description: Cross-platform, Active mode FTP SERVER, Start-up Code for Assignment 1
//
// This code gives parts of the answers away but this implementation is only IPv4-compliant.
// Remember that the assignment requires a fully IPv6-compliant cross-platform FTP server that can communicate with a
// built-in FTP client either in Windows 10, Ubuntu Linux or MacOS.
//
// This program is cross-platform but your assignment will be marked only in Windows 10.
//
// You must change parts of this program to make it IPv6-compliant (replace all data structures and functions that work only with IPv4).
//
// Hint: The sample TCP server codes show the appropriate data structures and functions that work in both IPv4 and IPv6.
//       We also covered IPv6-compliant data structures and functions in our lectures.
//
// Author: n.h.reyes@massey.ac.nz
//=======================================================================================================================

#define USE_IPV6 false // if set to false, IPv4 addressing scheme will be used; you need to set this to true to
					   // enable IPv6 later on.  The assignment will be marked using IPv6!

#if defined __unix__ || defined __APPLE__
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h> //used by getnameinfo()
#include <iostream>

#elif defined __WIN32__
#include <winsock2.h>
#include <ws2tcpip.h> //required by getaddrinfo() and special constants
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#define WSVERS MAKEWORD(2, 2) /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
// The high-order byte specifies the minor version number;
// the low-order byte specifies the major version number.
WSADATA wsadata; // Create a WSADATA object called wsadata.
#endif

#define BUFFER_SIZE 256
#define DEFAULT_PORT "1234"
enum class FileType
{
	BINARY,
	TEXT,
	UNKNOWN
};

FileType file_type;

//********************************************************************
// MAIN
//********************************************************************
int main(int argc, char *argv[])
{
	//********************************************************************
	// INITIALIZATION
	//********************************************************************

	file_type = FileType::UNKNOWN;

#if defined __unix__ || defined __APPLE__
	// nothing to do here

#elif defined _WIN32

	int err = WSAStartup(WSVERS, &wsadata);

	if (err != 0)
	{
		WSACleanup();
		// Tell the user that we could not find a usable WinsockDLL
		printf("WSAStartup failed with error: %d\n", err);
		exit(1);
	}
#endif
#if defined __unix__ || defined __APPLE__

	int s, ns;				 // socket declaration
	int ns_data, s_data_act; // socket declaration
#elif defined _WIN32
	SOCKET s, ns;				// socket declaration
	SOCKET ns_data, s_data_act; // socket declaration
#endif

	struct sockaddr_storage localaddr, remoteaddr; // ipv4 only, needs replacing
	struct sockaddr_storage local_data_addr_act;   // ipv4 only, needs replacing

	char send_buffer[BUFFER_SIZE], receive_buffer[BUFFER_SIZE];
	char portNum[NI_MAXSERV];
	char clientHost[NI_MAXHOST];
	char clientService[NI_MAXSERV];

	// ns_data=INVALID_SOCKET;

#if defined __unix__ || defined __APPLE__
	ns_data = -1;
#elif defined _WIN32
	ns_data = INVALID_SOCKET;
#endif

	int active = 0;
	int n, bytes, addrlen;

	printf("\n============================================\n");
	printf("   << 159.342 Cross-platform FTP Server >>");
	printf("\n============================================\n");
	printf("   submitted by:     ");
	printf("\n           date:     ");
	printf("\n============================================\n");

	memset(&localaddr, 0, sizeof(localaddr));	// clean up the structure
	memset(&remoteaddr, 0, sizeof(remoteaddr)); // clean up the structure

	//********************************************************************
	// SOCKET
	//********************************************************************
	struct addrinfo *result = NULL;
	struct addrinfo hints;
	int iResult;
	memset(&hints, 0, sizeof(struct addrinfo));
        memset(&clientHints, 0, sizeof(struct addrinfo));
    if(USE_IPV6){
             hints.ai_family = AF_INET6;
         }	 else { //IPV4
             hints.ai_family = AF_INET;
         }
         hints.ai_socktype = SOCK_STREAM;
         hints.ai_protocol = IPPROTO_TCP;
         hints.ai_flags = AI_PASSIVE;
         clientHints.ai_socktype = SOCK_STREAM;
         clientHints.ai_protocol = IPPROTO_TCP;
         clientHints.ai_family = AI_NUMERICHOST;
	// CONTROL CONNECTION:  port number = content of argv[1]
	if (argc == 2)
	{
		iResult = getaddrinfo(NULL, argv[1], &hints, &result); // ipv4 only, needs replacing. In our lectures, we have an
		sprintf(portNum, "%s", argv[1]);
		printf("\nargv[1] = %s\n", argv[1]); // elegant way of resolving the local address and port to be used by the server.
	}
	else
	{
		iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result); // converts human-readable text strings representing hostnames or IP addresses
		// into a dynamically allocated linked list of struct addrinfo structures
		// IPV4 & IPV6-compliant
		sprintf(portNum, "%s", DEFAULT_PORT);
		printf("\nUsing DEFAULT_PORT = %s\n", portNum);
	}
	if (iResult != 0)
	{
		printf("getaddrinfo failed: %d\n", iResult);
#if defined _WIN32
		WSACleanup();
#endif
		return 1;
	}

#if defined __unix__ || defined __APPLE__
	s = -1;
#elif defined _WIN32
	s = INVALID_SOCKET;
#endif

	s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

#if defined __unix__ || defined __APPLE__
	if (s < 0)
	{
		printf("socket failed\n");
		freeaddrinfo(result);
	}
#elif defined _WIN32
	// check for errors in socket allocation
	if (s == INVALID_SOCKET)
	{
		printf("Error at socket(): %d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		exit(1); // return 1;
	}
#endif

	//********************************************************************
	// BIND
	//********************************************************************
	iResult = bind(s, result->ai_addr, (int)result->ai_addrlen);

#if defined __unix__ || defined __APPLE__
	if (iResult == -1)
	{
		printf("\nbind failed\n");
		freeaddrinfo(result);
		close(s); // close socket
#elif defined _WIN32
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(s);
		WSACleanup();
#endif
		return 1;
	}

	freeaddrinfo(result);

	//********************************************************************
	// LISTEN
	//********************************************************************
	listen(s, 5);

	//********************************************************************
	// INFINITE LOOP
	//********************************************************************
	int count = 0;
	//====================================================================================
	while (1)
	{ // Start of MAIN LOOP
		//====================================================================================
		addrlen = sizeof(remoteaddr);
		//********************************************************************
		// NEW SOCKET newsocket = accept  //CONTROL CONNECTION
		//********************************************************************
		printf("\n------------------------------------------------------------------------\n");
		printf("SERVER is waiting for an incoming connection request at port:%s", portNum);
		printf("\n------------------------------------------------------------------------\n");

#if defined __unix__ || defined __APPLE__
		ns = accept(s, (struct sockaddr *)(&remoteaddr), (socklen_t *)&addrlen);
#elif defined _WIN32
		ns = accept(s, (struct sockaddr *)(&remoteaddr), &addrlen);
#endif
		memset(clientHost, 0, sizeof(clientHost));		 // clean up the structure
		memset(clientService, 0, sizeof(clientService)); // clean up the structure

		if (ns < 0)
			break;
		getnameinfo((struct sockaddr *)&remoteaddr, addrlen, clientHost, sizeof(clientHost), clientService, sizeof(clientService), NI_NUMERICHOST);
		printf("\n============================================================================\n");
		printf("connected to [CLIENT's IP %s , port %s] through SERVER's port %s", clientHost, clientService, portNum); // ipv4 only, needs replacing
		printf("\n============================================================================\n");
		// printf("detected CLIENT's port number: %d\n", ntohs(remoteaddr.sin_port));

		// printf("connected to CLIENT's IP %s at port %d of SERVER\n",
		//     inet_ntoa(remoteaddr.sin_addr),ntohs(localaddr.sin_port));

		// printf("detected CLIENT's port number: %d\n", ntohs(remoteaddr.sin_port));
		//********************************************************************
		// Respond with welcome message
		//*******************************************************************
		count = snprintf(send_buffer, BUFFER_SIZE, "220 FTP Server ready. \r\n");
		if (count >= 0 && count < BUFFER_SIZE)
		{
			bytes = send(ns, send_buffer, strlen(send_buffer), 0);
		}

		//********************************************************************
		// COMMUNICATION LOOP per CLIENT
		//********************************************************************

		while (1)
		{

			n = 0;

			while (1)
			{
				//********************************************************************
				// RECEIVE MESSAGE AND THEN FILTER IT
				//********************************************************************
				bytes = recv(ns, &receive_buffer[n], 1, 0); // receive byte by byte...

				if ((bytes < 0) || (bytes == 0))
					break;
				if (receive_buffer[n] == '\n')
				{ /*end on a LF*/
					receive_buffer[n] = '\0';
					break;
				}
				if (receive_buffer[n] != '\r')
					n++; /*Trim CRs*/
			}

			if (bytes == 0)
				printf("\nclient has gracefully exited.\n"); // 2022

			if ((bytes < 0) || (bytes == 0))
				break;

			printf("[DEBUG INFO] command received:  '%s\\r\\n' \n", receive_buffer);

			//********************************************************************
			// PROCESS COMMANDS/REQUEST FROM USER
			//********************************************************************
			if (strncmp(receive_buffer, "USER", 4) == 0)
			{
				printf("Logging in... \n");
				count = snprintf(send_buffer, BUFFER_SIZE, "331 Password required (anything will do really... :-) \r\n");
				if (count >= 0 && count < BUFFER_SIZE)
				{
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				}
				printf("[DEBUG INFO] <-- %s\n", send_buffer);
				if (bytes < 0)
					break;
			}
			//---
			if (strncmp(receive_buffer, "PASS", 4) == 0)
			{

				count = snprintf(send_buffer, BUFFER_SIZE, "230 Public login successful \r\n");
				if (count >= 0 && count < BUFFER_SIZE)
				{
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				}
				printf("[DEBUG INFO] <-- %s\n", send_buffer);
				if (bytes < 0)
					break;
			}
			//---
			if (strncmp(receive_buffer, "SYST", 4) == 0)
			{
				printf("Information about the system \n");
				count = snprintf(send_buffer, BUFFER_SIZE, "215 Windows 64-bit\r\n");
				if (count >= 0 && count < BUFFER_SIZE)
				{
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				}
				printf("[DEBUG INFO] <-- %s\n", send_buffer);
				if (bytes < 0)
					break;
			}

			//---
			if (strncmp(receive_buffer, "TYPE", 4) == 0)
			{

				bytes = 0;
				printf("<--TYPE command received.\n\n");

				char objType;
				int scannedItems = sscanf(receive_buffer, "TYPE %c", &objType);
				if (scannedItems < 1)
				{
					count = snprintf(send_buffer, BUFFER_SIZE, "501 Syntax error in arguments\r\n");
					if (count >= 0 && count < BUFFER_SIZE)
					{
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					}
					printf("[DEBUG INFO] <-- %s\n", send_buffer);
					if (bytes < 0)
						break;
				}

				switch (toupper(objType))
				{
				case 'I':
					file_type = FileType::BINARY;
					printf("using binary mode to transfer files.\n");
					count = snprintf(send_buffer, BUFFER_SIZE, "200 command OK.\r\n");
					if (count >= 0 && count < BUFFER_SIZE)
					{
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					}
					printf("[DEBUG INFO] <-- %s\n", send_buffer);
					if (bytes < 0)
						break;

					break;
				case 'A':
					file_type = FileType::TEXT;
					printf("using ASCII mode to transfer files.\n");
					count = snprintf(send_buffer, BUFFER_SIZE, "200 command OK.\r\n");
					if (count >= 0 && count < BUFFER_SIZE)
					{
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					}
					printf("[DEBUG INFO] <-- %s\n", send_buffer);

					if (bytes < 0)
						break;

					break;
				default:
					count = snprintf(send_buffer, BUFFER_SIZE, "501 Syntax error in arguments\r\n");
					if (count >= 0 && count < BUFFER_SIZE)
					{
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					}
					printf("[DEBUG INFO] <-- %s\n", send_buffer);
					if (bytes < 0)
						break;
					break;
				}
			}
			//---
			if (strncmp(receive_buffer, "STOR", 4) == 0)
			{
				printf("unrecognised command \n");
				count = snprintf(send_buffer, BUFFER_SIZE, "502 command not implemented\r\n");
				if (count >= 0 && count < BUFFER_SIZE)
				{
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				}
				printf("[DEBUG INFO] <-- %s\n", send_buffer);
				if (bytes < 0)
					break;
			}
			//---
			if (strncmp(receive_buffer, "RETR", 4) == 0)
			{
				printf("unrecognised command \n");
				count = snprintf(send_buffer, BUFFER_SIZE, "502 command not implemented\r\n");
				if (count >= 0 && count < BUFFER_SIZE)
				{
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				}
				printf("[DEBUG INFO] <-- %s\n", send_buffer);
				if (bytes < 0)
					break;
			}
			//---
			if (strncmp(receive_buffer, "OPTS", 4) == 0)
			{
				printf("unrecognised command \n");
				count = snprintf(send_buffer, BUFFER_SIZE, "502 command not implemented\r\n");
				if (count >= 0 && count < BUFFER_SIZE)
				{
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				}
				printf("[DEBUG INFO] <-- %s\n", send_buffer);
				if (bytes < 0)
					break;
			}
			//---
			if (strncmp(receive_buffer,"EPRT",4)==0)  {  //more work needs to be done here
                     struct addrinfo *clientResult = NULL;
                     struct addrinfo clientHints;
                     clientHints.ai_socktype = SOCK_STREAM;
                     clientHints.ai_protocol = IPPROTO_TCP;
                     clientHints.ai_family = AI_NUMERICHOST;
                     char tempString[strlen(receive_buffer)];
                     char* ptr;
                     strcpy(tempString,receive_buffer);
                     strcpy(tempString, tempString+6);
                     strcpy(tempString, tempString+4);
                     ptr = strchr( tempString,  '|');
                     int index = (int)(ptr - tempString);
                     strcpy(clientHost, tempString);
                     clientHost[index-2]='\0';
                     int i=0;
                     while(clientHost[i]!='\0'){
                         if(clientHost[i]==',') {
                             clientHost[i] = ':';
                         }
                         i++;
                     }
                     strcpy(clientService, tempString+index+1);
                     clientService[strlen(clientService)-2]='\0';
                     int status = getaddrinfo(clientHost, clientService, &clientHints, &clientResult);
                     if (status != 0){
		         break;
		     }
#if defined __unix__ || defined __APPLE__
                     s_data_act = -1;
#elif defined _WIN32
                     s_data_act = INVALID_SOCKET;
#endif

                     s_data_act = socket(clientResult->ai_family, clientResult->ai_socktype, clientResult->ai_protocol);

                 }
			//---
			if (strncmp(receive_buffer, "CWD", 3) == 0)
			{
				printf("unrecognised command \n");
				count = snprintf(send_buffer, BUFFER_SIZE, "502 command not implemented\r\n");
				if (count >= 0 && count < BUFFER_SIZE)
				{
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				}
				printf("[DEBUG INFO] <-- %s\n", send_buffer);
				if (bytes < 0)
					break;
			}
			//---
			if (strncmp(receive_buffer, "QUIT", 4) == 0)
			{
				printf("Quit \n");
				count = snprintf(send_buffer, BUFFER_SIZE, "221 Connection close by client\r\n");
				if (count >= 0 && count < BUFFER_SIZE)
				{
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				}
				printf("[DEBUG INFO] <-- %s\n", send_buffer);
				if (bytes < 0)
					break;
			}
			//---
			char ip_decimal[NI_MAXHOST];
                 if (strncmp(receive_buffer, "PORT", 4) == 0) {
                     struct addrinfo *clientResult = NULL;
                     int act_port[2], port_dec;
                     printf("===================================================\n");
                     printf("\tActive FTP mode, the client is listening... \n");
                     active = 1; // flag for active connection --- idk what this does
                     int act_ip[4];
                     int scannedItems = sscanf(receive_buffer, "PORT %d,%d,%d,%d,%d,%d",
                                               &act_ip[0], &act_ip[1], &act_ip[2], &act_ip[3],
                                               &act_port[0], &act_port[1]);
                     if (scannedItems < 6) {
                         count = snprintf(send_buffer, BUFFER_SIZE, "501 Syntax error in arguments \r\n");
                         if (count >= 0 && count < BUFFER_SIZE) {
                             bytes = send(ns, send_buffer, strlen(send_buffer), 0);
                         }
                         printf("[DEBUG INFO] <-- %s\n", send_buffer);
                         if (bytes < 0)
                             break;
                     }
	             port_dec = act_port[0];
                     port_dec = port_dec << 8;
                     port_dec = port_dec + act_port[1];
                     count = snprintf(ip_decimal, NI_MAXHOST, "%d.%d.%d.%d", act_ip[0], act_ip[1], act_ip[2],
                                      act_ip[3]);
                     itoa(port_dec, clientService, sizeof port_dec);
                     strcpy(clientHost, ip_decimal);
                     int status = getaddrinfo(clientHost, clientService, &clientHints, &clientResult);
                     if(status!=0) {
                         break;
                     }

                     s_data_act = socket(clientResult->ai_family, clientResult->ai_socktype, clientResult->ai_protocol);
                 }
                 printf("\tCLIENT's IP is %s\n", clientHost); // IPv4 format
                 printf("\tCLIENT's Port is %s\n", clientService);
                 printf("===================================================\n");
                 sprintf(clientService, "%s", clientHost);

				// Note: the following connect() function is not correctly placed.  It works, but technically, as defined by
				//  the protocol, connect() should occur in another place.  Hint: carefully inspect the lecture on FTP, active operations
				//  to find the answer.
				if (connect(s_data_act, (struct sockaddr *)&local_data_addr_act, (int)sizeof(struct sockaddr)) != 0)
				{
					printf("trying connection in %s %s\n", clientHost, clientService);
					count = snprintf(send_buffer, BUFFER_SIZE, "425 Something is wrong, can't start active connection... \r\n");
					if (count >= 0 && count < BUFFER_SIZE)
					{
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);

						printf("[DEBUG INFO] <-- %s\n", send_buffer);
					}

#if defined __unix__ || defined __APPLE__
					close(s_data_act);
#elif defined _WIN32
					closesocket(s_data_act);
#endif
				}
				else
				{
					count = snprintf(send_buffer, BUFFER_SIZE, "200 PORT Command successful\r\n");
					if (count >= 0 && count < BUFFER_SIZE)
					{
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						printf("[DEBUG INFO] <-- %s\n", send_buffer);
						printf("Connected to client\n");
					}
				}
			}
			//---

			// technically, LIST is different than NLST,but we make them the same here
			if ((strncmp(receive_buffer, "LIST", 4) == 0) || (strncmp(receive_buffer, "NLST", 4) == 0))
			{
#if defined __unix__ || defined __APPLE__

				int i = system("ls -la > tmp.txt"); // change that to 'dir', so windows can understand

#elif defined _WIN32

				int i = system("dir > tmp.txt");
#endif
				printf("The value returned by system() was: %d.\n", i);

				FILE *fin;

				fin = fopen("tmp.txt", "r"); // open tmp.txt file

				// snprintf(send_buffer,BUFFER_SIZE,"125 Transfering... \r\n");
				// snprintf(send_buffer,BUFFER_SIZE,"150 Opening ASCII mode data connection... \r\n");
				count = snprintf(send_buffer, BUFFER_SIZE, "150 Opening data connection... \r\n");
				if (count >= 0 && count < BUFFER_SIZE)
				{
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					printf("[DEBUG INFO] <-- %s\n", send_buffer);
				}
				char temp_buffer[80];
				printf("transferring file...\n");
				while (!feof(fin))
				{
					strcpy(temp_buffer, "");
					if (fgets(temp_buffer, 78, fin) != NULL)
					{

						count = snprintf(send_buffer, BUFFER_SIZE, "%s", temp_buffer);
						if (count >= 0 && count < BUFFER_SIZE)
						{

							if (active == 0)
								send(ns_data, send_buffer, strlen(send_buffer), 0);
							else
								send(s_data_act, send_buffer, strlen(send_buffer), 0);
						}
					}
				}

				fclose(fin);
				count = snprintf(send_buffer, BUFFER_SIZE, "226 File transfer complete. \r\n");
				if (count >= 0 && count < BUFFER_SIZE)
				{
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					printf("[DEBUG INFO] <-- %s\n", send_buffer);
				}

#if defined __unix__ || defined __APPLE__
				if (active == 0)
					close(ns_data);
				else
					close(s_data_act);

#elif defined _WIN32
				if (active == 0)
					closesocket(ns_data);
				else
					closesocket(s_data_act);

					// OPTIONAL, delete the temporary file
					// system("del tmp.txt");
#endif
			}
			//---
			//=================================================================================
		} // End of COMMUNICATION LOOP per CLIENT
		  //=================================================================================

		//********************************************************************
		// CLOSE SOCKET
		//********************************************************************

#if defined __unix__ || defined __APPLE__
		close(ns);
#elif defined _WIN32

		closesocket(ns);
#endif
		printf("DISCONNECTED from %s\n", clientHost); // IPv4 only, needs replacing

		//====================================================================================
	} // End of MAIN LOOP
	//====================================================================================

	printf("\nSERVER SHUTTING DOWN...\n");

#if defined __unix__ || defined __APPLE__
	close(s);

#elif defined _WIN32
	closesocket(s);
	WSACleanup();
#endif
	return 0;
}
