#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <unistd.h>

#define loopCount 10
#define delayMax 1024

int main(int argc,char** argv)
{
	int gpioNo;
	int i,delayTime=0;
	int state=0;
	int count=0;
	
	wiringPiSetup();
	
	gpioNo = atoi (argv[1]);
	
	pinMode(gpioNo,OUTPUT);
	

	for(i=0; i<loopCount;i++)
	{
		for(count=0;count<(delayMax*2);count++)
		{
			digitalWrite(gpioNo,HIGH);
			usleep(delayTime);
			digitalWrite(gpioNo,LOW);
			usleep(delayMax - delayTime);
			if(count<delayMax)
				delayTime++;
			else 
				delayTime--;
		}





		/*
		while(1)
		{
			
			digitalWrite(gpioNo,HIGH);
			usleep(delayTime);
			digitalWrite(gpioNo,LOW);
			usleep(delayMax-delayTime);
			if(state==0) 
			{
				delayTime++;
			}
			else 
				delayTime--;
			
			if(delayTime==delayMax)
				state=1;
			
			if(delayTime==0&&state==1)
			{
				state=0;
				break;
			}
		}*/
		
		
		/*for(delayTime=delayMax;delayTime>0;delayTime--)
		{
			digitalWrite(gpioNo,HIGH);
			usleep(delayTime);
			digitalWrite(gpioNo,LOW);
			usleep(delayMax-delayTime);
		}*/
	}
}
