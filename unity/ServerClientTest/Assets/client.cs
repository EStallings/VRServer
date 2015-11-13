using UnityEngine;
using System.Collections;
using UnityEngine.Networking;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;

public class client : MonoBehaviour {

	int mServerSocket = -1;
	int mClientSocket = -1;
	int mClientConnection = -1;
	int mMaxConnections = 10;
	byte mChannelUnreliable;
	byte mChannelReliable;
	bool mHostClientInitialized = false;
	bool mClientConnected = false;

	int port = 3490;
	int ourPort = 7777;

	// Use this for initialization
	void Start () {

		// Build global config
		GlobalConfig gc = new GlobalConfig();
		gc.ReactorModel = ReactorModel.FixRateReactor;
		gc.ThreadAwakeTimeout = 10;
		
		// Build channel config
		ConnectionConfig cc = new ConnectionConfig();
		mChannelReliable = cc.AddChannel (QosType.ReliableSequenced);
		mChannelUnreliable = cc.AddChannel(QosType.UnreliableSequenced);
		
		// Create host topology from config
		HostTopology ht = new HostTopology( cc , mMaxConnections  );

		// We have all of our other stuff figured out, so init
		NetworkTransport.Init(gc);

		// Open sockets for server and client
		mServerSocket = NetworkTransport.AddHost ( ht , ourPort  );
		mClientSocket = NetworkTransport.AddHost ( ht  );

		// Check to make sure our socket formations were successful
		if( mServerSocket < 0 ){ Debug.Log ("Server socket creation failed!"); } else { Debug.Log ("Server socket creation successful!"); }
		if( mClientSocket < 0 ){ Debug.Log ("Client socket creation failed!"); } else { Debug.Log ("Client socket creation successful!"); }

		mHostClientInitialized = true;

		// Connect to server
		byte error;
		mClientConnection = NetworkTransport.Connect( mClientSocket , "127.0.0.1" , port , 0 , out error );
		
		LogNetworkError( error );

	
	}

	void SendTestData(){
		// Send the server a message
		byte error;
		byte[] buffer = new byte[1024];
		Stream stream = new MemoryStream(buffer);
		BinaryFormatter f = new BinaryFormatter();
		f.Serialize ( stream , "Hello!" );
		
		NetworkTransport.Send ( mClientSocket , mClientConnection , mChannelReliable , buffer , (int)stream.Position , out error );
		
		LogNetworkError ( error );
	}

	/// <summary>
	/// Log any network errors to the console.
	/// </summary>
	/// <param name="error">Error.</param>
	void LogNetworkError(byte error){
		if( error != (byte)NetworkError.Ok){
			NetworkError nerror = (NetworkError)error;
			Debug.Log ("Error: " + nerror.ToString ());
		}
	}


	// Update is called once per frame
	void Update () {

		if(!mHostClientInitialized){
			return;
		}

		int recHostId; 
		int connectionId;
		int channelId;
		int dataSize;
		byte[] buffer = new byte[1024];
		byte error;
		
		NetworkEventType networkEvent = NetworkEventType.DataEvent;
		
		// Poll both server/client events
		do
		{
			networkEvent = NetworkTransport.Receive( out recHostId , out connectionId , out channelId , buffer , 1024 , out dataSize , out error );
			
			switch(networkEvent){
			case NetworkEventType.Nothing:
				break;
			case NetworkEventType.ConnectEvent:
				// Server received disconnect event
				if( recHostId == mServerSocket ){
					Debug.Log ("Server: Player " + connectionId.ToString () + " connected!" );
				}
				if( recHostId == mClientSocket ){
					Debug.Log ("Client: Client connected to " + connectionId.ToString () + "!" );

					// Set our flag to let client know that they can start sending data and send some data
					mClientConnected = true; 
					SendTestData ();
				}

				break;
			
			case NetworkEventType.DataEvent:
				// Server received data
				if( recHostId == mServerSocket ){

					// Let's decode data
					Stream stream = new MemoryStream(buffer);
					BinaryFormatter f = new BinaryFormatter();
					string msg = f.Deserialize( stream ).ToString ();

					Debug.Log ("Server: Received Data from " + connectionId.ToString () + "! Message: " + msg );
				}
				if( recHostId == mClientSocket ){
					Debug.Log ("Client: Received Data from " + connectionId.ToString () + "!" );
				}
				break;

			case NetworkEventType.DisconnectEvent:
				// Client received disconnect event
				if( recHostId == mClientSocket && connectionId == mClientConnection ){
					Debug.Log ("Client: Disconnected from server!");

					// Flag to let client know it can no longer send data
					mClientConnected = false;
				}

				// Server received disconnect event
				if( recHostId == mServerSocket ){
					Debug.Log ("Server: Received disconnect from " + connectionId.ToString () );
				}
				break;
			}
			
		} while ( networkEvent != NetworkEventType.Nothing );
	}
	
}


