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
class DockingPort
{
private:
	 MFRC522 _UserReader;
	 int	 SerialIndex;
	  int	 CheckedInLED;
	  int	ProcessLED;
	  int	CheckedOutLED;
	  int	ErrorLED;
	  int	LockControl;
	  int	CardIndex;

	  unsigned long InProcessStart;
	  unsigned long CheckedOutStart;

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
	char CardData[20];
	

	DockingPort();

	DockingPort(int id, int sad, int reset, int serial, int baud,
		 const int checkedInLed, const int processLED, const int checkedOutLED,int ErrorLED,
		const int lockControl);

	bool Service();
	int 	POST();

	bool SendToServer();
	void Validate();
	void HandleServerData(String data);
};

#endif

