#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Net.h"
#include "Model.h"

using namespace std;
using namespace net;

#define TIMEOUT_TICKS 1000

int main( int argc, char * argv[] )
{
	// initialize socket layer

	if ( !InitializeSockets() )
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
	if ( !socket.Open( port ) )
	{
		printf( "failed to create socket!\n" );
		return 1;
	}

	vector<Address> addresses;

	// send and receive packets until the user ctrl-breaks...
	string handshakeInSignal = "init_client";
	string pingSignal = "ping";
	const char handshakeOutSignal[] = "init_server";
	const char timeoutSignal[] = "disconnected_timeout";

	Model model = Model();

	while ( true )
	{
		const char data[] = "hello world!\n";
		vector<Address> nextAddresses;
		for ( int i = 0; i < (int) addresses.size(); ++i ){
			printf("Sending data to peer %d t=%d\n", i, addresses[i].GetTimeout());
			if(addresses[i].GetTimeout() == 0) {
				socket.Send( addresses[i], handshakeOutSignal, sizeof( handshakeOutSignal ));
			}
			else if(addresses[i].GetTimeout() < TIMEOUT_TICKS) {
				socket.Send( addresses[i], data, sizeof( data ) );
			}
			else {
				socket.Send( addresses[i], timeoutSignal, sizeof( timeoutSignal ));
				continue;
			}
			addresses[i].IncTimeout();
			nextAddresses.push_back(addresses[i]);
		}
		addresses = nextAddresses;
			
		while ( true )
		{
			Address sender;
			unsigned char buffer[256];
			int bytes_read = socket.Receive( sender, buffer, sizeof( buffer ) );

			if ( !bytes_read )
				break;

			string message(reinterpret_cast<char*>(buffer));
			cout << "Msg " << message << "\n";

			if(std::find(addresses.begin(), addresses.end(), sender) != addresses.end()) {
			    /* v contains x */
		    	for( int j = 0; j < (int) addresses.size(); ++j) {
		    		if(addresses[j] == sender){
		    			sender = addresses[j];
		    			break;
		    		}
		    	}
		    	sender.ResetTimeout();
			    if(message == pingSignal) {
			    	cout << "Pinged by peer! " << message << "\n";
			    }
			    else {
			    	//Update model
			    	model.sendUpdate(buffer, sender.GetAddress());
			    }
			} else {
			    /* v does not contain x */
			    if(message == handshakeInSignal) {
			    	cout << "Discovered peer! " << message << "\n";
			    	addresses.push_back(sender);

			    }
			}

		
			printf( "received packet from %d.%d.%d.%d:%d (%d bytes)\n", sender.GetA(), sender.GetB(), sender.GetC(), sender.GetD(), sender.GetPort(), bytes_read );
		}
		
		wait( 1.0f );
	}
	
	// shutdown socket layer
	
	ShutdownSockets();

	return 0;
}
