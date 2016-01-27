// loss_vs.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include<stdlib.h>
#include<string.h>


void getParametersFromFile(char *configFileName, int *width, int *height, int *frames, char *synthViewFileName)
{
   int size;
   int i;
   FILE *fhandle;
   char line[256];

   fhandle = fopen(configFileName, "rt");

   while(fgets(line, 256, fhandle) != NULL)
   {
      int parameterStart=0;
      int valueStart=0;
      char parameter[256];
      char value[256];

      for(size=0; size<strlen(line); size++)
      {
         if( (line[size]=='#') || (line[size]=='\n') )
         {
            break;
         }
      }

      memset(parameter, 0, 256);
      memset(value, 0, 256);

      for(i=0; i<size; i++)
      {
         if((line[i]!=32) && (line[i]!=9))
         {
            parameterStart = i;
            break;
         }
      }
      for(; i<size; i++)
      {
         if((line[i]==32) || (line[i]==9))
         {
            break;
         }
      }
      memcpy(parameter, &line[parameterStart], i-parameterStart);

      for(; i<size; i++)
      {
         if((line[i]!=32) && (line[i]!=9))
         {
            valueStart = i;
            break;
         }
      }
      for(; i<size; i++)
      {
         if((line[i]==32) || (line[i]==9))
         {
            break;
         }
      }
      memcpy(value, &line[valueStart], i-valueStart);

      if(strcmp(parameter, "SourceWidth")==0)
      {
         *width = atoi(value);
      }
      else if(strcmp(parameter, "SourceHeight")==0)
      {
         *height = atoi(value);
      }
      else if(strcmp(parameter, "TotalNumberOfFrames")==0)
      {
         *frames = atoi(value);
      }
      else if(strcmp(parameter, "OutputVirtualViewImageName")==0)
      {
         strcpy(synthViewFileName, value);
      }
      else
      {
      }
   }

   fclose(fhandle);
}

void main(int argc, char* argv[])
{
   int i;
   unsigned int randSeed;

   char vsArg[512];

   int width;
   int height;
   int frames;
   int iterations;
   char synViewName[256];

   FILE *synthView;
   FILE *refSynthView;

   FILE *SSEDataFile;

   FILE *randSeedFile;

   unsigned char *synthFrameBuffer;
   unsigned char *refSynthFrameBuffer;
   unsigned int *tdistortionBufferPP;
   __int64 *tdistortionBufferPF;


   if(argc!=9)
   {
      printf("corrupt_vs [vsConfig] [seed] [iterations] [refSynViewName] [seedFile] [numberBins] [stdev] [synthAppName]\n");
      return;
   }

   randSeed = atoi(argv[2]);
   iterations = atoi(argv[3]);

   getParametersFromFile(argv[1], &width, &height, &frames, synViewName);

   /*write the seed value to the file so that the child application can use it and update it*/
   randSeedFile = fopen(argv[5],"wb");
   fwrite(&randSeed, sizeof(unsigned int), 1, randSeedFile);
   fclose(randSeedFile);

   /*allocate the distortion data buffer per pixel and per frame*/
   tdistortionBufferPP = (unsigned int *)malloc(sizeof(unsigned int)*width*height*frames);
   memset(tdistortionBufferPP, 0, sizeof(unsigned int)*width*height*frames);

   tdistortionBufferPF = (__int64 *)malloc(sizeof(__int64)*frames);
   memset(tdistortionBufferPF, 0, sizeof(__int64)*frames);

   /*allocate pixel buffer for synthesized view and reference view*/
   synthFrameBuffer = (unsigned char *)malloc((width*height*3)>>1);
   refSynthFrameBuffer = (unsigned char *)malloc((width*height*3)>>1);

   for(i=0; i<iterations; i++)
   {
      int j;
      int k;
      int ret;

      memset(vsArg, 0, 512);
      sprintf(vsArg, "%s %s %s %s %s", argv[8], argv[1],argv[5],argv[6],argv[7]);
      printf("Start synthesizing middle view\n");
      ret = system(vsArg);
      if(ret!=0)
      {
         printf("Error launching %s\n",vsArg);
      }

      /*accumulate SSE for each pixel*/
      synthView = fopen(synViewName, "rb");
      if(synthView==NULL)
      {
         printf("Can't open synthesized view file %s\n", synViewName);
      }

      refSynthView = fopen(argv[4], "rb");
      if(refSynthView==NULL)
      {
         printf("Can't open reference synthesized view file %s\n", argv[4]);
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


   free(tdistortionBufferPP);
   free(tdistortionBufferPF);
   free(synthFrameBuffer);
   free(refSynthFrameBuffer);

   return;
}

