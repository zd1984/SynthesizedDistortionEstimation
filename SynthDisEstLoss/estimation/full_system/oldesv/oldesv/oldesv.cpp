// oldesv.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define MSG_NO_MESSAGE      0
#define MSG_START_FRAME     1
#define MSG_FRAME_FINISHED  2
#define MSG_SEQUENCE_FINISHED  3

#define STR_SIZE 256
#define NUM_ROPE_PMF 4

#define MAX_NUM_FRAMES 300
double frameDistortion[MAX_NUM_FRAMES];
FILE *frameDistortionFile;

void removeCR(char *input, int size)
{
	int i;
	for(i=0; i<size; i++)
	{
		if(input[i]==10 || input[i]==13)//LF or CR
		{
			input[i]=0;
			break;
		}
	}
}

void charToTCHAR(TCHAR output[STR_SIZE], char input[STR_SIZE])
{
   int i=0;
   while(1)
   {
      if(input[i] == 0)
      {
         break;
      }
      output[i] = input[i];
      i++;
   }

   output[i] = 0;
}

void msgSend(HANDLE hSlot, int *msg, int msgSize)
{
   BOOL fResult; 
   DWORD cbWritten; 
 
   fResult = WriteFile(hSlot, 
     msg, 
     (DWORD) msgSize,
     &cbWritten, 
     (LPOVERLAPPED) NULL); 
 
   if (!fResult) 
   { 
      printf("WriteFile failed with %d.\n", GetLastError()); 
      exit(0); 
   }
}

int msgRead(HANDLE hSlot)
{
	TCHAR eventNameTCHAR[STR_SIZE];
	char eventName[STR_SIZE];

    DWORD cbMessage, cMessage, cbRead; 
    BOOL fResult; 
    int receivedMsg;

    HANDLE hEvent;
    OVERLAPPED ov;
 
    cbMessage = cMessage = cbRead = 0; 

	sprintf(eventName,"%d_event",GetCurrentProcessId());
	charToTCHAR(eventNameTCHAR, eventName);
    hEvent = CreateEvent(NULL, FALSE, FALSE, eventNameTCHAR);
    if( NULL == hEvent )
        return FALSE;
    ov.Offset = 0;
    ov.OffsetHigh = 0;
    ov.hEvent = hEvent;
 
    fResult = GetMailslotInfo(hSlot, // mailslot handle 
        (LPDWORD) NULL,               // no maximum message size 
        &cbMessage,                   // size of next message 
        &cMessage,                    // number of messages 
        (LPDWORD) NULL);              // no read time-out 
    if (!fResult) 
    { 
        printf("GetMailslotInfo failed with %d.\n", GetLastError()); 
        exit(0);
    } 
 
    if (cbMessage == MAILSLOT_NO_MESSAGE) 
    { 
        /*no message in the mail box*/
        return MSG_NO_MESSAGE; 
    } 
	
	/*with the way we set up the mail box, there should only be 1 message waiting*/
    if(cMessage!=1)
	{
		printf("ERROR messaging system is not synchronized\n");
		exit(0);
	}
    if(cbMessage!=4)
	{
		printf("ERROR message size is incorrect\n");
		exit(0);
	}
 
    /*retrieve the message*/
    fResult = ReadFile(hSlot, 
        &receivedMsg, 
        cbMessage, 
        &cbRead, 
        &ov); 
    if (!fResult) 
    { 
        printf("ReadFile failed with %d.\n", GetLastError());
        exit(0);
    } 

    CloseHandle(hEvent);

    return receivedMsg; 
}

int main(int argc, char * argv[])
{
	int i;

	double lossRate;

	TCHAR temp[STR_SIZE]; //used for conversion from char to TCHAR whenever is needed
	char  tempChar[STR_SIZE];

	HANDLE hFileMailBoxOut[NUM_ROPE_PMF];/*handles for mail boxes so that we can communicate with child proesses*/
	HANDLE hFileMailBoxIn[NUM_ROPE_PMF];

	STARTUPINFO si[NUM_ROPE_PMF];
	PROCESS_INFORMATION pi[NUM_ROPE_PMF];

	char ropePMFConfigFiles[NUM_ROPE_PMF][STR_SIZE];
	char sharedMemNames[NUM_ROPE_PMF][STR_SIZE];
	char mailBoxNamesOut[NUM_ROPE_PMF][STR_SIZE];
	char mailBoxNamesIn[NUM_ROPE_PMF][STR_SIZE];
	char VSconfigFileName[STR_SIZE];

	char cmdLine[NUM_ROPE_PMF][STR_SIZE];
	char cmdLineVS[STR_SIZE];

	FILE *configFile;

	int frameCounter=0;

	if(argc!=3)
	{
		printf("oldesv.exe [configFile] [lossRAte]");
		return -1;
	}

	/*get the loss rate*/
	lossRate = atof(argv[2]);

	/*open the config file for reading*/
	configFile = fopen(argv[1],"rt");
	if(!configFile)
	{
		printf("Error opening config file %s\n",argv[1]);
		return -1;
	}
	/*get the config file names*/
	for(i=0; i<NUM_ROPE_PMF; i++)
	{
		fgets(ropePMFConfigFiles[i], STR_SIZE, configFile);
		removeCR(ropePMFConfigFiles[i], STR_SIZE);
	}
	/*get the share memory names*/
	for(i=0; i<NUM_ROPE_PMF; i++)
	{
		fgets(sharedMemNames[i], STR_SIZE, configFile);
		removeCR(sharedMemNames[i], STR_SIZE);
	}
	/*get the mail box names*/
	for(i=0; i<NUM_ROPE_PMF; i++)
	{
		fgets(mailBoxNamesOut[i], STR_SIZE, configFile);
		removeCR(mailBoxNamesOut[i], STR_SIZE);
		sprintf(tempChar, "\\\\.\\mailslot\\%s", mailBoxNamesOut[i]);
		sprintf(mailBoxNamesOut[i], "%s", tempChar);

		fgets(mailBoxNamesIn[i], STR_SIZE, configFile);
		removeCR(mailBoxNamesIn[i], STR_SIZE);
		sprintf(tempChar, "\\\\.\\mailslot\\%s", mailBoxNamesIn[i]);
		sprintf(mailBoxNamesIn[i], "%s", tempChar);
	}
	/*get the VS config file name*/
	fgets(VSconfigFileName, STR_SIZE, configFile);
	removeCR(VSconfigFileName, STR_SIZE);
	/*close the config file*/
	fclose(configFile);
	
	/*create the incoming mail boxes first before launching the child processes*/
	for(i=0; i<NUM_ROPE_PMF; i++)
	{
		/*create the incoming mail boxes*/
		charToTCHAR(temp, mailBoxNamesIn[i]);
		printf("Main process(%d): incoming mail box:%s\n",GetCurrentProcessId(),mailBoxNamesIn[i]);//debug
		hFileMailBoxIn[i] = CreateMailslot(temp, 
			0,                             // no maximum message size 
			MAILSLOT_WAIT_FOREVER,         // no time-out for operations 
			(LPSECURITY_ATTRIBUTES) NULL); // default security
 
		if (hFileMailBoxIn[i] == INVALID_HANDLE_VALUE) 
		{ 
			printf("CreateMailslot failed with %d\n", GetLastError());
			return -1; 
		}
	}

	/*get the command line ready*/
	for(i=0; i<NUM_ROPE_PMF; i++)
	{
		/*the first two encoders are for the texture*/
		if(i<=1)
		{
			sprintf(cmdLine[i],"lencod_t.exe -f %s %f %s %s %s", ropePMFConfigFiles[i], lossRate, sharedMemNames[i], mailBoxNamesOut[i], mailBoxNamesIn[i]);
		}
		else
		{
			sprintf(cmdLine[i],"lencod_d.exe -f %s %f %s %s %s", ropePMFConfigFiles[i], lossRate, sharedMemNames[i], mailBoxNamesOut[i], mailBoxNamesIn[i]);
		}
	}

	/*launch the processes*/
	for(i=0; i<NUM_ROPE_PMF; i++)
	{
		ZeroMemory( &si[i], sizeof(si[i]) );
		si[i].cb = sizeof(si[i]);
		ZeroMemory( &pi[i], sizeof(pi[i]) );
		charToTCHAR(temp, cmdLine[i]);
		// Start the child process. 
		if( !CreateProcess( NULL,   // No module name (use command line)
			temp,        // Command line
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			0,              // No creation flags
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si[i],            // Pointer to STARTUPINFO structure
			&pi[i] )           // Pointer to PROCESS_INFORMATION structure
		) 
		{
			printf( "CreateProcess failed (%d).\n", GetLastError() );
			return -1;
		}
	}
	printf("Main process(%d): child processes launched\n",GetCurrentProcessId());


	/*give it some time for the child processes to create the mail boxes*/
	Sleep(6000);
	for(i=0; i<NUM_ROPE_PMF; i++)
	{
		/*create the outgoing mail boxes*/
		charToTCHAR(temp, mailBoxNamesOut[i]);
		printf("Main process(%d): outgoing mail box:%s\n",GetCurrentProcessId(),mailBoxNamesOut[i]);//debug
		hFileMailBoxOut[i] = CreateFile(temp, 
		 GENERIC_WRITE, 
		 FILE_SHARE_READ,
		 (LPSECURITY_ATTRIBUTES) NULL, 
		 OPEN_EXISTING, 
		 FILE_ATTRIBUTE_NORMAL, 
		 (HANDLE) NULL); 
		
		if (hFileMailBoxOut[i] == INVALID_HANDLE_VALUE) 
	    { 
		   printf("CreateFile failed with %d.\n", GetLastError()); 
		   return -1; 
	    }
	}
	printf("Main process(%d): outgoing mail boxes linked\n",GetCurrentProcessId());


	/*main task loop*/
	while(1)
	{
		int frameFinishFlag=0;
		int sequenceFinishFlag=0;
		int message;

		//Sleep(5000);//debug
		//printf("Main process(%d): child processes kicked wait for 5 seconds and then start checking for return messages........\n",GetCurrentProcessId());//debug
		//printf("\nPress enter for next frame\n");//debug
		//getchar();//debug

		/*kick the child processes going*/
		message = MSG_START_FRAME;
		for(i=0; i<NUM_ROPE_PMF; i++)
		{
			msgSend(hFileMailBoxOut[i], &message, sizeof(int));
		}

		/*get responses from the child processes*/
		while(1)
		{
			for(i=0; i<NUM_ROPE_PMF; i++)
			{
				message = msgRead(hFileMailBoxIn[i]);
				if(message==MSG_FRAME_FINISHED)
				{
					frameFinishFlag++;
				}
				else if(message==MSG_SEQUENCE_FINISHED)
				{
					sequenceFinishFlag++;
				}
				else if(message==MSG_NO_MESSAGE)
				{
				}
				else
				{
					printf("Unexpected message\n");
					exit(0);
				}
			}

			/*if all child processes have finished then we exit otherwise we wait for 200ms and then check again*/
			if((frameFinishFlag==NUM_ROPE_PMF) || (sequenceFinishFlag==NUM_ROPE_PMF))
			{
				printf("Main process(%d): frame finish or sequence finish\n",GetCurrentProcessId());//debug
				break;
			}
			Sleep(200);
		}
		if(sequenceFinishFlag==NUM_ROPE_PMF)
		{
			printf("Main process(%d): sequence finish\n",GetCurrentProcessId());//debug
			break;
		}
		else
		{
			int ret;
			/*view synthesizer can be launched using a blocking system call*/
			/*view synthesizer just need to run for a single frame*/
			sprintf(cmdLineVS,"ViewSynVC8d.exe %s %d %s %s %s %s", VSconfigFileName, frameCounter, sharedMemNames[0], sharedMemNames[1], sharedMemNames[2], sharedMemNames[3]);
			ret = system(cmdLineVS);
			if(ret!=0)
			{
				printf("Error lanching the view synthesizer\n");
				exit(0);
			}

			/*open the distortion file and save the distortion*/
			frameDistortionFile = fopen("singleFrameDistortionFile.bin","rb");
			if(frameDistortionFile == NULL)
			{
				printf("Error opening distortion file\n");
				exit(0);
			}
			fread(&frameDistortion[frameCounter],sizeof(double),1,frameDistortionFile);
			fclose(frameDistortionFile);
		}
		frameCounter++;
	}


	/*wait until the child processes finish*/
	for(i=0; i<NUM_ROPE_PMF; i++)
	{
		// Wait until child process exits.
		WaitForSingleObject( pi[i].hProcess, INFINITE );
		// Close process and thread handles. 
		CloseHandle( pi[i].hProcess );
		CloseHandle( pi[i].hThread );
	}
	printf("Main process(%d): Child processes finished\n",GetCurrentProcessId());

	/*close the handles for the mail boxes*/
	for(i=0; i<NUM_ROPE_PMF; i++)
	{
		CloseHandle(hFileMailBoxOut[i]);
		CloseHandle(hFileMailBoxIn[i]);
	}
	printf("Main process(%d): mail boxes closed\n",GetCurrentProcessId());
	
	/*print the distortions*/
	for(i=0; i<frameCounter; i++)
	{
		printf("Frame:%d distortion:%f\n",i,frameDistortion[i]);
	}
	return 0;
}

