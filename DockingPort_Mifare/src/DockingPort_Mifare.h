// DockingPort.h

#ifndef _DOCKINGPORT_h
#define _DOCKINGPORT_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#include "MFRC522.h"
#else
	#include "WProgram.h"
#endif

#define INPROCESS_TIMEOUT	30000	
#define CHECKOUT_STARTUP_DELAY 5000	
#define LOCK_DELAY				3500
class DockingPort
{
private:
	 MFRC522 _UserReader;
	 MFRC522 _CycleReader;
	 int	 SerialIndex;
	  int	 CheckedInLED;
	  int	ProcessLED;
	  int	CheckedOutLED;
	  int	ErrorLED;
	  int	LockControl;
	  int	CardIndex;
	  bool  CheckedInLock;
	  bool  CheckedOutLock;
	  bool  cycleStaus;

	  unsigned long InProcessStart;
	  unsigned long CheckedOutStart;
	  unsigned long lockStart;

public:
	typedef enum
	{
		STARTUP = 0,
		CHECKED_IN = 1,
		IN_PROCESS = 2,
		CHECKED_OUT = 3,
		TRANSTITION =4,
		PORT_ERROR = 0XFF

	}PORT_STATE;

	PORT_STATE State;
	PORT_STATE	PreviousState;
	int ID;
	byte UserTagData[MAX_LEN];
	byte CycleTagData[MAX_LEN];
	

	DockingPort();

	DockingPort(int id, int sad_u, int sad_c, int reset,
		 const int checkedInLed, const int processLED, const int checkedOutLED,int ErrorLED,
		const int lockControl);

	uint8_t Service();
	int 	POST();

	bool SendToServer();
	void Validate();
	void HandleServerData(String data);
};

#endif

