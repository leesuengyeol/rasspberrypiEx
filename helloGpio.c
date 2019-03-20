#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>

#define delayTime 500 //500ms
#define loopCount 10

// ./helloGpio [PinNmber]
int main(int argc, char** argv)
{
	int gpioNo;
	int i;

	//step1. init library setup
	wiringPiSetup();

	// pin number Error
	if(argc<2)
	{
		printf("Usage : %s gpioNo\n",argv[0]);
		return -1;
	}

	gpioNo =atoi(argv[1]);
	
	//step2. Pin direction setup
	pinMode(gpioNo,OUTPUT); 
	
	for(i=0;i<loopCount; i++)
	{
		//STEP3.. pin Write
		digitalWrite(gpioNo,HIGH);
		delay(delayTime);

		digitalWrite(gpioNo,LOW);
		delay(delayTime);
	}
	return 0;
}
