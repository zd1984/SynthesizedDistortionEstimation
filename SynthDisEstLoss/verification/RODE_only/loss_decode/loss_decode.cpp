// loss_vs.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include<stdlib.h>
#include<string.h>




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

#define MAX_NUMBER_OF_SLICES 8192
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








void main(int argc, char* argv[])
{
	int i;
	unsigned int randSeed;
	int iterations;
	char decArg[256];
	int width;
	int height;
	int frames;
	unsigned int lossRate;

	FILE *decodedSeq;
	FILE *referenceSeq;

	unsigned int slicesPerFrame;

    unsigned char *decodedFrameBuffer;
    unsigned char *referenceFrameBuffer;
	unsigned int *tdistortionBufferPP;
	__int64 *tdistortionBufferPF;


	if(argc!=13)
	{
		printf("loss_decode [264file] [lost264file] [lossRate] [slicesPerFrame] [decConfig] [seed] [iterations] [width] [height] [frames] [decodedSeq] [referenceSeq]\n");
		return;
	}

	lossRate = atoi(argv[3]);
	slicesPerFrame = atoi(argv[4]);

	randSeed = atoi(argv[6]);
	iterations = atoi(argv[7]);

	width = atoi(argv[8]);
	height = atoi(argv[9]);
	frames = atoi(argv[10]);


	/*allocate the distortion data buffer per pixel and per frame*/
	tdistortionBufferPP = (unsigned int *)malloc(sizeof(unsigned int)*width*height*frames);
	memset(tdistortionBufferPP, 0, sizeof(unsigned int)*width*height*frames);

	tdistortionBufferPF = (__int64 *)malloc(sizeof(__int64)*frames);
	memset(tdistortionBufferPF, 0, sizeof(__int64)*frames);
	
	/*allocate pixel buffer for synthesized view and reference view*/
	decodedFrameBuffer = (unsigned char *)malloc((width*height*3)>>1);
	referenceFrameBuffer = (unsigned char *)malloc((width*height*3)>>1);

	/*seed the process*/
	srand(randSeed);

	for(i=0; i<iterations; i++)
	{
		int j;
		int k;
		int ret;

		/*drop NALU in the sequence*/
		dropSlicesInSequence(argv[1], argv[2], lossRate, slicesPerFrame, frames);

		/*launch the decoder*/
		memset(decArg, 0, 256);
		sprintf(decArg, "ldecod.exe -f %s", argv[5]);
		printf("Start decoding\n");
		ret = system(decArg);
		if(ret!=0)
		{
			printf("Error launching %s\n",decArg);
		}
		







		/*accumulate SSE for each pixel*/
		decodedSeq = fopen(argv[11], "rb");
		if(decodedSeq==NULL)
		{
			printf("Can't open decoded sequence file %s\n", argv[11]);
		}

		referenceSeq = fopen(argv[12], "rb");
		if(referenceSeq==NULL)
		{
			printf("Can't open reference sequence file %s\n", argv[12]);
		}

		/*going through all the frames to find SSE at each pixel*/
		for(j=0; j<frames; j++)
		{
			unsigned int SSEthisFrame=0;

			fread(decodedFrameBuffer,sizeof(unsigned char),(width*height*3)>>1,decodedSeq);
			fread(referenceFrameBuffer,sizeof(unsigned char),(width*height*3)>>1,referenceSeq);

			for(k=0; k<width*height; k++)
			{
				unsigned int difference=0;
				if(decodedFrameBuffer[k]>=referenceFrameBuffer[k])
				{
					difference = (unsigned int)(decodedFrameBuffer[k]-referenceFrameBuffer[k]);
				}
				else
				{
					difference = (unsigned int)(referenceFrameBuffer[k]-decodedFrameBuffer[k]);
				}
				SSEthisFrame = SSEthisFrame + difference*difference;

				/*accumulate sse for each pixel*/
				tdistortionBufferPP[j*width*height+k] = tdistortionBufferPP[j*width*height+k] + difference*difference;
			}
			
			/*accumulate sse for each frame*/
			tdistortionBufferPF[j] = tdistortionBufferPF[j] + (__int64)SSEthisFrame;
		}

		fclose(decodedSeq);
		fclose(referenceSeq);
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

#if 0
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
	free(decodedFrameBuffer);
	free(referenceFrameBuffer);

	return;
}
