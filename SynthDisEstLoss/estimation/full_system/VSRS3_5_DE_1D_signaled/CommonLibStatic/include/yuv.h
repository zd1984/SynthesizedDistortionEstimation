/***********************************************************

             YUV image input/output functions

 ***********************************************************/
#ifndef _INCLUDE_YUV_H_
#define _INCLUDE_YUV_H_

#include <stdio.h>
#include <cv.h>
#include <cxcore.h>
#include <tchar.h>/*dzhanga ENABLE_VSDE*/
#include <windows.h>/*dzhanga ENABLE_VSDE*/
#ifndef WIN32
#define BYTE unsigned char
#endif

#include <highgui.h>

class CIYuv
{
public:
  CIYuv            ();
  CIYuv             (int h, int w, int chroma_format);
  virtual ~CIYuv   ();

  bool      Resize          (int h, int w, int chroma_format);
  bool      ReadOneFrame      (FILE *fp, int frameno = -1);
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