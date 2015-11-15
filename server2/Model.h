#include <vector>
#include <map>

using namespace std;

#define OBJ_PACK_LENGTH 255

int SOID = 0;

// Responsible for keeping the object state for a single object in our 'world' model
class StateObject {
public:
	int lastUpdatedTimestamp;
	int lastUpdatedIP;
	int objectID;
	unsigned char data[OBJ_PACK_LENGTH];
	int timesUpdated;

	StateObject() {
		objectID = SOID++;
		lastUpdatedIP = 0;
		timesUpdated = 0;
		lastUpdatedTimestamp = 0;
	}

	// Updates a StateObject - someone's client has updated this data and we need to reflect that here
	// timestamp - most recent update-tick
	// ip - the server-side ID for the user that sent the update
	// newdata - our new data! yay!
	void update(int timestamp, int ip, unsigned char* newData) {
		int i;
		for(i = 0; i < OBJ_PACK_LENGTH; i++) {
			data[i] = newData[i];
		}
		lastUpdatedTimestamp = timestamp;
		lastUpdatedIP = ip;
		timesUpdated++;
		printf("Set ip: %d, tu: %d\n", lastUpdatedIP, timesUpdated);
	}

	bool operator < ( const StateObject & other ) const
	{
		return objectID < other.objectID;
	}
};

// Maintains all the StateObjects that our 'world' model must contain
class Model {
private:
	map<int, StateObject> stateObjects;
	vector<int> updatedIds;
public:
	
	vector<int> getUpdatedIds(){
		return updatedIds;
	}

	map<int, StateObject> getStateObjects(){
		return stateObjects;
	}

	// params: newdata is the packet data, ip is the id of the peer that sent the packet
	void sendUpdate(unsigned char* newData, int ip) {
		int timestamp = (int)newData[1];
		int id = (int)newData[5];
		
		if(stateObjects.find(id) == stateObjects.end()) {
			printf("Object not found in model");
			return;
		}
		// printf("comparing %d < %d\n", stateObjects[id].lastUpdatedTimestamp, timestamp);
		//If the update is more recent than the last update...
		if(stateObjects[id].lastUpdatedTimestamp <= timestamp){
			// printf("Updating Object: id: %d, t: %d, ip:%d\n", id, timestamp, ip);
			stateObjects[id].update(timestamp, ip, newData);
			updatedIds.push_back(stateObjects[id].objectID);
			// printf("   IP after update: %d\n", stateObjects[id].lastUpdatedIP);
		}
	}

	// Initialize a global object - something that comes with the scene.
	void initializeGlobal(unsigned char* newData, int ip) {
		int timestamp = 0;
		int id = (int)newData[5];
		//Check that the object hasn't already been initialized
		if(stateObjects.find(id) != stateObjects.end()){
			return; //the object has already been initialized.
		}

		//Create object and add it to map.
		stateObjects[id] = StateObject();
		stateObjects[id].objectID = id;

		// printf("ID / IP / TU after init: %d / %d / %d\n", id, stateObjects[id].lastUpdatedIP, stateObjects[id].timesUpdated);
		
		//Need to give the actual data to the object.
		stateObjects[id].update(timestamp, ip, newData);

		// Since all global objects have same starting conditions, we can just skip updating other users
	}

	// Initialize a local object - something that one of the clients made that needs to be initialized on other clients
	// returns new, global ID to use for object.
	bool initializeLocal(unsigned char* newData, int ip) {
		int timestamp = 0;

		//Create object and add it to map.
		StateObject stateObject = StateObject();
		int id = stateObject.objectID;
		stateObjects[id] = stateObject;

		//Give data to object.
		stateObject.update(timestamp, ip, newData);

		//We do need to update because other users don't have this object
		updatedIds.push_back(id);
		
		return id;
	}

	void resetIdsToUpdate() {
		vector<int> cleared;
		updatedIds = cleared;
	}

};