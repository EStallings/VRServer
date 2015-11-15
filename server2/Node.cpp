#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <time.h>

#include "Net.h"
#include "Model.h"

using namespace std;
using namespace net;

#define TIMEOUT_TICKS 100000000

int main( int argc, char * argv[] )
{
	// initialize socket layer

	if ( !initializeSockets() )
	{
		printf( "failed to initialize sockets\n" );
		return 1;
	}
	
	// create socket

	int port = 30000;

	if ( argc == 2 )
		port = atoi( argv[1] );

	printf( "creating socket on port %d\n", port );

	Socket socket;
	if ( !socket.open( port ) )
	{
		printf( "failed to create socket!\n" );
		return 1;
	}

	vector<Address> addresses;

	// send and receive packets until the user ctrl-breaks...

	Model model = Model();
	int packetsSent = 0;
	int packetsRecv = 0;
	int ticks = 0;
	while ( true )
	{
		ticks++;
		clock_t t;
  		t = clock();
  
  
		map<int, StateObject> stateObjects = model.getStateObjects();
		vector<int> idsToUpdate = model.getUpdatedIds();
		vector<Address> nextAddresses;
		
		for ( int i = 0; i < (int) addresses.size(); ++i ){
			
			if(addresses[i].getTimeout() > TIMEOUT_TICKS) {
				printf("Disconnecting Peer %d\n", i);
				unsigned char data[] = {'d'};
				socket.send( addresses[i], data, 1);
				continue;
			}

			// printf("Sending data to peer %d t=%d\n", i, addresses[i].getTimeout());

			int ip = addresses[i].getAddress();
			for( int j = 0; j < (int) idsToUpdate.size(); ++j){
				int id = idsToUpdate[j];

				if(stateObjects.find(id) != stateObjects.end()) {
					// Object found. Check timestamp
					// printf("%d vs %d, tu=%d\n", stateObjects[id].lastUpdatedIP, ip, stateObjects[id].timesUpdated);
					if(stateObjects[id].lastUpdatedIP == ip) continue;
					// printf("   Sending Packet...\n");
					packetsSent++;
					stateObjects[id].data[0] = (unsigned char)'n';
					socket.send( addresses[i], stateObjects[id].data, OBJ_PACK_LENGTH);
				}
			}
			
			addresses[i].incTimeout();
			nextAddresses.push_back(addresses[i]);
		}
		addresses = nextAddresses;
		model.resetIdsToUpdate();
			
		while ( true )
		{
			Address sender;
			unsigned char buffer[256];
			int bytes_read = socket.receive( sender, buffer, sizeof( buffer ) );
			bool newSender = false;
			if ( !bytes_read )
				break;

			string message(reinterpret_cast<char*>(buffer));
			// cout << "Msg " << message << endl;

			if(std::find(addresses.begin(), addresses.end(), sender) != addresses.end()) {
				/* v contains x */
				for( int j = 0; j < (int) addresses.size(); ++j) {
					if(addresses[j] == sender){
						sender = addresses[j];
						break;
					}
				}
			} else {
				/* v does not contain x */
				newSender = true;
			}
			
			// printf( "received packet from %d.%d.%d.%d:%d (%d bytes)\n", sender.getA(), sender.getB(), sender.getC(), sender.getD(), sender.getPort(), bytes_read );
			int newId, theirId;
			unsigned char sendBack[255];
			sender.resetTimeout();

			switch(buffer[0]){
				case 'a': // a - handshake signal from client
					// printf("Recieved Handshake Signal From Client\n");
					if(newSender) {
						cout << "Discovered peer! " << message << "\n";
						addresses.push_back(sender);
						sendBack[0] = 'b';
						socket.send(sender, sendBack, sizeof(sendBack));
					}
					break;
				case 'b': // b - handshake signal from server
					// printf("Incorrectly Recieved Server-Side Handshake Signal\n");
					break;
				case 'c': // c - ping signal, no data
					// printf("Pinged by peer!\n");
					break;
				case 'd': // d - disconnected by server due to timeout
					// printf("Incorrectly Recieved Server-Side Disconnect Signal\n");
					break;
				case 'e': // e - disconnected from server
					// printf("Client Disconnected\n");
					sender.maxTimeout();
					break;
				case 'i': // i - initialize global object
								// Object exists for all people. This will only happen during boot-up sequence.
								// Ignored by server if it already has this information
					printf("Initialize Global Object\n");
					model.initializeGlobal(buffer, sender.getAddress());
					break;
				case 'j': // j - initialize non-global object
								// Object created on one client, needs to be replicated across all clients
								// Sender client needs to be informed what the new (server-side) object ID is.
					printf("Initialize Local Object\n");
					newId = model.initializeLocal(buffer, sender.getAddress());
					theirId = (int)buffer[4];

					//Build response packet
					sendBack[0] = (unsigned char)'k';
					sendBack[1] = (unsigned char)newId;
					sendBack[4] = (unsigned char)theirId;

					//Send back newid/oldid pair
					socket.send(sender, sendBack, sizeof(sendBack));
					break;
				case 'k': // k - new ID to old ID callback for non-local initiation
					// printf("Incorrectly Recieved ID Callback\n");
					break;
				case 'm': // m - object update Client-To-Server
					// printf("Object Update\n");
					packetsRecv++;
					model.sendUpdate(buffer, sender.getAddress());
					break;
				case 'n': // n - object update Server-To-Client
					// printf("Incorrectly Recieved Server-Side Object Update\n");
					break;
				default:
					// printf("Unknown Command For Server: %c\n", buffer[0]);
					break;
			}

		}
		t = clock() - t;
		float timePassed = ((float)t)/CLOCKS_PER_SEC;
		printf("Packets: S:%d\t\tR:%d\n", packetsSent, packetsRecv);
  		// printf ("(%f seconds).\n",timePassed);

		wait( 0.015f - timePassed );
	}
	
	// shutdown socket layer
	
	shutdownSockets();

	return 0;
}
