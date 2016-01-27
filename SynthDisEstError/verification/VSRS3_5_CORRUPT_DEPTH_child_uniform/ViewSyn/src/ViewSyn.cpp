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
/*The number of bins is odd.*/
void zeroMeanWhiteNoiseInit(double stdev, int numberBins, double *distribution)
{
   double probabilitySum;
   int i;

   if(numberBins<=0)
   {
      printf("Error numberBins less than 0: %d\n",numberBins);
   }
   if((numberBins%2)==0)
   {
      printf("Error numberBins is even: %d\n",numberBins);
   }


   for(i=0; i<numberBins; i++)
   {
      distribution[i] = 1/((double)numberBins);
   }


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

unsigned int seedValue=0;
void mySrand(unsigned int seed)
{
   seedValue = seed;
}

int myRand()
{
        return( ((seedValue = seedValue * 214013L
            + 2531011L) >> 16) & 0x7fff );
}
#endif

int main(int argc , char *argv[])
{
  unsigned int n;
#if CORRUPT_DEPTH_MAP
  double stdev;
  int numberBins;
  double *distribution;
  char seedFileName[256];
  FILE *seedFile;
#endif

  CParameterViewInterpolation  cParameter;

  CViewInterpolation cViewInterpolation;
  CIYuv yuvBuffer;

#if CORRUPT_DEPTH_MAP
  if(argc!=5)
  {
     printf("ViewSynVC8 [config file] [seeFile] [numberBins] [stdev(not used)]\n");
     return 0;
  }

  stdev = atof(argv[argc-1]);
  numberBins = atoi(argv[argc-2]);
  strcpy(seedFileName, argv[2]);
  seedFile = fopen(seedFileName, "rb");
  fread(&seedValue, sizeof(unsigned int), 1, seedFile);
  fclose(seedFile);
  mySrand(seedValue);//seed the experiment
  printf("Seeded with %d\n",seedValue);

  /*initialize the distribution*/
  distribution = (double *)malloc(sizeof(double)*numberBins);
  memset(distribution, 0, sizeof(double)*numberBins);
  zeroMeanWhiteNoiseInit(stdev, numberBins, distribution);
  printf("SeedFileName:%s Seed:%u bins:%u stdev:%f(not used)\n",seedFileName,seedValue,numberBins,stdev);
  argc = argc - 3;
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
  seedFile = fopen(seedFileName, "wb");
  fwrite(&seedValue, sizeof(unsigned int), 1, seedFile);
  fclose(seedFile);
  free(distribution);
#endif
  return 0;
}

