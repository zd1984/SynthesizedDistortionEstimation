// loss_vs.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include<stdlib.h>
#include<string.h>

#define NUM_ROPE_PMF     4
#define STR_SIZE         256

#define SAVE_SSE_TO_FILE 0

#if SAVE_SSE_TO_FILE
#define OUTPUT_FILE_NAME "seq_sse.bin"
#endif

/*returned file size is in byte*/
int sizeDetection(char *fileName)
{
   char buffer[1024];
   unsigned int fileSize=0;
   unsigned int byteRead=0;
   FILE *sourceFile;


   sourceFile = fopen(fileName,"rb");
   if(sourceFile == NULL)
   {
      printf("ERROR reading source file\n");
      return 0;
   }


   while(1)
   {
      byteRead = fread(buffer,sizeof(char),1024,sourceFile);
      fileSize = fileSize + byteRead;
      if(byteRead<1024)
      {
         break;
      }
   }

   fclose(sourceFile);

   return fileSize;
}

#define MAX_NUMBER_OF_SLICES 15000
unsigned char *sliceLocations[MAX_NUMBER_OF_SLICES];
unsigned int sliceSizes[MAX_NUMBER_OF_SLICES];
unsigned int totalSlices;
unsigned int totalVCLSlices;
unsigned int totalNonVCLSlices;
void findSliceLocations(unsigned char *buffer, unsigned int size)
{
	unsigned int i;
	totalSlices = 0;
	totalVCLSlices = 0;
	totalNonVCLSlices = 0;
	for(i=0; i<size; i++)
	{
		unsigned char NALUtype;
		if(buffer[i]==0 && buffer[i+1]==0 && buffer[i+2]==1)
		{
			sliceLocations[totalSlices] = &buffer[i];
			totalSlices++;
			if(totalSlices>=MAX_NUMBER_OF_SLICES)
			{
				printf("Error total number of slices exceeds max\n");
				exit(0);
			}

			NALUtype = buffer[i+3]&0x1F;
			if(NALUtype==1)
			{
				totalVCLSlices++;
			}
			else if(NALUtype==5)
			{	
				totalVCLSlices++;
			}
			else if(NALUtype==7)
			{
				totalNonVCLSlices++;
			}
			else if(NALUtype==8)
			{
				totalNonVCLSlices++;
			}
			else
			{
				printf("Error unsupported NAL unit %d\n",NALUtype);
			}

			i=i+2;
		}
		else if(buffer[i]==0 && buffer[i+1]==0 && buffer[i+2]==0 && buffer[i+3]==1)
		{
			sliceLocations[totalSlices] = &buffer[i];
			totalSlices++;
			if(totalSlices>=MAX_NUMBER_OF_SLICES)
			{
				printf("Error total number of slices exceeds max\n");
				exit(0);
			}

			NALUtype = buffer[i+4]&0x1F;
			if(NALUtype==1)
			{
				totalVCLSlices++;
			}
			else if(NALUtype==5)
			{	
				totalVCLSlices++;
			}
			else if(NALUtype==7)
			{
				totalNonVCLSlices++;
			}
			else if(NALUtype==8)
			{
				totalNonVCLSlices++;
			}
			else
			{
				printf("Error unsupported NAL unit %d\n",NALUtype);
			}

			i=i+3;
		}
		else
		{
		}
	}

	for(i=0; i<totalSlices-1; i++)
	{
		if((unsigned int)sliceLocations[i+1] <= (unsigned int)sliceLocations[i])
		{
			printf("Error next slice should have a higher address\n");
			exit(0);
		}
		sliceSizes[i] = (unsigned int)(sliceLocations[i+1] - sliceLocations[i]);
	}
	sliceSizes[totalSlices-1] = size - (unsigned int)(sliceLocations[totalSlices-1] - sliceLocations[0]);
}
void dropSlices(unsigned char *outputBuffer, unsigned char *inputBuffer, unsigned int lossRate, unsigned int slicesPerFrame, unsigned int *outputSize, unsigned int inputFrames)
{
	unsigned int i;
	int nextSlicePosition;

	unsigned char NALUtype;
	unsigned int detectedframes;
	unsigned int VCLsliceCounter;
	unsigned int dropCount;

	/*error checking*/
	if((totalVCLSlices%slicesPerFrame)!=0)
	{
		printf("Error slicesPerFrame:%d totalVCLslices:%d\n",slicesPerFrame,totalVCLSlices);
		exit(0);
	}
	detectedframes = totalVCLSlices/slicesPerFrame;
	if(inputFrames!=detectedframes)
	{
		printf("Number of frames mismatch %d frames detected %d frames specified\n",detectedframes,inputFrames);
	}
	
	VCLsliceCounter=0;
	nextSlicePosition = 0;
	*outputSize = 0;
	dropCount = 0;
	for(i=0; i<totalSlices; i++)
	{
		unsigned char *slicep = sliceLocations[i];

		/*get the NAL unit type*/
		if(slicep[0]==0 && slicep[1]==0 && slicep[2]==1)
		{
			NALUtype = slicep[3]&0x1F;
		}
		else if(slicep[0]==0 && slicep[1]==0 && slicep[2]==0 && slicep[3]==1)
		{
			NALUtype = slicep[4]&0x1F;
		}
		else
		{
			printf("Error start code is missing\n");
			exit(0);
		}

		/*count the VCL slices*/
		if(NALUtype==1)
		{
			VCLsliceCounter++;
		}
		else if(NALUtype==5)
		{	
			VCLsliceCounter++;
		}
		else if(NALUtype==7)
		{
		}
		else if(NALUtype==8)
		{
		}
		else
		{
			printf("Error unsupported NAL unit %d\n",NALUtype);
		}


		/*drop the slices*/
		if( (NALUtype==1) || (NALUtype==5) )
		{
			if((VCLsliceCounter>slicesPerFrame) && (VCLsliceCounter<=(detectedframes-1)*slicesPerFrame))
			{
				if( rand() >= ((RAND_MAX/100)*lossRate) )/*do not drop*/
				//if(VCLsliceCounter%2==1)//debug dropping every other slice
				{
					memcpy(&outputBuffer[nextSlicePosition], slicep, sliceSizes[i]);
					nextSlicePosition = nextSlicePosition + sliceSizes[i];
					*outputSize = *outputSize + sliceSizes[i];
				}
				else
				{
					dropCount++;
				}
			}
			else/*do not drop the first frame and last frame*/
			{
				memcpy(&outputBuffer[nextSlicePosition], slicep, sliceSizes[i]);
				nextSlicePosition = nextSlicePosition + sliceSizes[i];
				*outputSize = *outputSize + sliceSizes[i];
			}
		}
		else/*do not drop SPS or PPS*/
		{
			memcpy(&outputBuffer[nextSlicePosition], slicep, sliceSizes[i]);
			nextSlicePosition = nextSlicePosition + sliceSizes[i];
			*outputSize = *outputSize + sliceSizes[i];
		}
	}
	printf("%d slices in total %d dropped\n",totalVCLSlices,dropCount);
}

void dropSlicesInSequence(char *inputFileName, char *outputFileName, unsigned int lossRate, unsigned int VCLslicesPerFrame, unsigned int frames)
{
	unsigned int inputFileSize;
	unsigned int outputFileSize;
	unsigned char *inputData;
	unsigned char *outputData;

	FILE *inputFile;
	FILE *outputFile;

	/*grab the input data*/
	inputFileSize = sizeDetection(inputFileName);
	inputData = (unsigned char *)malloc(inputFileSize);
	if(inputData==NULL)
	{
		printf("Error can't allocate memory\n");
	}
	inputFile = fopen(inputFileName, "rb");
    if(inputFile == NULL)
    {
       printf("ERROR can't open input file\n");
       exit(0);
    }
	fread(inputData, inputFileSize, 1, inputFile);
	fclose(inputFile);


	/*remove NAL units*/
	/*make the output buffer as big as the input buffer*/
	outputData = (unsigned char *)malloc(inputFileSize);
	if(outputData==NULL)
	{
		printf("Error can't allocate memory\n");
	}
	findSliceLocations(inputData, inputFileSize);
	dropSlices(outputData, inputData, lossRate, VCLslicesPerFrame, &outputFileSize, frames);


	/*save the output file*/
    outputFile = fopen(outputFileName, "wb");
    if(outputFile == NULL)
    {
       printf("ERROR can't open output file\n");
       exit(0);
    }
	fwrite(outputData, outputFileSize, 1, outputFile);
	fclose(outputFile);

	free(inputData);
	free(outputData);
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

void main(int argc, char* argv[])
{
	int i;
	int iter;
	char decConfigFiles[NUM_ROPE_PMF][STR_SIZE];
	char intactStreamNames[NUM_ROPE_PMF][STR_SIZE];
	char lossyStreamNames[NUM_ROPE_PMF][STR_SIZE];

	char decArgs[NUM_ROPE_PMF][STR_SIZE];
	char vsArg[STR_SIZE];

	int iterations;
	int width;
	int height;
	int frames;
	unsigned int lossRate;
	int slicesPerFrame;
	int seedValue;

	FILE *configFile;/*decoder configuration file*/


	FILE *synthView;
	FILE *refSynthView;

	FILE *randSeedFile;

#if SAVE_SSE_TO_FILE
	FILE *SSEDataFile;
#endif

    unsigned char *synthFrameBuffer;
    unsigned char *refSynthFrameBuffer;
	unsigned int *tdistortionBufferPP;
	__int64 *tdistortionBufferPF;

	if(argc!=13)
	{
		printf("loss_vs [DecoderConfigFile] [vsConfig] [iterations] [width] [height] [frames] [synthView] [refSynthView] [lossRate] [synthAppName] [slicesPerFrame] [seed]\n");
		return;
	}

	iterations = atoi(argv[3]);
	width = atoi(argv[4]);
	height = atoi(argv[5]);
	frames = atoi(argv[6]);
	lossRate = atoi(argv[9]);
	slicesPerFrame = atoi(argv[11]);
	seedValue = atoi(argv[12]);

	/*process the config file for the decoder application*/
	configFile = fopen(argv[1],"rt");
	if(!configFile)
	{
		printf("Error opening config file %s\n",argv[1]);
		return;
	}
	/*get the decoder config files*/
	for(i=0; i<NUM_ROPE_PMF; i++)
	{
		fgets(decConfigFiles[i], STR_SIZE, configFile);
		removeCR(decConfigFiles[i], STR_SIZE);
	}
	/*get the intact H.264 stream names*/
	for(i=0; i<NUM_ROPE_PMF; i++)
	{
		fgets(intactStreamNames[i], STR_SIZE, configFile);
		removeCR(intactStreamNames[i], STR_SIZE);
	}
	/*get the lossy H.264 stream names*/
	for(i=0; i<NUM_ROPE_PMF; i++)
	{
		fgets(lossyStreamNames[i], STR_SIZE, configFile);
		removeCR(lossyStreamNames[i], STR_SIZE);
	}
	fclose(configFile);


	/*allocate the distortion data buffer per pixel and per frame*/
	tdistortionBufferPP = (unsigned int *)malloc(sizeof(unsigned int)*width*height*frames);
	memset(tdistortionBufferPP, 0, sizeof(unsigned int)*width*height*frames);

	tdistortionBufferPF = (__int64 *)malloc(sizeof(__int64)*frames);
	memset(tdistortionBufferPF, 0, sizeof(__int64)*frames);
	
	/*allocate pixel buffer for synthesized view and reference view*/
	synthFrameBuffer = (unsigned char *)malloc((width*height*3)>>1);
	refSynthFrameBuffer = (unsigned char *)malloc((width*height*3)>>1);



	/*get the decoder arguments ready*/
	for(i=0; i<NUM_ROPE_PMF; i++)
	{
		sprintf(decArgs[i],"ldecod.exe -f %s", decConfigFiles[i]);
	}

	/*get the argument ready for the view synthesizer*/
	sprintf(vsArg, "%s %s", argv[10], argv[2]);

	srand(seedValue);
	/*launch the applications*/
	for(iter=0; iter<iterations; iter++)
	{
		int j;
		int k;
		int ret;
		STARTUPINFO si[NUM_ROPE_PMF];
		PROCESS_INFORMATION pi[NUM_ROPE_PMF];
		TCHAR temp[STR_SIZE];


		/*introduce packet loss to the bitstreams*/
		printf("Dropping NALU\n");
		for(i=0; i<NUM_ROPE_PMF; i++)
		{
			dropSlicesInSequence(intactStreamNames[i], lossyStreamNames[i], lossRate, slicesPerFrame, frames);
		}

		printf("Launching the decoders\n");
		for(i=0; i<NUM_ROPE_PMF; i++)
		{
			/*lauch the decoders in parallel*/
			ZeroMemory( &si[i], sizeof(si[i]) );
			si[i].cb = sizeof(si[i]);
			ZeroMemory( &pi[i], sizeof(pi[i]) );

			charToTCHAR(temp, decArgs[i]);
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
				exit(0);
			}
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
		printf("Decoders finished start view synthesis\n");
		/*Run the view synthesizer*/
		ret = system(vsArg);
		if(ret!=0)
		{
			printf("Error launching %s\n",vsArg);
			exit(0);
		}
		printf("View synthesis finished\n");



		/*accumulate SSE for each pixel*/
		synthView = fopen(argv[7], "rb");
		if(synthView==NULL)
		{
			printf("Can't open synthesized view file %s\n", argv[7]);
		}

		refSynthView = fopen(argv[8], "rb");
		if(refSynthView==NULL)
		{
			printf("Can't open reference synthesized view file %s\n", argv[8]);
		}

		/*going through all the frames to find SSE at each pixel*/
		for(j=0; j<frames; j++)
		{
			unsigned int SSEthisFrame=0;

			fread(synthFrameBuffer,sizeof(unsigned char),(width*height*3)>>1,synthView);
			fread(refSynthFrameBuffer,sizeof(unsigned char),(width*height*3)>>1,refSynthView);

			for(k=0; k<width*height; k++)
			{
				unsigned int difference=0;
				if(synthFrameBuffer[k]>=refSynthFrameBuffer[k])
				{
					difference = (unsigned int)(synthFrameBuffer[k]-refSynthFrameBuffer[k]);
				}
				else
				{
					difference = (unsigned int)(refSynthFrameBuffer[k]-synthFrameBuffer[k]);
				}
				SSEthisFrame = SSEthisFrame + difference*difference;

				/*accumulate sse for each pixel*/
				tdistortionBufferPP[j*width*height+k] = tdistortionBufferPP[j*width*height+k] + difference*difference;
			}
			
			/*accumulate sse for each frame*/
			tdistortionBufferPF[j] = tdistortionBufferPF[j] + (__int64)SSEthisFrame;
		}

		fclose(synthView);
		fclose(refSynthView);
	}
	
	/*do an error checking on data collection*/
	{
		int j;
		int k;
		for(j=0; j<frames; j++)
		{
			__int64 frameSSE=0;
			for(k=0; k<width*height; k++)
			{
				frameSSE = frameSSE + (__int64)tdistortionBufferPP[j*width*height+k];
			}
			if(tdistortionBufferPF[j]!=frameSSE)
			{
				printf("Error total per pixel SSE does not add up to total per frame SSE frame:%u %I64d %I64d\n",j ,tdistortionBufferPF[j] ,frameSSE);
			}
		}
	}
	
	/*calculate the average*/
	for(i=0; i<frames; i++)
	{
		int j;
		tdistortionBufferPF[i] = tdistortionBufferPF[i]/iterations;
		printf("Frame:%u SSE:%I64d\n",i,tdistortionBufferPF[i]);
		for(j=0; j<width*height; j++)
		{
			 tdistortionBufferPP[i*width*height+j] = tdistortionBufferPP[i*width*height+j]/iterations;
		}
	}

#if SAVE_SSE_TO_FILE
	/*write the binary data out*/
	SSEDataFile = fopen(OUTPUT_FILE_NAME,"wb");
	if(SSEDataFile==NULL)
	{
		printf("Can't open output SSE data file for writing %s\n",OUTPUT_FILE_NAME);
	}
    fwrite(tdistortionBufferPP, 1, sizeof(unsigned int)*width*height*frames, SSEDataFile);
	fclose(SSEDataFile);
#endif
	free(tdistortionBufferPP);
	free(tdistortionBufferPF);
	free(synthFrameBuffer);
	free(refSynthFrameBuffer);

	return;
}

