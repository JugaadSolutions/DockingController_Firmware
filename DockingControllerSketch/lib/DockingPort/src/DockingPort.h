// DockingPort.h

#ifndef _DOCKINGPORT_h
#define _DOCKINGPORT_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#include "MFRC522.h"
#else
	#include "WProgram.h"
#endif

//#define _DEBUG_
#define INPROCESS_TIMEOUT	20000	
#define CHECKOUT_STARTUP_DELAY 5000	
#define LOCK_DELAY				3500
#define CYCLE_READ_TIMEOUT	7500	
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

	  unsigned long InProcessStart;
	  unsigned long CheckedOutStart;
	  unsigned long lockStart;
	  unsigned long cycleReadStart;

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
	byte TagData[MAX_LEN];
	

	DockingPort();

	DockingPort(int id, int sad_u, int sad_c, int reset,
		 const int checkedInLed, const int processLED, const int checkedOutLED,int ErrorLED,
		const int lockControl);

	int Service();
	int 	POST();

	bool SendToServer();
	void Validate();
	void HandleServerData(String data);
};

#endif

