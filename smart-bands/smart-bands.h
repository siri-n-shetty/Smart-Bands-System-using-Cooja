#ifndef PRELOADED_KEY
	#define PRELOADED_KEY "aaaaaaaaaaaaaaaaaaaa"
#endif

#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "string.h"
#include "stdbool.h"

#include <stdio.h>
#define DIM_KEY 20


// Definition of Static Variables //
static char key[DIM_KEY] = PRELOADED_KEY;

static struct ctimer timer;
static bool pairing = true;
static bool unicastReceived = false;
static bool operationMode = false;
static bool parent = false;
static bool messageArrived = false;
static linkaddr_t* addr;

struct position_t{
	int x;
	int y;
};

static struct position_t childPosition;

static void callbackMissing(void *ptr)
 {
   ctimer_reset(&timer);
   printf("ALARM: MISSING last position -> x = %d, y = %d\n", childPosition.x, childPosition.y);

 }

enum STATUS{
	STANDING,
	WALKING,
	RUNNING,
	FALLING
	};
	
static inline const char *toString(enum STATUS s){
    static const char *strings[] = { "STANDING", "WALKING",	"RUNNING", "FALLING" };

    return strings[s];
}

static struct message_t{
	struct position_t position;
	enum STATUS status;
} message;

// Processes Definitions //
PROCESS(key_generation_process, "Key generation");
PROCESS(Parent_bands_process, "Parent bands");
PROCESS(Child_bands_process, "Child bands");

//--------------------------------- KEY GENERATION ----------------------------------

// Generates 20 random printable characters
PROCESS_THREAD(key_generation_process, ev, data)
{
  PROCESS_BEGIN();

  static int i;
  
	for(i = 0; i<DIM_KEY; i++){
		key[i] = random_rand()%95 + 32;
	}
	
	printf("%s \n", key);

  PROCESS_END();
}

/*------------------------------- BROADCAST --------------------------------------------*/

// This method manages the reception of broadcast messages
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
	// Enter the method if we are in Pairing Phase and a Unicast has not been received yet
	if(unicastReceived || !pairing)
		return;	
	
	// If the message matches the key, it stops Pairing Phase and saves sender's address
	if(strcmp((char *)packetbuf_dataptr(), key) == 0){
		printf("Paired device -> key found, Address: %d.%d\n", from->u8[0], from->u8[1]);
		pairing = false;
		linkaddr_copy(addr, from);
	}
	else
		printf("This (%s) is NOT my key\n", (char *)packetbuf_dataptr());
		
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;


// ------------------------------ UNICAST -------------------------------------- //

// This method manages the reception of a Unicast Message
static void recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
	
	// If in Pairing Phase and the received message is a Stop Pairing
	// It ends the Pairing Phase and saves the address of the sender
	if(pairing && strcmp((char *)packetbuf_dataptr(), "Stop Pairing") == 0){
		printf("Paired device -> unicast message received, Address: %d.%d\n", from->u8[0], from->u8[1]);
		pairing = false;
		unicastReceived = true;
		linkaddr_copy(addr, from);
	}
	
	// If the Parent mote is in Operation Mode 
	// Checks if the address is the same of the children
	// In that case saves the child's position and checks the Status
	// If the status is FALLING, fires a FALL ALARM
	else if (operationMode && parent){
		if (linkaddr_cmp(from, addr)){
			messageArrived = true;
			printf("A message arrived from the child\n");
			message = *((struct message_t*) packetbuf_dataptr());
			printf("x = %d, y = %d, status = %s\n", message.position.x, message.position.y, toString(message.status));
			childPosition = message.position;
			if(message.status == FALLING){
				printf("ALARM: FALL x = %d, y = %d\n", message.position.x, message.position.y);
			}
			//If a message arrive restart the timer 
			ctimer_restart(&timer);
		}
	}
}

static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;

/*---------------------------------- MESSAGES -----------------------------------------*/

// This method sends a Stop Pairing message if the mote hasn't already received one
static void sendStopPairing(){
	if(!unicastReceived){
		packetbuf_copyfrom("Stop Pairing", 13);
		unicast_send(&uc, addr);
		printf("Message unicast sent\n");
	}
}

// This Method sends a broadcast message containing the key
static void sendPairingMessage(){
    
    packetbuf_copyfrom(key, 21);
    broadcast_send(&broadcast);
    printf("Broadcast message sent: %s\n", key);
	
}

// This method generates and sends as Unicast the INFO message
static void sendInfo(){
	
	// X and Y coordinates are random values
	message.position.x = random_rand();
	message.position.y = random_rand();
	
	// Status is a random variable following the 0.3, 0.3, 0.3 and 0.1 probability distribution
	int rand = random_rand()%100;
	if (rand < 30)       //  30%
    	message.status = 0;
	else if (rand < 60)  //  30%
    	message.status = 1;
	else if (rand < 90)  //  30%
    	message.status = 2;
	else				 //	 10%
    	message.status = 3;
	
	
	packetbuf_copyfrom(&message, sizeof(struct message_t));
    unicast_send(&uc, addr);
    printf("Unicast message sent: x = %d, y = %d, status = %s\n", message.position.x, message.position.y, toString(message.status));
}


/*--------------------------- PARENT ------------------------------------------------*/

PROCESS_THREAD(Parent_bands_process, ev, data)
{
  static struct etimer et;
  
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();
	
	// Start Broadcast connection
  	parent = true;
  	broadcast_open(&broadcast, 129, &broadcast_call);
  	
	// Starts Unicast Connection
  	unicast_open(&uc, 146, &unicast_callbacks);
  	
	// Sends Pairing Message every 2 seconds
	while(pairing){
		sendPairingMessage();
		/* Delay 2 seconds */
    	etimer_set(&et, CLOCK_SECOND * 2);
    	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	}
	
	//Switch from Pairing to Operation Mode
	broadcast_close(&broadcast);
	printf("Closed broadcast connection\n");
	
	sendStopPairing();
	
	printf("Start op mode\n");
	operationMode = true;


	// In Operation Mode, if the mote doesn't receive the unicast INFO message within 60 seconds, it fires a MISSING ALARM
	ctimer_set(&timer, CLOCK_SECOND * 60, callbackMissing, NULL);
	while(1){
		PROCESS_WAIT_EVENT();
	}

	
  PROCESS_END();
}




//---------------------------- CHILD -----------------------------------------------

PROCESS_THREAD(Child_bands_process, ev, data)
{
  static struct etimer et;
  
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();
	
	parent = false;

	// Start Broadcast connection
  	broadcast_open(&broadcast, 129, &broadcast_call);
  	
	// Start Unicast connection
  	unicast_open(&uc, 146, &unicast_callbacks);
  	
	// Sends Pairing Message every 2 seconds
	while(pairing){
	
		sendPairingMessage();
		/* Delay 2 seconds */
    	etimer_set(&et, CLOCK_SECOND * 2);
    	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

	}
	
	//Switch from Pairing to Operation Mode
	broadcast_close(&broadcast);
	printf("Closed broadcast connection\n");
	
	sendStopPairing();
	
	printf("Start op mode\n");
	operationMode = true;


	// In Operation Mode sends a Unicast message containing the INFO every 10 seconds
	while(1){
		etimer_set(&et, CLOCK_SECOND * 10);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		sendInfo();
	}

  PROCESS_END();
}

