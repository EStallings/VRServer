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

	StateObject() {
		objectID = SOID++;
		lastUpdatedIP = 0;
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

	//Returns true if we need to resend the object to all listeners
	// params: newdata is the packet data, ip is the id of the peer that sent the packet
	void sendUpdate(unsigned char* newData, int ip) {

		//If timestamp is negative, then we have a new, NON-SYSTEM-WIDE object on our hands!
		int timestamp = (int)newData[0];
		int id = (int)newData[4];
		bool updateObject = false;
		map<int, StateObject>::iterator it = stateObjects.find(id);
		StateObject stateObject;

		//Whether or not we need to ping back to client the global ID
		bool needToInform = false;
		
		if(it == stateObjects.end()) {
			// Does not exist in map. Add it?
			// Note and MASSIVE caveat:
			// In a full-sized system, we would need to ping the server to get an ID back to use for an object.
			// However, we'll just assume that all clients have the same local ids, and use the ID they provide us with.
			// TODO: Make this FLEXIBLE.
			stateObject = StateObject();
			if(timestamp < 0){
				//This is a new object, and we need to send back a global id.
				id = stateObject.objectID;
			}else {
				//Trust that this is a global object - something that comes with the scene.
				stateObject.objectID = id;
			}
			stateObjects[id] = stateObject;
			updateObject = true;
		}
		else {
			// Object found. Check timestamp
			stateObject = it->second;
			if(stateObject.lastUpdatedTimestamp < timestamp)
				updateObject = true;
		}

		if(updateObject){
			//If we have to, update the object.
			stateObject.update(timestamp, ip, newData);
			updatedIds.push_back(stateObject.objectID);
		}
	}

	// Fills a StateObject* with a pointer to the StateObject specified by <id>
	StateObject* getObject(StateObject* fill, int id){
		map<int, StateObject>::iterator it = stateObjects.find(id);
		if(it == stateObjects.end()) {
			fill = NULL;
		}
		else {
			// Object found. Check timestamp
			StateObject stateObject = it->second;
			fill = &stateObject;
		}
		return fill;
	}
};