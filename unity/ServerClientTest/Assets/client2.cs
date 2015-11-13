using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Text;
using UnityEngine;
using System.Collections;

// State object for receiving data from remote device.
public class StateObject {
	// Client socket.
	public Socket workSocket = null;
	// Size of receive buffer.
	public const int BufferSize = 256;
	// Receive buffer.
	public byte[] buffer = new byte[BufferSize];
	// Received data string.
	public StringBuilder sb = new StringBuilder();
}

public class client2 : MonoBehaviour {
	// Server IP
	public string hostIP = "127.0.0.1";
	public string handshakeOutSignal = "init_client";
	public string handshakeInSignal = "init_server";
	public string pingSignal = "ping";
	public string timeoutSignal = "disconnected_timeout";

	// The port number for the remote device.
	public int targetPort = 30000;
	public int listenPort = 3450;

	// Modes
	public bool mode_strict_incoming_IP = true; //Ensure incoming packets ONLY come from server
	public bool mode_strict_incoming_PORT = true; //Ensure incoming packets ONLY come from server
	public bool mode_manual_ping = true;
	public int ping_after_ticks = 100;

	public bool sig_handshake0 = false; //We've notified the server
	public bool sig_handshake1 = false; //Recieved confirmation message
	
	public bool sig_incomingWaiting = false; //we have a new message waiting for processing
	public bool sig_outgoingWaiting = false; //we have a message waiting to go out

	// The response from the remote device.
	public string response = String.Empty;
	public byte[] rawResponse;

	public int ticksSinceLastSend = 0;

	UdpClient listener = null;
	IPEndPoint groupEP = null;
	IPEndPoint remoteEP = null;
	void Connect() {
		print("Starting Client");
		// Connect to a remote device.
		try {

			listener = new UdpClient(listenPort);
			groupEP = new IPEndPoint(IPAddress.Any, listenPort);

			// Establish the remote endpoint for the socket.
			IPHostEntry ipHostInfo = Dns.GetHostEntry(hostIP);
			IPAddress ipAddress = ipHostInfo.AddressList[0];
			remoteEP = new IPEndPoint(ipAddress, targetPort);

		} catch (Exception e) {	print(e.ToString()); }
	}

	void Kill() {
		try {
			// client.Close();
			listener.Close();
		} catch (Exception e) {	print(e.ToString()); }
	}

	void Receive() {
		try {
			if(listener.Available > 0){
				rawResponse = listener.Receive(ref groupEP);
				response = Encoding.ASCII.GetString(rawResponse, 0, rawResponse.Length);
				if(rawResponse.Length > 0){
					sig_incomingWaiting = true;
				}
			}

		} catch (Exception e) {	print(e.ToString()); }	
	}

	bool Send(String data) {
		try {
			// Convert the string data to byte data using ASCII encoding.
			byte[] byteData = Encoding.ASCII.GetBytes(data);

			// Begin sending the data to the remote device.
			listener.Send(byteData, byteData.Length, remoteEP);
			print("Sent " + byteData.Length + " bytes to server.");
			sig_outgoingWaiting = false;
			ticksSinceLastSend = 0;
			return true;
		} catch (Exception e) {	print(e.ToString()); }
		return false;
	}

	void ProcessIncoming() {
		if((groupEP.Address.ToString() != hostIP && mode_strict_incoming_IP) || (groupEP.Port != targetPort && mode_strict_incoming_PORT)) {
			print("ERROR");
			return;
		}
		if(!sig_handshake1){
			if(response.Length == handshakeInSignal.Length+1 && String.Compare(response.Substring(0, handshakeInSignal.Length), handshakeInSignal) == 0){
				print("HANDSHAKE WITH SERVER COMPLETED");
				sig_handshake1 = true;
			}
		}
		if(response.Length == timeoutSignal.Length+1 && String.Compare(response.Substring(0, timeoutSignal.Length), timeoutSignal) == 0){
			print("DISCONNECTED FROM SERVER, RECONNECTING...");
			sig_handshake1 = false;
			sig_handshake0 = false;
		}
		print("Received a broadcast from " + groupEP.Address.ToString() + " port " + groupEP.Port + " msg: " + response);

		sig_incomingWaiting = false;
		response = String.Empty;
		rawResponse = null;
	}

	void ProcessOutgoing() {
		print("Processing Outgoing");
	}


	void Start() {
		Connect();
	}

	void Update() {
		ticksSinceLastSend++;
		if(sig_incomingWaiting) {
			ProcessIncoming();
		}
		Receive();

		
		if(!sig_handshake0){
			sig_handshake0 = Send(handshakeOutSignal);
		}
		else if(sig_outgoingWaiting) {
			ProcessOutgoing();
		}
		else if(ticksSinceLastSend > ping_after_ticks && mode_manual_ping){
			Send(pingSignal);
		}
	}
}