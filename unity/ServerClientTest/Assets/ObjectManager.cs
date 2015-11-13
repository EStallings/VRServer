using UnityEngine; 
using System.IO;
using System.IO.Compression;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;
using System.Collections;
using System.Collections.Generic;

class ObjectWrapper {
	Transform myObj;
	Vector3 lastPos;
	Quaternion lastRot;

	private static int posLength = 92;
	private static int rotLength = 103;

	public ObjectWrapper(Transform t){
		lastPos = t.position;
		lastRot = t.rotation;
		myObj = t;
	}

	public bool Updated(){
		if(lastPos != myObj.position || lastRot != myObj.rotation) {
			lastPos = myObj.position;
			lastRot = myObj.rotation;
			return true;
		}
		return false;
	}
	
	public byte[] GetPacketData(BinaryFormatter bf){
		MemoryStream ms = new MemoryStream();
		bf.Serialize(ms, lastPos);
		bf.Serialize(ms, lastRot);

		return ms.ToArray();
	}
	
	private static System.Object DeserializeObj(byte[] data, BinaryFormatter bf) {
		using (var ms = new MemoryStream(data))
		{
			return bf.Deserialize(ms);
		}
	}

	public void LoadFromPacketData(byte[] data, BinaryFormatter bf) {
		byte[] posPart = data.SubArray(0, posLength);
		byte[] rotPart = data.SubArray(posLength, rotLength);
		Vector3 posD = (Vector3)DeserializeObj(posPart, bf);
		Quaternion rotD = (Quaternion)DeserializeObj(rotPart, bf);

		// Debug.Log((posD == lastPos) + " and " + (rotD == lastRot));
	}
}

public class ObjectManager : MonoBehaviour {
	public Transform[] watchObjects;
	public client2 client;

	public int masterLocalID = 0;

	private SurrogateSelector ss;
	private MemoryStream ms;
	private BinaryFormatter bf;
	

	Dictionary<int, ObjectWrapper> objectWrappersWithLocalID; // unsynced with server, awaiting a foreign ID
	Dictionary<int, ObjectWrapper> objectWrappersWithForeignID;

	// Use this for initialization
	void Start () {
		// Set up our serialization objects - a formatter and the SurrogateSelector
		bf = new BinaryFormatter();
		ss = new SurrogateSelector();
		Vector3SerializationSurrogate v3ss = new Vector3SerializationSurrogate();
		QuaternionSerializationSurrogate qss = new QuaternionSerializationSurrogate();
		ss.AddSurrogate(typeof(Vector3), 
						new StreamingContext(StreamingContextStates.All), 
						v3ss);
		ss.AddSurrogate(typeof(Quaternion), 
						new StreamingContext(StreamingContextStates.All), 
						qss);
		bf.SurrogateSelector = ss;

		// Set up maps for object wrappers
		objectWrappersWithLocalID = new Dictionary<int, ObjectWrapper>();
		objectWrappersWithForeignID = new Dictionary<int, ObjectWrapper>();

		// Process the statically-assigned objects.
		for(int i = 0; i < watchObjects.Length; i++) {
			objectWrappersWithLocalID.Add(i, new ObjectWrapper(watchObjects[i]));
		}
	}
	
	// Update is called once per frame
	void Update () {
		foreach(ObjectWrapper ow in objectWrappersWithLocalID) {
			if(ow.Updated()){
				//Make packet, add to client to-send list
			}
			byte[] data = ow.GetPacketData(bf);
			print("Object Serialized Size: " + data.Length);
			ow.LoadFromPacketData(data, bf);
		}
	}

	void RecieveUpdate(byte[] packet, int objId) {

	}

	void RecieveId(int localId, int foreignId) {

	}

	void AddNewItem(Transform t) {

	}
}
