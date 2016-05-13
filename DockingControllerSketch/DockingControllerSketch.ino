#include "DockingPort_Mifare.h"
#include <SPI.h>
#include <MFRC522.h>

#define PORTS_NO  3
#define RECONNECT_TIME   10000
enum {
	IDLE = 0,
	CIPSTART = 1,
	CIPSEND = 2
};

enum
{
	CMD_CIPSTART = 1,
	CMD_CIPSEND
};

int State;
unsigned long connectTime_start;

/*_____________________________________________________________________________________
DockingPort	constructor

Ports{PORTS_NO} = {port id , sad for user reader , sad for cycle reader, reset for readers
checked Led , process Led , checkedout Led, error Led ,lock controller}
________________________________________________________________________________________*/

//DockingPort	 Ports[2] = { { 1, 6, 10, 1, 9600, 41, 39, 37, 35, 25 }, { 2, 7, 10, 2, 9600, 33, 31, 29, 27, 23 } };
//DockingPort	 Ports[2] = { { 4, 6, 10, 2, 9600, 33, 31, 29, 27, 25 }, { 3, 7, 10, 1, 9600, 41, 39, 37, 35, 23 } };
DockingPort	 Ports[PORTS_NO] = { { 1, 7, 6, 10, 22, 24, 26, 28, 30 },
{ 2, 5, 4, 10, 32, 34, 36, 38, 40 },
{ 3, 3, 2, 10, 23, 25, 27, 29, 31 },
//{ 4, A7, A8, 10, 38, 39, 40, 41, 7}
};

#define DOCKING_UNIT_NO 1
#define PORT_START_INDEX 0

void setup()
{
	//intialize all communication
	SPI.begin();
	Serial.begin(9600);
	Serial3.begin(9600);
	delay(1000);

	//checking for port status
	Serial.println("Starting Ports..");
	delay(1000);


	//connection establish to server
	Serial3.println("ATE0");
	delay(1000);
	//Serial3.println("AT+CIPSTART=\"TCP\",\"192.168.1.100\",6868");
	//delay(1000);


	while (true)
	{
		ReconnectServer();
		uint8_t connectionStatus = checkConnectionStatus();
		if (connectionStatus == 3)
			break;
	}

	//int stateStatus = 0xFF;
	for (int i = 0; i < PORTS_NO; i++)
	{
		Serial.print("Port-");
		Serial.print(i);

		//stateStatus = Ports[i].POST();
		if (Ports[i].POST() != 0xFF)
		{

			Serial.println(" Started");

		}
		else
		{
			Serial.println(" Failed");
		}
	}
	/* add setup code here */
	State = IDLE;

	

	connectTime_start = millis();

}

void loop()
{
	uint8_t result;
	//Serial.flush();
	//Serial3.flush(); 
	if (millis() - connectTime_start > RECONNECT_TIME)
	{
		//Serial.println("reconnect");
		//ReconnectServer();
		connectTime_start = millis();
	}

	String data;
	for (int i = 0; i < PORTS_NO; i++)
	{
		result = Ports[i].Service();
		if (result != 0xFF)
		{
			Serial.flush();
                        Serial.print("result=");
                        Serial.print(result,DEC);
			Serial.println("Commmunicating with Server");
			SendToServer(i, result);
		}

		if (Serial3.available())
		{
			data = Serial3.readStringUntil('\n');

			//Serial.println(data);

			if (data.startsWith("+IPD,"))
			{
				int portIndexStart = data.indexOf(':', 0);
				int portIndexEnd = data.indexOf('|', 0);


				//Serial.print("Port Start:");
				//Serial.println(portIndexStart);

				//Serial.print("Port End:");
				//Serial.println(portIndexEnd);

				String portStr = data.substring(portIndexStart + 1, portIndexEnd);
				//Serial.print("Port Str:");
				//Serial.println(portStr);


				int portIndex = atoi(portStr.c_str());

				//Serial.print("Port:");
				//Serial.println(portIndex);
				for (int i = 0; i < PORTS_NO; i++)
				{
					if (Ports[i].ID == portIndex)
					{
						Ports[i].HandleServerData(data.substring(portIndexEnd + 1));
					}
				}
			}
		}

	}



}

void SendToServer(int index, int result)
{
	
	if (result == 1)
	{
		Serial3.println("AT+CIPSEND=11");
		delay(1000);
		Serial3.write(DOCKING_UNIT_NO);			//location
		Serial3.write((byte)Ports[index].ID + PORT_START_INDEX);		//port
		Serial3.write(0x01);			//01 - checkout operation
		for (int i = 0; i < 4; i++)
		{
			Serial3.write(Ports[index].UserTagData[i]);

		}
		for (int i = 0; i < 4; i++)
		{
			Serial3.write(Ports[index].CycleTagData[i]);

		}
	}
	else if (result == 3)
	{
		Serial3.println("AT+CIPSEND=7");
		delay(1000);
		Serial3.write(DOCKING_UNIT_NO);			//location
		Serial3.write((byte)Ports[index].ID + PORT_START_INDEX);		//port
		Serial3.write(0x02);			//02 - checkin operation
		// Serial.println(index,HEX);
		for (int i = 0; i < 4; i++)
		{
			Serial3.write(Ports[index].CycleTagData[i]);
			// Serial.println(Ports[index].TagData[i],HEX);

		}
	}
	else if (result == 0)
	{
		Serial3.println("AT+CIPSEND=7");
		delay(1000);
		Serial3.write(DOCKING_UNIT_NO);			//location
		Serial3.write((byte)Ports[index].ID + PORT_START_INDEX);		//port
		Serial3.write(0x00);			//00 - cycle status
		// Serial.println(index,HEX);
		for (int i = 0; i < 4; i++)
		{
			Serial3.write(Ports[index].CycleTagData[i]);
			// Serial.println(Ports[index].TagData[i],HEX);

		}
	}

	//delay(1000);
	//Serial3.println("AT+CIPCLOSE");
}

uint8_t checkConnectionStatus()
{
	Serial3.println("AT+CIPSTATUS");
	delay(3000);

	while (!Serial3.available());
	while (Serial3.available())
	{
		String data = Serial3.readStringUntil('\n');
		//Serial.println(data);
		if (data.startsWith("STATUS:"))
		{
			return data[7] - '0';
		}
	}
}
void  ReconnectServer()
{
	while (Serial3.available())
	{
		Serial3.read();
	}
	Serial3.flush();


	Serial3.println("AT+CIPSTART=\"TCP\",\"192.168.0.100\",6868");
	delay(3000);

}
