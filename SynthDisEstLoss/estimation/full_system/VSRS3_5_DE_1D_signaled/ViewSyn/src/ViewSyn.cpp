//#include <string.h>
#include <time.h>

#include "version.h"
#include "yuv.h"
#include "ParameterViewInterpolation.h"
#include "ViewInterpolation.h"
#include "vsde.h" //vsde added

#ifndef WIN32
#define BYTE unsigned char
#endif
#if ENABLE_VSDE
#define STR_SIZE 256
FILE *frameDistortionFile;
double distortionGlobal;
char textureLeftSharedMemName[STR_SIZE];
char textureRightSharedMemName[STR_SIZE];
char depthLeftSharedMemName[STR_SIZE];
char depthRightSharedMemName[STR_SIZE];
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
#endif
int main(int argc , char *argv[])
{
#if ENABLE_VSDE
  TCHAR tempTCHAR[STR_SIZE];
  HANDLE textureHandleLeft;
  HANDLE textureHandleRight;
  HANDLE depthHandleLeft;
  HANDLE depthHandleRight;
#endif
  unsigned int n;

  CParameterViewInterpolation  cParameter;

  CViewInterpolation cViewInterpolation;
  CIYuv yuvBuffer;

#ifdef OUTPUT_COMPUTATIONAL_TIME
  clock_t start, finish, first;
  first = start = clock();
#endif

#if ENABLE_VSDE
  printf("View Synthesis Reference Software (VSRS), Version %.1f (with VSDE on loss)\n", VERSION);
#else
  printf("View Synthesis Reference Software (VSRS), Version %.1f\n", VERSION);
#endif
  printf("     - MPEG 3DV, August 2009\n\n");

#if ENABLE_VSDE
  /*configFileName textureLeftSharedMemName textureRightSharedMemName depthLeftSharedMemName depthRightSharedMemName sharedMemIndex*/
  if(argc!=6)
  {
     printf("Error View synthesizer needs correct number of parameters\n");
     return 0;
  }
  sprintf(depthRightSharedMemName, "%s", argv[argc-1]);
  sprintf(depthLeftSharedMemName, "%s", argv[argc-2]);
  sprintf(textureRightSharedMemName, "%s", argv[argc-3]);
  sprintf(textureLeftSharedMemName, "%s", argv[argc-4]);
  argc = argc - 4;

  /*Open the shared memory handle*/
  charToTCHAR(tempTCHAR, textureLeftSharedMemName);
  textureHandleLeft = OpenFileMapping(
                  FILE_MAP_ALL_ACCESS,   // read/write access
                  FALSE,                 // do not inherit the name
                  tempTCHAR);               // name of mapping object

  if (textureHandleLeft == NULL)
  {
     printf("Could not open file mapping object (%d).\n",
            GetLastError());
     exit(0);
  }

  charToTCHAR(tempTCHAR, textureRightSharedMemName);
  textureHandleRight = OpenFileMapping(
                  FILE_MAP_ALL_ACCESS,   // read/write access
                  FALSE,                 // do not inherit the name
                  tempTCHAR);               // name of mapping object

  if (textureHandleRight == NULL)
  {
     printf("Could not open file mapping object (%d).\n",
            GetLastError());
     exit(0);
  }

  charToTCHAR(tempTCHAR, depthLeftSharedMemName);
  depthHandleLeft = OpenFileMapping(
                  FILE_MAP_ALL_ACCESS,   // read/write access
                  FALSE,                 // do not inherit the name
                  tempTCHAR);               // name of mapping object

  if (depthHandleLeft == NULL)
  {
     printf("Could not open file mapping object (%d).\n",
            GetLastError());
     exit(0);
  }

  charToTCHAR(tempTCHAR, depthRightSharedMemName);
  depthHandleRight = OpenFileMapping(
                  FILE_MAP_ALL_ACCESS,   // read/write access
                  FALSE,                 // do not inherit the name
                  tempTCHAR);               // name of mapping object

  if (depthHandleRight == NULL)
  {
     printf("Could not open file mapping object (%d).\n",
            GetLastError());
     exit(0);
  }
#endif

  if ( cParameter.Init( argc, argv ) != 1 ) return 0;

#if ENABLE_VSDE
  if(!cViewInterpolation.Init(cParameter, textureHandleLeft, textureHandleRight, depthHandleLeft, depthHandleRight)) return 10;
#else
  if(!cViewInterpolation.Init(cParameter)) return 10;
#endif

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

    if( !cViewInterpolation.getDepthBufferLeft() ->ReadOneFrame(fin_depth_l, n) ||
        !cViewInterpolation.getDepthBufferRight()->ReadOneFrame(fin_depth_r, n)  ) break;
    printf(".");

    cViewInterpolation.setFrameNumber( n - cParameter.getStartFrame()); // Zhejiang

    if(!yuvBuffer.ReadOneFrame(fin_view_l, n)) break;
    if(!cViewInterpolation.SetReferenceImage(1, &yuvBuffer)) break;
    printf(".");

    if(!yuvBuffer.ReadOneFrame(fin_view_r, n)) break;
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

#if ENABLE_VSDE
  /*close the shared memory handle*/
  CloseHandle(textureHandleLeft);
  CloseHandle(textureHandleRight);
  CloseHandle(depthHandleLeft);
  CloseHandle(depthHandleRight);

  /*save the frame distortion to a file*/
  frameDistortionFile = fopen("singleFrameDistortionFile.bin","wb");
  if(frameDistortionFile==NULL)
  {
     printf("Error opening file\n");
  }
  fwrite(&distortionGlobal, sizeof(double), 1, frameDistortionFile);
  fclose(frameDistortionFile);
#endif

#ifdef OUTPUT_COMPUTATIONAL_TIME
  finish = clock();
  printf("Total: %.4f sec\n", ((double)(finish-first))/((double)CLOCKS_PER_SEC));
#endif

  return 0;
}

