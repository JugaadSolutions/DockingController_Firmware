// 
// 
// 

#include "DockingPort_Mifare.h"

/*_____________________________________________________________________________________
DockingPort	constructor 

Ports{PORTS_NO} = {port id , sad for user reader , sad for cycle reader, reset for readers
                   checked Led , process Led , checkedout Led, error Led ,lock controller}
________________________________________________________________________________________*/

DockingPort::DockingPort(int id, int sad_u,int sad_c, int reset, 
	const int checkedInLed, const int processLED, const int checkedOutLED, int errorLED,
	const int lockControl)
{
	_UserReader.configure(sad_u, reset);
	_CycleReader.configure(sad_c, reset);
	
	

	pinMode(checkedInLed, OUTPUT);
	digitalWrite(checkedInLed, HIGH);	//off for common cathode

	pinMode(processLED, OUTPUT);
	digitalWrite(processLED, HIGH);

	pinMode(checkedOutLED, OUTPUT);
	digitalWrite(checkedOutLED, HIGH);

	pinMode(errorLED, OUTPUT);
	digitalWrite(errorLED, HIGH);

	pinMode(lockControl, OUTPUT);
	digitalWrite(lockControl, LOW);


	CheckedInLED = checkedInLed;
	ProcessLED = processLED;
	CheckedOutLED = checkedOutLED;
	ErrorLED = errorLED;


	LockControl = lockControl;

	ID = id;
	State = STARTUP;
	PreviousState = STARTUP;

}
/*_______________________________________________
int  DockingPort::POST()
summery : selftest all leds and  take satus of Readers
		  accoring results of reader it set state 
		  
input   :none
retutrn :state id
output  :
___________________________________________________*/
int  DockingPort::POST()
{
	byte status;
	
	//self test of all leds
	digitalWrite(CheckedInLED, LOW);
	delay(100);

	digitalWrite(ProcessLED, LOW);
	delay(100);

	digitalWrite(CheckedOutLED, LOW);
	delay(100);

	digitalWrite(ErrorLED, LOW);
	delay(100);



	digitalWrite(CheckedInLED, HIGH);
	delay(100);

	digitalWrite(ProcessLED, HIGH);
	delay(100);

	digitalWrite(CheckedOutLED, HIGH);
	delay(100);

	digitalWrite(ErrorLED, HIGH);
	delay(100);
	
	CheckedInLock = false;
	CheckedOutLock = false;
	cycleStaus  = false;
	

	//function for begin user Reader.
	_UserReader.begin();
	
	//function for begin cycle Reader.
	_CycleReader.begin();
	
	//self test for user reader
	if (_UserReader.digitalSelfTestPass())
	{
		_UserReader.begin();
		//self test for user reader
		if (_CycleReader.digitalSelfTestPass())
		{
			_CycleReader.begin();
			
			//status for checking if there is any tag 
			status = _CycleReader.requestTag(MF1_REQIDL, CycleTagData);

			if (status == MI_OK)
			{
				Serial.println("Tag detected.");
				Serial.print("Type: ");
				Serial.print(CycleTagData[0], HEX);
				Serial.print(", ");
				Serial.println(CycleTagData[1], HEX);

				// calculate the anti-collision value for the currently detected
				// tag and write the serial into the data array.
				status = _CycleReader.antiCollision(CycleTagData);


				Serial.println("The serial nb of the tag is:");
				Serial.println(ID);
				for (int i = 0; i < 3; i++) {
					Serial.print(CycleTagData[i], HEX);
					Serial.print(", ");
				}
				Serial.println(CycleTagData[3], HEX);

				_CycleReader.haltTag();	

				//change the led status and set the state
				
				digitalWrite(CheckedInLED, LOW);
				Serial.println("CheckedIn");
				State = CHECKED_IN;
				PreviousState = STARTUP;
				return State;	//TO BE CHANGED LATER					
			}
			else
			{

				//change the led status and set the state
				digitalWrite(CheckedOutLED, LOW);
				Serial.println("CheckedOut");
				State = CHECKED_OUT;
				return State;	//TO BE CHANGED LATER			
			}
		}
		else
		{
			digitalWrite(ErrorLED, LOW);
			return PORT_ERROR;
		}


	}
	else
	{
		digitalWrite(ErrorLED, LOW);
		return PORT_ERROR;
	}

}
/*______________________________________________________________
bool DockingPort::Service();
summery	:

input	:none
output	:
________________________________________________________________*/

uint8_t DockingPort::Service()
{
	byte status;

	switch (State)
	{
	case CHECKED_IN:
	
		if(CheckedOutLock == true)
		{
			if ((millis() - lockStart) > LOCK_DELAY)
			{
				digitalWrite(LockControl, LOW);
				CheckedOutLock = false;
				Serial.println("Lock end");
				
				cycleStaus = true;
			}
			
		}
		
		else if(cycleStaus == true)
		{
			//Serial.println("check for cycle");
			status = _CycleReader.requestTag(MF1_REQIDL, CycleTagData);
			if (status == MI_OK)
			{
				
				Serial.println("Tag detected.");
				Serial.print("Type: ");
				Serial.print(CycleTagData[0], HEX);
				Serial.print(", ");
				Serial.println(CycleTagData[1], HEX);

				// calculate the anti-collision value for the currently detected
				// tag and write the serial into the data array.
				status = _CycleReader.antiCollision(CycleTagData);


				Serial.println("The serial nb of the tag is:");
				Serial.println(ID);
				for (int i = 0; i < 3; i++) {
					Serial.print(CycleTagData[i], HEX);
					Serial.print(", ");
				}
				Serial.println(CycleTagData[3], HEX);

				_CycleReader.haltTag();
				cycleStaus  = false;
				return 0;
			}
			
		}
		
		else
		{
			status = _UserReader.requestTag(MF1_REQIDL, UserTagData);

			if (status == MI_OK)
			{
				Serial.println("Tag detected.");
				Serial.print("Type: ");
				Serial.print(UserTagData[0], HEX);
				Serial.print(", ");
				Serial.println(UserTagData[1], HEX);

				// calculate the anti-collision value for the currently detected
				// tag and write the serial into the data array.
				status = _UserReader.antiCollision(UserTagData);


				Serial.println("The serial nb of the tag is:");
				Serial.println(ID);
				for (int i = 0; i < 3; i++) {
					Serial.print(UserTagData[i], HEX);
					Serial.print(", ");
				}
				Serial.println(UserTagData[3], HEX);

				_UserReader.haltTag();

				digitalWrite(CheckedInLED, HIGH);
				digitalWrite(ProcessLED, LOW);


				PreviousState = CHECKED_IN;
				State = IN_PROCESS;
				InProcessStart = millis();

				return 1;

				
			}
		}

		
		/*

		else
		{
			Serial.print(ID);
			Serial.println("ND");
		}
		*/
		break;

	case IN_PROCESS:
	
		if ((millis() - InProcessStart) > INPROCESS_TIMEOUT)	//event timeout go back to previous state
		{
			if (PreviousState == CHECKED_IN)
			{
				digitalWrite(CheckedInLED, LOW);
				State = CHECKED_IN;

			}
			else if (PreviousState == CHECKED_OUT)
			{
				digitalWrite(CheckedOutLED, LOW);
				State = CHECKED_OUT;
				

			}

			digitalWrite(ProcessLED, HIGH);
		}
		break;
	
	case CHECKED_OUT:

			//Serial.println("In Check out");
			
		if(CheckedInLock == true)
		{
			if ((millis() - lockStart) > LOCK_DELAY)
			{
				digitalWrite(LockControl, LOW);
				CheckedInLock = false;
				Serial.println("Lock end");				
			}
		}
		else
		{			
			status = _CycleReader.requestTag(MF1_REQIDL, CycleTagData);

			if (status == MI_OK)
			{
				Serial.println("Tag detected.");
				Serial.print("Type: ");
				Serial.print(CycleTagData[0], HEX);
				Serial.print(", ");
				Serial.println(CycleTagData[1], HEX);

				// calculate the anti-collision value for the currently detected
				// tag and write the serial into the data array.
				status = _CycleReader.antiCollision(CycleTagData);


				Serial.println("The serial nb of the tag is:");
				Serial.println(ID);
				for (int i = 0; i < 3; i++) {
					Serial.print(CycleTagData[i], HEX);
					Serial.print(", ");
				}
				Serial.println(CycleTagData[3], HEX);

				_CycleReader.haltTag();
				
				digitalWrite(CheckedOutLED, HIGH);
				digitalWrite(ProcessLED, LOW);

				

				PreviousState = CHECKED_OUT;
				State = IN_PROCESS;
				InProcessStart = millis();
			
				return 3;

			}
		}
		
		/*

		else
		{
			Serial.print(ID);
			Serial.println("ND");
		}
		*/

		break;

		default:
		break;
	}
	

	return 0xFF;
}

void DockingPort::HandleServerData(String data)
{
	switch (State)
	{
	case  IN_PROCESS:
		if (data.indexOf('Y', 0) == -1)	// invalid 
		{
			Serial.println("Invalid");
			if (PreviousState == CHECKED_IN)
			{
				digitalWrite(CheckedInLED, LOW);
				State = CHECKED_IN;
				PreviousState = STARTUP;

				
			}
			else if (PreviousState == CHECKED_OUT)
			{
				digitalWrite(CheckedOutLED, LOW);
				State = CHECKED_OUT;
				PreviousState = STARTUP;

			}

			digitalWrite(ProcessLED, HIGH);


		}
		else
		{
			if (PreviousState == CHECKED_IN)
			{
				digitalWrite(CheckedOutLED, LOW);
				State = CHECKED_OUT;
				PreviousState = STARTUP;
				lockStart = millis();
				Serial.println("Checked Out");
				digitalWrite(ProcessLED, HIGH);
				digitalWrite(LockControl, HIGH);
				CheckedInLock = true;
				//Serial.println("lock start");
				//delay(3500);
				//digitalWrite(LockControl, LOW);

			}
			else if (PreviousState == CHECKED_OUT)	
			{
				digitalWrite(CheckedInLED, LOW);
				State = CHECKED_IN;
				PreviousState = STARTUP;
				lockStart = millis();
				Serial.println("Checked In");
				
				digitalWrite(ProcessLED, HIGH);
				//digitalWrite(LockControl, HIGH);
				CheckedOutLock = true;
				
			}

			digitalWrite(ProcessLED, HIGH);

			

		}
		break;

	case  CHECKED_OUT:
		if (data.indexOf('I', 0) == -1)	// invalid 
		{
			Serial.println("Invalid");
			return;
		}
		
		else
		{
			
			
				digitalWrite(CheckedOutLED, HIGH);
				digitalWrite(CheckedInLED, LOW);
				State = CHECKED_IN;
				PreviousState = STARTUP;

				Serial.println("Checked In");
			



		}
		break;

	default:
		break;
	}
}









