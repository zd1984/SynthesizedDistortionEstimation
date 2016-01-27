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

int main(int argc , char *argv[])
{
  unsigned int n;

  CParameterViewInterpolation  cParameter;

  CViewInterpolation cViewInterpolation;
  CIYuv yuvBuffer;

#ifdef OUTPUT_COMPUTATIONAL_TIME
  clock_t start, finish, first;
  first = start = clock();
#endif

#if ENABLE_VSDE
  printf("View Synthesis Reference Software (VSRS), Version %.1f (with VSDE on depth error)\n", VERSION);
#else
  printf("View Synthesis Reference Software (VSRS), Version %.1f\n", VERSION);
#endif
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
 
#ifdef OUTPUT_COMPUTATIONAL_TIME
  finish = clock();
  printf("Total: %.4f sec\n", ((double)(finish-first))/((double)CLOCKS_PER_SEC));
#endif

  return 0;
}

