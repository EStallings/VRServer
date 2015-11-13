
int main(void){
    // create socket

    const int port = 30000;
    Socket socket;
    if ( !socket.Open( port ) )
    {
        printf( "failed to create socket!\n" );
        return false;
    }

    // send a packet

    const char data[] = "hello world!";
    socket.Send( Address(127,0,0,1,port), data, sizeof( data ) );

    // receive packets

    while ( true )
    {
        Address sender;
        unsigned char buffer[256];
        int bytes_read = 
            socket.Receive( sender, 
                            buffer, 
                            sizeof( buffer ) );
        if ( !bytes_read )
            break;
     
        // process packet
    }

}