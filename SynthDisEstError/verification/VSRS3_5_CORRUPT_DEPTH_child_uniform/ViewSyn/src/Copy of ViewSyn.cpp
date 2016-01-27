//#include <string.h>
#include <time.h>

#include "version.h"
#include "yuv.h"
#include "ParameterViewInterpolation.h"
#include "ViewInterpolation.h"

#ifndef WIN32
#define BYTE unsigned char
#endif

#if CORRUPT_DEPTH_MAP
#if HOLE_MAP_CHECK
unsigned char overlappedHoleMap[768][1024];
FILE *overlappedHoleMapImage;
#endif
/*This is a Gaussian approximation*/
/*The number of bins is odd. The probability is grown from the bins on the side. In order to make the probability distribution valid, center bin take the rest of the probability*/
void zeroMeanGaussianNoiseInit(double stdev, int numberBins, double *distribution)
{
   double probabilitySum;
   int i;

   double a;
   double b;

   if(numberBins<=0)
   {
      printf("Error numberBins less than 0: %d\n",numberBins);
   }
   if((numberBins%2)==0)
   {
      printf("Error numberBins is even: %d\n",numberBins);
   }

   a=1/sqrt(2*3.14159265);
   a=a/stdev;

   b=-2*stdev*stdev;

   probabilitySum = 0;
   for(i=0; i<(numberBins>>1); i++)
   {
      double x=(double)(i-(numberBins>>1));
      distribution[i] = a*exp(x*x/b);
      distribution[numberBins-1-i] = distribution[i];
      probabilitySum = probabilitySum + distribution[i]*2;
   }
   distribution[numberBins>>1] = 1 - probabilitySum;

   /*checking*/
   probabilitySum = 0;
   printf("Probability Distribution:\n");
   for(i=0; i<numberBins; i++)
   {
      printf("Val:%d Prob:%f\n", i-(numberBins>>1), distribution[i]);
      probabilitySum = probabilitySum + distribution[i];
   }
   printf("Total probability %f\n",probabilitySum);
   if(probabilitySum!=1)
   {
      printf("Error distribution does not sum up to 1: %f\n",probabilitySum);
   }

   return;
}
#endif

int main(int argc , char *argv[])
{
  unsigned int n;
#if CORRUPT_DEPTH_MAP
  int i;
  int j;
  int k;

  double stdev;
  int numberBins;
  int seed;
  int iterations;

  double *distribution;

  char referenceVirtualViewName[256];

  FILE *referenceVirtualView;
  FILE *virtualView;

  unsigned char *synthFrameBuffer;
  unsigned char *refSynthFrameBuffer;
  unsigned int *tdistortionBufferPP;
  __int64 *tdistortionBufferPF;

  CParameterViewInterpolation  cParameterTemp;
#endif

  CParameterViewInterpolation  cParameter;

  CViewInterpolation cViewInterpolation;
  CIYuv yuvBuffer;

#if CORRUPT_DEPTH_MAP
  if(argc!=7)
  {
     printf("ViewSynVC8 [config file] [reference virtual view] [seed] [iterations] [number of bins] [standard deviation]\n");
     return 0;
  }
#if HOLE_MAP_CHECK
  memset(overlappedHoleMap, 0, 1024*768);
  overlappedHoleMapImage = fopen("overlappedHoleMapImage.yuv","wb");
#endif
  stdev = atof(argv[argc-1]);
  numberBins = atoi(argv[argc-2]);
  iterations = atoi(argv[argc-3]);
  seed = atoi(argv[argc-4]);
  memset(referenceVirtualViewName, 0, 256);
  sprintf(referenceVirtualViewName, "%s", argv[argc-5]);
  argc = argc - 5;

  /*grab a copy of the parameters for initilization*/
  if ( cParameterTemp.Init( argc, argv ) != 1 ) return 0;

  /*allocate the distortion data buffer per pixel and per frame*/
  tdistortionBufferPP = (unsigned int *)malloc(sizeof(unsigned int)*cParameterTemp.getSourceWidth()*cParameterTemp.getSourceHeight()*cParameterTemp.getNumberOfFrames());
  memset(tdistortionBufferPP, 0, sizeof(unsigned int)*cParameterTemp.getSourceWidth()*cParameterTemp.getSourceHeight()*cParameterTemp.getNumberOfFrames());

  tdistortionBufferPF = (__int64 *)malloc(sizeof(__int64)*cParameterTemp.getNumberOfFrames());
  memset(tdistortionBufferPF, 0, sizeof(__int64)*cParameterTemp.getNumberOfFrames());

  /*allocate pixel buffer for synthesized view and reference view*/
  synthFrameBuffer = (unsigned char *)malloc((cParameterTemp.getSourceWidth()*cParameterTemp.getSourceHeight()*3)>>1);
  refSynthFrameBuffer = (unsigned char *)malloc((cParameterTemp.getSourceWidth()*cParameterTemp.getSourceHeight()*3)>>1);

  /*initialize the distribution*/
  distribution = (double *)malloc(sizeof(double)*numberBins);
  memset(distribution, 0, sizeof(double)*numberBins);
  zeroMeanGaussianNoiseInit(stdev, numberBins, distribution);

  /*seed and start the tests*/
  srand(seed);
  for(i=0; i<iterations; i++)
  {
#endif

#ifdef OUTPUT_COMPUTATIONAL_TIME
  clock_t start, finish, first;
  first = start = clock();
#endif

  printf("View Synthesis Reference Software (VSRS), Version %.1f\n", VERSION);
  printf("     - MPEG 3DV, August 2009\n\n");

  if ( cParameter.Init( argc, argv ) != 1 ) return 0;

  if(!cViewInterpolation.Init(cParameter)) return 10;

  if(!yuvBuffer.Resize(cParameter.getSourceHeight(), cParameter.getSourceWidth(), 420)) return 2;

  FILE *fin_view_r, *fin_view_l, *fin_depth_r, *fin_depth_l, *fout;

  if( (fin_view_l  = fopen(cParameter.getLeftViewImageName() .c_str(), "rb"))==NULL ||
      (fin_view_r  = fopen(cParameter.getRightViewImageName().c_str(), "rb"))==NULL ||
      (fin_depth_l = fopen(cParameter.getLeftDepthMapName()  .c_str(), "rb"))==NULL ||
      (fin_depth_r = fopen(cParameter.getRightDepthMapName() .c_str(), "rb"))==NULL ||
      (fout = fopen(cParameter.getOutputVirViewImageName()   .c_str(), "wb"))==NULL )
  {
    fprintf(stderr, "Can't open input file(s)\n");
    return 3;
  }

#ifdef OUTPUT_COMPUTATIONAL_TIME
  finish = clock();
  printf( "Initialization: %.4f sec\n", (double)(finish - start) / CLOCKS_PER_SEC);
  start = finish;
#endif

  for(n = cParameter.getStartFrame(); n < cParameter.getStartFrame() + cParameter.getNumberOfFrames(); n++)
  {
    printf("frame number = %d ", n);
#if CORRUPT_DEPTH_MAP
    if( !cViewInterpolation.getDepthBufferLeft() ->ReadOneFrame(fin_depth_l, n, distribution, numberBins, 1) ||
        !cViewInterpolation.getDepthBufferRight()->ReadOneFrame(fin_depth_r, n, distribution, numberBins, 1)  ) break;
#else
    if( !cViewInterpolation.getDepthBufferLeft() ->ReadOneFrame(fin_depth_l, n) ||
        !cViewInterpolation.getDepthBufferRight()->ReadOneFrame(fin_depth_r, n)  ) break;
#endif
    printf(".");

    cViewInterpolation.setFrameNumber( n - cParameter.getStartFrame()); // Zhejiang

#if CORRUPT_DEPTH_MAP
    if(!yuvBuffer.ReadOneFrame(fin_view_l, n, NULL, 0, 0)) break;
#else
    if(!yuvBuffer.ReadOneFrame(fin_view_l, n)) break;
#endif
    if(!cViewInterpolation.SetReferenceImage(1, &yuvBuffer)) break;
    printf(".");

#if CORRUPT_DEPTH_MAP
    if(!yuvBuffer.ReadOneFrame(fin_view_r, n, NULL, 0, 0)) break;
#else
    if(!yuvBuffer.ReadOneFrame(fin_view_r, n)) break;
#endif
    if(!cViewInterpolation.SetReferenceImage(0, &yuvBuffer)) break;
    printf(".");

    if(!cViewInterpolation.DoViewInterpolation( &yuvBuffer )) break;
    printf(".");

    if(!yuvBuffer.WriteOneFrame(fout)) break;

#ifdef OUTPUT_COMPUTATIONAL_TIME
    finish = clock();
    printf("->End (%.4f sec)\n", (double)(finish - start) / CLOCKS_PER_SEC);
    start = finish;
#else
    printf("->End\n");
#endif

  } // for n

  fclose(fout);
  fclose(fin_view_l);
  fclose(fin_view_r);
  fclose(fin_depth_l);
  fclose(fin_depth_r);

#ifdef OUTPUT_COMPUTATIONAL_TIME
  finish = clock();
  printf("Total: %.4f sec\n", ((double)(finish-first))/((double)CLOCKS_PER_SEC));
#endif

#if CORRUPT_DEPTH_MAP
  {
      /*accumulate SSE for each pixel*/
      virtualView = fopen(cParameterTemp.getOutputVirViewImageName().c_str(), "rb");
      if(virtualView==NULL)
      {
         printf("Can't open synthesized view file %s\n", cParameterTemp.getOutputVirViewImageName().c_str());
      }

      referenceVirtualView = fopen(referenceVirtualViewName, "rb");
      if(referenceVirtualView==NULL)
      {
         printf("Can't open reference synthesized view file %s\n", referenceVirtualViewName);
      }

      /*going through all the frames to find SSE at each pixel*/
      for(j=0; j<cParameter.getNumberOfFrames(); j++)
      {
         unsigned int SSEthisFrame=0;

         fread(synthFrameBuffer, sizeof(unsigned char), (cParameter.getSourceWidth()*cParameter.getSourceHeight()*3)>>1, virtualView);
         fread(refSynthFrameBuffer, sizeof(unsigned char), (cParameter.getSourceWidth()*cParameter.getSourceHeight()*3)>>1, referenceVirtualView);

         for(k=0; k<cParameter.getSourceWidth()*cParameter.getSourceHeight(); k++)
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
            tdistortionBufferPP[j*cParameter.getSourceWidth()*cParameter.getSourceHeight()+k] = tdistortionBufferPP[j*cParameter.getSourceWidth()*cParameter.getSourceHeight()+k] + difference*difference;
         }

         /*accumulate sse for each frame*/
         tdistortionBufferPF[j] = tdistortionBufferPF[j] + (__int64)SSEthisFrame;
      }

      fclose(virtualView);
      fclose(referenceVirtualView);
  }
#endif

#if CORRUPT_DEPTH_MAP
  }

   /*do an error checking on data collection*/
   {
#if SAVE_ONE_FRAME_SSE
      FILE *perPixelSSEAvg;//saving one frame per pixel average SSE
      perPixelSSEAvg = fopen("oneFramePPavgSSE.bin","wb");//saving one frame per pixel average SSE
#endif
      for(j=0; j<cParameterTemp.getNumberOfFrames(); j++)
      {
         __int64 frameSSE=0;
         for(k=0; k<cParameterTemp.getSourceWidth()*cParameterTemp.getSourceHeight(); k++)
         {
#if SAVE_ONE_FRAME_SSE
            unsigned int outData = tdistortionBufferPP[j*cParameterTemp.getSourceWidth()*cParameterTemp.getSourceHeight()+k]/iterations;//saving one frame per pixel average SSE
            fwrite(&outData, sizeof(unsigned int), 1, perPixelSSEAvg);//saving one frame per pixel average SSE
#endif
            frameSSE = frameSSE + (__int64)tdistortionBufferPP[j*cParameterTemp.getSourceWidth()*cParameterTemp.getSourceHeight()+k];
         }
         if(tdistortionBufferPF[j]!=frameSSE)
         {
            printf("Error total per pixel SSE does not add up to total per frame SSE frame:%u %I64d %I64d\n",j ,tdistortionBufferPF[j] ,frameSSE);
         }
      }
#if SAVE_ONE_FRAME_SSE
      fclose(perPixelSSEAvg);//saving one frame per pixel average SSE
#endif
   }

   /*calculate the average*/
   printf("Total number of iterations:%d Seed:%d\n", iterations, seed);
   for(i=0; i<cParameterTemp.getNumberOfFrames(); i++)
   {
      tdistortionBufferPF[i] = tdistortionBufferPF[i]/iterations;
      printf("Frame:%u SSE:%I64d\n",i,tdistortionBufferPF[i]);
      for(j=0; j<cParameterTemp.getSourceWidth()*cParameterTemp.getSourceHeight(); j++)
      {
          tdistortionBufferPP[i*cParameterTemp.getSourceWidth()*cParameterTemp.getSourceHeight()+j] = tdistortionBufferPP[i*cParameterTemp.getSourceWidth()*cParameterTemp.getSourceHeight()+j]/iterations;
      }
   }

  free(synthFrameBuffer);
  free(refSynthFrameBuffer);
  free(tdistortionBufferPP);
  free(tdistortionBufferPF);

  free(distribution);

#if HOLE_MAP_CHECK
  fwrite(overlappedHoleMap, 1024*768, 1, overlappedHoleMapImage);
  fclose(overlappedHoleMapImage);
#endif
#endif
  return 0;
}

