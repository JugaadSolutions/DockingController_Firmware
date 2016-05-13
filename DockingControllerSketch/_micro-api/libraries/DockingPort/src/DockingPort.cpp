// 
// 
// 

#include "DockingPort.h"


DockingPort::DockingPort(int id, int sad, int reset, int serial, int baud,
	const int checkedInLed, const int processLED, const int checkedOutLED, int errorLED,
	const int lockControl)
{
	_UserReader.configure(sad, reset);

	SerialIndex = serial;


	switch (serial)
	{
	case 1:
		Serial1.begin(9600);
		break;
	case 2:
		Serial2.begin(9600);
		break;

	

	default:
		break;
	}
	

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


int  DockingPort::POST()
{
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


	_UserReader.begin();
	if (_UserReader.digitalSelfTestPass())
	{
		_UserReader.begin();
		digitalWrite(CheckedInLED, LOW);
		Serial.println("CheckedIn");
		State = CHECKED_IN;
		return State;	//TO BE CHANGED LATER

	}
	else
	{
		digitalWrite(ErrorLED, LOW);
		return PORT_ERROR;
	}

}

bool DockingPort::Service()
{
	byte status;


	switch (State)
	{
	case CHECKED_IN:
		status = _UserReader.requestTag(MF1_REQIDL, TagData);

		if (status == MI_OK)
		{
			Serial.println("Tag detected.");
			Serial.print("Type: ");
			Serial.print(TagData[0], HEX);
			Serial.print(", ");
			Serial.println(TagData[1], HEX);

			// calculate the anti-collision value for the currently detected
			// tag and write the serial into the data array.
			status = _UserReader.antiCollision(TagData);


			Serial.println("The serial nb of the tag is:");
			Serial.println(ID);
			for (int i = 0; i < 3; i++) {
				Serial.print(TagData[i], HEX);
				Serial.print(", ");
			}
			Serial.println(TagData[3], HEX);

			_UserReader.haltTag();

			digitalWrite(CheckedInLED, HIGH);
			digitalWrite(ProcessLED, LOW);




			//SendtoServer

			//SendToServer();

			PreviousState = CHECKED_IN;
			State = IN_PROCESS;
			InProcessStart = millis();

			return true;
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
	case TRANSTITION:

		if ((millis() - CheckedOutStart) > CHECKOUT_STARTUP_DELAY)	//don't read data till 
		{

			Serial.println("Transitioning to Checkout");
			State = CHECKED_OUT;
			CardIndex = 0;

			if (SerialIndex == 1)
			{
				while (Serial1.available())
				{
					Serial1.read();
				}
			}
			else if (SerialIndex == 2)
			{
				while (Serial2.available())
				{
					Serial2.read();
				}
			}
		}
		break;
	case CHECKED_OUT:

			//Serial.println("In Check out");
			switch (SerialIndex)
			{
			case 1:
				if (Serial1.available())
				{
					if (Serial1.available() && CardIndex < 12)
					{
						CardData[CardIndex++] = Serial1.read();

						Serial.print(CardData[CardIndex - 1]);

						return false;
					}

					CardData[CardIndex] = '\0';
				}
				break;
			case 2:

				if (Serial2.available())
				{
					if (CardIndex < 12)
					{
						CardData[CardIndex++] = Serial2.read();
						Serial.print(CardData[CardIndex - 1]);
						return false;
					}

					CardData[CardIndex] = '\0';
				}
				break;

			}

			if (CardIndex >= 12)
			{
				digitalWrite(CheckedOutLED, HIGH);
				digitalWrite(ProcessLED, LOW);
				
				PreviousState = CHECKED_OUT;
				InProcessStart = millis();
				State = IN_PROCESS;

				Serial.println("Checking in");

				return true;
			}
		
		break;

	default:
		break;
	}

	return false;
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
				State = TRANSTITION;
				PreviousState = STARTUP;
				CheckedOutStart = millis();
				Serial.println("Checked Out");
				digitalWrite(ProcessLED, HIGH);

				digitalWrite(LockControl, HIGH);
				delay(3500);
				digitalWrite(LockControl, LOW);

			}
			else if (PreviousState == CHECKED_OUT)	
			{
				digitalWrite(CheckedInLED, LOW);
				State = CHECKED_IN;
				PreviousState = STARTUP;
				
				Serial.println("Checked In");
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


void DockingPort::Validate()
{
	if (PreviousState == CHECKED_IN)
	{
		digitalWrite(CheckedOutLED, LOW);
		while (Serial1.available() > 0)		//clear input serial 1 buffer
		{
			Serial1.read();
		}
		State = CHECKED_OUT;

	}
	else if (PreviousState == CHECKED_OUT)	//clear input serial 2 buffer
	{
		digitalWrite(CheckedInLED, LOW);
		while (Serial2.available() > 0)
		{
			Serial2.read();
		}
		State = CHECKED_IN;
	}

	digitalWrite(ProcessLED, HIGH);


	digitalWrite(LockControl, HIGH);
	delay(2000);
	digitalWrite(LockControl, LOW);

}











