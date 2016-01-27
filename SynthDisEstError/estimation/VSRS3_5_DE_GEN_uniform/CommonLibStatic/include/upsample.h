#ifndef __INCLUDE_UPSAMPLE_H__
#define __INCLUDE_UPSAMPLE_H__

void HorizontalLinearFilter_1D_half(BYTE *in, BYTE *out, int width, int padding_size);
void HorizontalLinearFilter_2D_half(BYTE **in, BYTE **out, int width, int height, int padding_size);
void HorizontalLinearFilter_1D_qpel(BYTE *in, BYTE *out, int width, int padding_size);
void HorizontalLinearFilter_2D_qpel(BYTE **in, BYTE **out, int width, int height, int padding_size);
void HorizontalAVCFilter_1D_half(BYTE *in, BYTE *out, int width, int padding_size);
void HorizontalAVCFilter_2D_half(BYTE **in, BYTE **out, int width, int height, int padding_size);
void HorizontalAVCFilter_1D_qpel(BYTE *in, BYTE *out, int width, int padding_size);
void HorizontalAVCFilter_2D_qpel(BYTE **in, BYTE **out, int width, int height, int padding_size);
void Horizontal6tapFilter_1D_half(BYTE *in, BYTE *out, int width, int padding_size, int coeff[6]);
void Horizontal6tapFilter_2D_half(BYTE **in, BYTE **out, int width, int height, int padding_size, int coeff[6]);
void HorizontalCubicFilter_1D_half(BYTE *in, BYTE *out, int width, int padding_size);
void HorizontalCubicFilter_2D_half(BYTE **in, BYTE **out, int width, int height, int padding_size);
void HorizontalCubicFilter_1D_qpel(BYTE *in, BYTE *out, int width, int padding_size);
void HorizontalCubicFilter_2D_qpel(BYTE **in, BYTE **out, int width, int height, int padding_size);
void DummyFilter_1D(BYTE *in, BYTE *out, int width, int padding_size);
void DummyFilter_2D(BYTE **in, BYTE **out, int width, int height, int padding_size);

#ifndef CLIP
#define CLIP(x,min,max) ( (x)<(min)?(min):((x)>(max)?(max):(x)) )
#endif

/*
 * TODO: More s/w work to encapsulate the above functions into the following class
 */

class CPictureResample
{
private:

  // Algorithm 0: Simple repeat /drop pixels
  void RepeatPixel420to444(unsigned char* oPic, unsigned char* iPic, int Width, int Height);
  void DropPixel444to420  (unsigned char* oPic, unsigned char* iPic, int Width, int Height);

  unsigned char PixelClip(const int pxlVal);
  
  void DoUpsample(unsigned char* Yo, unsigned char* Yi, int Width, int Height);
  void DoDownsample(unsigned char* Yo, unsigned char* Yi, int Width, int Height);

public:
  CPictureResample();
  ~CPictureResample();
  
  int  State();
  
  // For UV format conversion
  void PictureResample420to444(unsigned char* oPic, unsigned char* iPic, int Width, int Height);
  void PictureResample444to420(unsigned char* oPic, unsigned char* iPic, int Width, int Height);
  
  // For picture upsample and downsample
  int UpsampleView(unsigned char* Yo, unsigned char* Yi, int Width, int Height, int Factor);
  int DownsampleView(unsigned char* Yo, unsigned char* Yi, int Width, int Height, int Factor);
};



#endif
