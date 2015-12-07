using UnityEngine;
using System.Collections;

public class Wiggler : MonoBehaviour {
	public float angle = 0f;

	// Use this for initialization
	void Start () {
	
	}
	
	// Update is called once per frame
	void Update () {
		angle += 0.05f;
		transform.position = new Vector3(Mathf.Abs(Mathf.Sin(angle)*4f), 0, 0);
	}
}
