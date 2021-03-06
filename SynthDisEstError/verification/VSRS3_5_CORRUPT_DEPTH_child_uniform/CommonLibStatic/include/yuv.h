/***********************************************************

             YUV image input/output functions

 ***********************************************************/
#ifndef _INCLUDE_YUV_H_
#define _INCLUDE_YUV_H_

#include <stdio.h>
#include <cv.h>
#include <cxcore.h>
#ifndef WIN32
#define BYTE unsigned char
#else
#define BYTE unsigned char
#endif

#include <highgui.h>

#define CORRUPT_DEPTH_MAP 0
#define MY_RAND_MAX 0x7fff

class CIYuv
{
public:
  CIYuv            ();
  CIYuv             (int h, int w, int chroma_format);
  virtual ~CIYuv   ();

  bool      Resize          (int h, int w, int chroma_format);
#if CORRUPT_DEPTH_MAP
  bool      ReadOneFrame      (FILE *fp, int frameno = -1, double *distribution=NULL, int numberBins=0, int enableFlag=0);
#else
  bool      ReadOneFrame      (FILE *fp, int frameno = -1);
#endif
  bool      WriteOneFrame      (FILE *fp);

  bool      WriteOneFrame_inYUV(FILE *fp);  // GIST test

  void      getData_inBGR      (IplImage *imgBGR);

  void      setDataFromImgBGR    (IplImage *imgBGR);
  void      setDataFromImgYUV    (IplImage *imgYUV);

  bool      setData444_inIBGR    (CIYuv *yuvSrc);
  bool      setData444_inIYUV    (CIYuv *yuvSrc);

  bool      setUpsampleFilter    (unsigned int filter, unsigned int precision);
  bool      upsampling        (CIYuv *src, int padding_size=0);

  int        getSampling  ()  { return sampling; }
  int        getHeight    ()  { return height; }
  int        getWidth     ()  { return width; }
  int        getHeightUV  ()  { return heightUV; }
  int        getWidthUV   ()  { return widthUV; }
  BYTE***    getData      ()  { return comp; }
  BYTE*      getBuffer    ()  { return pBuffer; }


  BYTE **Y;
  BYTE **U;
  BYTE **V;

private:
  bool      allocate_mem  ();
  void      release_mem    ();

  int        height;
  int        width;
  int        heightUV;
  int        widthUV;
  int        picsizeY;
  int        picsizeUV;
  int        size_in_byte;
  int        sampling;

  void      (*FuncFilter)(BYTE **in, BYTE **out, int width, int height, int padding_size);

  BYTE      **comp[3];

  BYTE      *pBuffer;  //!> 1D array for the Y, U and V component.
};

#endif