
#ifdef WIN32
#pragma warning(disable : 4996)
#endif

#include "yuv.h"
#include "upsample.h"

#ifndef CLIP3
#define CLIP3(x,min,max) ( (x)<(min)?(min):((x)>(max)?(max):(x)) )
#endif

#ifndef TURN3
#define TURN3(x,min,max) ( (x)<(min)?(-x): ((x)>(max)?(2*(max)-(x)):(x)) )
#endif

#ifndef BYTE
#define BYTE unsigned char
#endif

void HorizontalLinearFilter_1D_half(BYTE *in, BYTE *out, int width, int padding_size)
{
  int i, left, right;
  int max_width = width + padding_size;
  int width_minus1 = width-1;

  for(i=-padding_size; i<max_width; i++)
  {
    left = CLIP3(i, 0, width_minus1);
    right = CLIP3(i+1, 0, width_minus1);
    out[ (i+padding_size)<<1   ] =  in[left];
    out[((i+padding_size)<<1)+1] = (in[left]+in[right]+1)>>1;
  }
}

void HorizontalLinearFilter_2D_half(BYTE **in, BYTE **out, int width, int height, int padding_size)
{
  int i, left, right;
  int max_width = width + padding_size;
  int width_minus1 = width-1;
  int h, int_pel, half_pel;

  for(i=-padding_size; i<max_width; i++)
  {
    left = CLIP3(i, 0, width_minus1);
    right = CLIP3(i+1, 0, width_minus1);
    int_pel = (i+padding_size)<<1;
    half_pel = int_pel+1;
    for(h=0; h<height; h++)
    {
      out[h][int_pel] = in[h][left];
      out[h][half_pel] = (in[h][left]+in[h][right]+1)>>1;
    }
  }
}

void HorizontalLinearFilter_1D_qpel(BYTE *in, BYTE *out, int width, int padding_size)
{
  int i, left, right;
  int max_width = width + padding_size;
  int width_minus1 = width-1;

  for(i=-padding_size; i<max_width; i++)
  {
    left = CLIP3(i, 0, width_minus1);
    right = CLIP3(i+1, 0, width_minus1);
    out[ (i+padding_size)<<2   ] =  in[left];
    out[((i+padding_size)<<2)+1] = (in[left]*3 + in[right]   + 2)>>2;
    out[((i+padding_size)<<2)+2] = (in[left]   + in[right]   + 1)>>1;
    out[((i+padding_size)<<2)+3] = (in[left]   + in[right]*3 + 2)>>2;
  }
}

void HorizontalLinearFilter_2D_qpel(BYTE **in, BYTE **out, int width, int height, int padding_size)
{
  int i, left, right;
  int max_width = width + padding_size;
  int width_minus1 = width-1;
  int h, pel[4];

  for(i=-padding_size; i<max_width; i++)
  {
    left = CLIP3(i, 0, width_minus1);
    right = CLIP3(i+1, 0, width_minus1);
    pel[0] = (i+padding_size)<<2;
    pel[1] = pel[0]+1;
    pel[2] = pel[1]+1;
    pel[3] = pel[2]+1;
    for(h=0; h<height; h++)
    {
      out[h][pel[0]] =  in[h][left];
      out[h][pel[1]] = (in[h][left]*3 + in[h][right]   + 2)>>2;
      out[h][pel[2]] = (in[h][left]   + in[h][right]   + 1)>>1;
      out[h][pel[3]] = (in[h][left]   + in[h][right]*3 + 2)>>2;
    }
  }
}

void HorizontalAVCFilter_1D_half(BYTE *in, BYTE *out, int width, int padding_size)
{
  int i, pel[6];
  int max_width = width + padding_size;
  int width_minus1 = width-1;

  for(i=-padding_size; i<max_width; i++)
  {
    pel[0] = CLIP3(i-2, 0, width_minus1);
    pel[1] = CLIP3(i-1, 0, width_minus1);
    pel[2] = CLIP3(i  , 0, width_minus1);
    pel[3] = CLIP3(i+1, 0, width_minus1);
    pel[4] = CLIP3(i+2, 0, width_minus1);
    pel[5] = CLIP3(i+3, 0, width_minus1);

    out[ (i+padding_size)<<1   ] =  in[pel[2]];
    out[((i+padding_size)<<1)+1] = CLIP3( (20*(in[pel[2]]+in[pel[3]]) - 5*(in[pel[1]]+in[pel[4]]) + (in[pel[0]]+in[pel[5]]) +16)>>5, 0, 255 );
  }
}

void HorizontalAVCFilter_2D_half(BYTE **in, BYTE **out, int width, int height, int padding_size)
{
  int i, pel[6];
  int max_width = width + padding_size;
  int width_minus1 = width-1;
  int h, int_pel, half_pel;

  for(i=-padding_size; i<max_width; i++)
  {
    pel[0] = CLIP3(i-2, 0, width_minus1);
    pel[1] = CLIP3(i-1, 0, width_minus1);
    pel[2] = CLIP3(i  , 0, width_minus1);
    pel[3] = CLIP3(i+1, 0, width_minus1);
    pel[4] = CLIP3(i+2, 0, width_minus1);
    pel[5] = CLIP3(i+3, 0, width_minus1);

    int_pel = (i+padding_size)<<1;
    half_pel = int_pel+1;
    for(h=0; h<height; h++)
    {
      out[h][int_pel] = in[h][pel[2]];
      out[h][half_pel] = CLIP3( (20*(in[h][pel[2]]+in[h][pel[3]]) - 5*(in[h][pel[1]]+in[h][pel[4]]) + (in[h][pel[0]]+in[h][pel[5]]) +16)>>5, 0, 255 );
    }
  }
}

void HorizontalAVCFilter_1D_qpel(BYTE *in, BYTE *out, int width, int padding_size)
{
  int i, pel[6];
  int max_width = width + padding_size;
  int width_minus1 = width-1;

  for(i=-padding_size; i<max_width; i++)
  {
    pel[0] = CLIP3(i-2, 0, width_minus1);
    pel[1] = CLIP3(i-1, 0, width_minus1);
    pel[2] = CLIP3(i  , 0, width_minus1);
    pel[3] = CLIP3(i+1, 0, width_minus1);
    pel[4] = CLIP3(i+2, 0, width_minus1);
    pel[5] = CLIP3(i+3, 0, width_minus1);

    out[ (i+padding_size)<<2   ] = in[pel[2]];
    out[((i+padding_size)<<2)+2] = CLIP3( (20*(in[pel[2]]+in[pel[3]]) - 5*(in[pel[1]]+in[pel[4]]) + (in[pel[0]]+in[pel[5]]) +16)>>5, 0, 255 );
    out[((i+padding_size)<<2)+1] = (in[pel[2]] + out[((i+padding_size)<<2)+2] + 1)>>1;
    out[((i+padding_size)<<2)+3] = (in[pel[3]] + out[((i+padding_size)<<2)+2] + 1)>>1;
  }
}

void HorizontalAVCFilter_2D_qpel(BYTE **in, BYTE **out, int width, int height, int padding_size)
{
  int i;
  int max_width = width + padding_size;
  int width_minus1 = width-1;
  int h, ipel[6], opel[4];

  for(i=-padding_size; i<max_width; i++)
  {
    ipel[0] = CLIP3(i-2, 0, width_minus1);
    ipel[1] = CLIP3(i-1, 0, width_minus1);
    ipel[2] = CLIP3(i  , 0, width_minus1);
    ipel[3] = CLIP3(i+1, 0, width_minus1);
    ipel[4] = CLIP3(i+2, 0, width_minus1);
    ipel[5] = CLIP3(i+3, 0, width_minus1);

    opel[0] = (i+padding_size)<<2;
    opel[1] = opel[0]+1;
    opel[2] = opel[1]+1;
    opel[3] = opel[2]+1;
    for(h=0; h<height; h++)
    {
      out[h][opel[0]] =  in[h][ipel[2]];
      out[h][opel[2]] = CLIP3( (20*(in[h][ipel[2]]+in[h][ipel[3]]) - 5*(in[h][ipel[1]]+in[h][ipel[4]]) + (in[h][ipel[0]]+in[h][ipel[5]]) +16)>>5, 0, 255 );
      out[h][opel[1]] = (in[h][ipel[2]] + out[h][opel[2]] + 1)>>1;
      out[h][opel[3]] = (in[h][ipel[3]] + out[h][opel[2]] + 1)>>1;
    }
  }
}

void Horizontal6tapFilter_1D_half(BYTE *in, BYTE *out, int width, int padding_size, int coeff[6])
{
  int i, ipel[6], opel[2];
  int max_width = width + padding_size;
  int width_minus1 = width-1;
  int sum_coeff, offset;

  for(i=sum_coeff=0; i<6; i++)
    sum_coeff += coeff[i];
  offset = sum_coeff>>1;

  for(i=-padding_size; i<max_width; i++)
  {
    ipel[0] = CLIP3(i-2, 0, width_minus1);
    ipel[1] = CLIP3(i-1, 0, width_minus1);
    ipel[2] = CLIP3(i  , 0, width_minus1);
    ipel[3] = CLIP3(i+1, 0, width_minus1);
    ipel[4] = CLIP3(i+2, 0, width_minus1);
    ipel[5] = CLIP3(i+3, 0, width_minus1);
    opel[0] = (i+padding_size)<<1;
    opel[1] = opel[0]+1;

    out[opel[0]] = in[ipel[2]];
    out[opel[1]] = CLIP3( (in[ipel[0]]*coeff[0] + in[ipel[1]]*coeff[1] + in[ipel[2]]*coeff[2] +
                           in[ipel[3]]*coeff[3] + in[ipel[4]]*coeff[4] + in[ipel[5]]*coeff[5] + offset)/sum_coeff, 0, 255);
  }
}

void Horizontal6tapFilter_2D_half(BYTE **in, BYTE **out, int width, int height, int padding_size, int coeff[6])
{
  int i, ipel[6], opel[2];
  int max_width = width + padding_size;
  int width_minus1 = width-1;
  int h;
  int sum_coeff, offset;

  for(i=sum_coeff=0; i<6; i++)
    sum_coeff += coeff[i];
  offset = sum_coeff>>1;

  for(i=-padding_size; i<max_width; i++)
  {
    ipel[0] = CLIP3(i-2, 0, width_minus1);
    ipel[1] = CLIP3(i-1, 0, width_minus1);
    ipel[2] = CLIP3(i  , 0, width_minus1);
    ipel[3] = CLIP3(i+1, 0, width_minus1);
    ipel[4] = CLIP3(i+2, 0, width_minus1);
    ipel[5] = CLIP3(i+3, 0, width_minus1);

    opel[0] = (i+padding_size)<<1;
    opel[1] = opel[0]+1;

    for(h=0; h<height; h++)
    {
      out[h][opel[0]] = in[h][ipel[2]];
      out[h][opel[1]] = CLIP3( (in[h][ipel[0]]*coeff[0] + in[h][ipel[1]]*coeff[1] + in[h][ipel[2]]*coeff[2] +
                               in[h][ipel[3]]*coeff[3] + in[h][ipel[4]]*coeff[4] + in[h][ipel[5]]*coeff[5] + offset)/sum_coeff, 0, 255);
    }
  }
}

void HorizontalCubicFilter_1D_half(BYTE *in, BYTE *out, int width, int padding_size)
{
  int i, ipel[4], opel[2];
  int max_width = width + padding_size;
  int width_minus1 = width-1;

  for(i=-padding_size; i<max_width; i++)
  {
//    ipel[0] = CLIP3(i-1, 0, width_minus1);
//    ipel[1] = CLIP3(i  , 0, width_minus1);
//    ipel[2] = CLIP3(i+1, 0, width_minus1);
//    ipel[3] = CLIP3(i+2, 0, width_minus1);
    ipel[0] = TURN3(i-1, 0, width_minus1);
    ipel[1] = TURN3(i  , 0, width_minus1);
    ipel[2] = TURN3(i+1, 0, width_minus1);
    ipel[3] = TURN3(i+2, 0, width_minus1);

    opel[0] = (i+padding_size)<<1;
    opel[1] = opel[0]+1;

    out[opel[0]] = in[ipel[1]];
    out[opel[1]] = CLIP3( (5*(in[ipel[1]] +    in[ipel[2]])-   in[ipel[0]] -   in[ipel[3]] +  4)>>3, 0, 255 );
  }
}

void HorizontalCubicFilter_2D_half(BYTE **in, BYTE **out, int width, int height, int padding_size)
{
  int i, ipel[4], opel[2];
  int max_width = width + padding_size;
  int width_minus1 = width-1;
  int h;

  for(i=-padding_size; i<max_width; i++)
  {
//    ipel[0] = CLIP3(i-1, 0, width_minus1);
//    ipel[1] = CLIP3(i  , 0, width_minus1);
//    ipel[2] = CLIP3(i+1, 0, width_minus1);
//    ipel[3] = CLIP3(i+2, 0, width_minus1);
    ipel[0] = TURN3(i-1, 0, width_minus1);
    ipel[1] = TURN3(i  , 0, width_minus1);
    ipel[2] = TURN3(i+1, 0, width_minus1);
    ipel[3] = TURN3(i+2, 0, width_minus1);

    opel[0] = (i+padding_size)<<1;
    opel[1] = opel[0]+1;

    for(h=0; h<height; h++)
    {
      out[h][opel[0]] = in[h][ipel[1]];
      out[h][opel[1]] = CLIP3( (5*(in[h][ipel[1]] +    in[h][ipel[2]])-   in[h][ipel[0]] -   in[h][ipel[3]] +  4)>>3, 0, 255 );
    }
  }
}

void HorizontalCubicFilter_1D_qpel(BYTE *in, BYTE *out, int width, int padding_size)
{
  int i, ipel[4], opel[4];
  int max_width = width + padding_size;
  int width_minus1 = width-1;

  for(i=-padding_size; i<max_width; i++)
  {
//    ipel[0] = CLIP3(i-1, 0, width_minus1);
//    ipel[1] = CLIP3(i  , 0, width_minus1);
//    ipel[2] = CLIP3(i+1, 0, width_minus1);
//    ipel[3] = CLIP3(i+2, 0, width_minus1);
    ipel[0] = TURN3(i-1, 0, width_minus1);
    ipel[1] = TURN3(i  , 0, width_minus1);
    ipel[2] = TURN3(i+1, 0, width_minus1);
    ipel[3] = TURN3(i+2, 0, width_minus1);

    opel[0] = (i+padding_size)<<2;
    opel[1] = opel[0]+1;
    opel[2] = opel[0]+2;
    opel[3] = opel[0]+3;

    out[opel[0]] = in[ipel[1]];
    out[opel[1]] = CLIP3( (57*in[ipel[1]] + 19*in[ipel[2]] - 9*in[ipel[0]] - 3*in[ipel[3]] + 32)>>6, 0, 255 );
    out[opel[2]] = CLIP3( (5*(in[ipel[1]] +    in[ipel[2]])-   in[ipel[0]] -   in[ipel[3]] +  4)>>3, 0, 255 );
    out[opel[3]] = CLIP3( (19*in[ipel[1]] + 57*in[ipel[2]] - 3*in[ipel[0]] - 9*in[ipel[3]] + 32)>>6, 0, 255 );
  }
}

void HorizontalCubicFilter_2D_qpel(BYTE **in, BYTE **out, int width, int height, int padding_size)
{
  int i, ipel[4], opel[4];
  int max_width = width + padding_size;
  int width_minus1 = width-1;
  int h;

  for(i=-padding_size; i<max_width; i++)
  {
//    ipel[0] = CLIP3(i-1, 0, width_minus1);
//    ipel[1] = CLIP3(i  , 0, width_minus1);
//    ipel[2] = CLIP3(i+1, 0, width_minus1);
//    ipel[3] = CLIP3(i+2, 0, width_minus1);
    ipel[0] = TURN3(i-1, 0, width_minus1);
    ipel[1] = TURN3(i  , 0, width_minus1);
    ipel[2] = TURN3(i+1, 0, width_minus1);
    ipel[3] = TURN3(i+2, 0, width_minus1);

    opel[0] = (i+padding_size)<<2;
    opel[1] = opel[0]+1;
    opel[2] = opel[0]+2;
    opel[3] = opel[0]+3;

    for(h=0; h<height; h++)
    {
      out[h][opel[0]] = in[h][ipel[1]];
      out[h][opel[1]] = CLIP3( (57*in[h][ipel[1]] + 19*in[h][ipel[2]] - 9*in[h][ipel[0]] - 3*in[h][ipel[3]] + 32)>>6, 0, 255 );
      out[h][opel[2]] = CLIP3( (5*(in[h][ipel[1]] +    in[h][ipel[2]])-   in[h][ipel[0]] -   in[h][ipel[3]] +  4)>>3, 0, 255 );
      out[h][opel[3]] = CLIP3( (19*in[h][ipel[1]] + 57*in[h][ipel[2]] - 3*in[h][ipel[0]] - 9*in[h][ipel[3]] + 32)>>6, 0, 255 );
    }
  }
}

void DummyFilter_1D(BYTE *in, BYTE *out, int width, int padding_size)
{
  return;
}

void DummyFilter_2D(BYTE **in, BYTE **out, int width, int height, int padding_size)
{
  int i;
  int h;
  int max_width = width + padding_size;
  int ipel;

  for(h=0; h<height; h++)
  for(i=-padding_size; i<max_width; i++)
  {
    ipel = CLIP3(i, 0, width);
    out[h][i+padding_size] = in[h][ipel];
  }
}


/* ************************************************
 *  class CPictureResample
 * *********************************************** */

CPictureResample::CPictureResample()
{
}

CPictureResample::~CPictureResample()
{
}

/*
 * \brief
 *    Clip the pixel value to [0, 255]
 * \param pxlVal
 *    Input pixel value
 * \return
 *    Pixel value within [0, 255]
 */
unsigned char CPictureResample::PixelClip(const int pxlVal)
{
  if (pxlVal < 0) return 0;
  if (pxlVal > 255) return 255;
  return (unsigned char) pxlVal;
}

/*
 * \brief
 *    Resampling by simple pixel repeating
 *
 * \input
 * \param Org
 *    Input image (One component, U or V) in WidthxHeight
 * \param Width, Height
 *    The resolution of the picture (UV component)
 *
 * \output
 * \param Dst
 *    Output image (One component, U or V) in 2Widthx2Height
 */
void CPictureResample::RepeatPixel420to444(unsigned char* Dst, unsigned char* Org, int Width, int Height)
{
  int x, y;
  int w, h;

  w = Width<<1;
  h = Height<<1;

  for (y = 0; y < h; y++)
  for (x = 0; x < w;  x++)
  {
    Dst[x+y*w] = Org[(x/2)+(y/2)*Width];
  }
}

/*
 * \brief
 *    Resampling by simple pixel dropping
 *
 * \input
 * \param Org
 *    Input image (One component, U or V) in 2Widthx2Height
 * \param Width, Height
 *    The resolution of the picture (UV component)
 *
 * \output
 * \param Dst
 *    Output image (One component, U or V) in WidthxHeight
 */
void CPictureResample::DropPixel444to420(unsigned char* Dst, unsigned char* Org, int Width, int Height)
{
  int x, y;
  int w, h;

  w = Width<<1;
  h = Height<<1;

  for (y = 0; y < Height; y++)
  for (x = 0; x < Width;  x++)
  {
    Dst[x+y*Width] = Org[x*2+y*2*w];
  }
}

/*
 * \brief
 *    Check if the Resampler is ready for use.
 * \return
 *    0: Ready
 *    non-zero: Not ready
 */
int CPictureResample::State()
{
  return 0;
}

/*
 * \brief
 *    Resampling from 420 to 444. WidthxHeight => 2Widthx2Height
 *
 * \input
 * \param Org
 *    Input image (One component, U or V) in WidthxHeight
 * \param Width, Height
 *    The resolution of the picture (UV component)
 *
 * \output
 * \param Dst
 *    Output image (One component, U or V) in 2Widthx2Height
 */
void CPictureResample::PictureResample420to444(unsigned char *Dst, unsigned char *Org, int Width, int Height)
{
  RepeatPixel420to444(Dst, Org, Width, Height);
}

/*
 * \brief
 *    Resampling from 444 to 420. 2Widthx2Height => WidthxHeight
 *
 * \input
 * \param Org
 *    Input image (One component, U or V) in 2Widthx2Height
 * \param Width, Height
 *    The resolution of the picture (UV component)
 *
 * \output
 * \param Dst
 *    Output image (One component, U or V) in WidthxHeight
 */
void CPictureResample::PictureResample444to420(unsigned char *Dst, unsigned char *Org, int Width, int Height)
{
  DropPixel444to420(Dst, Org, Width, Height);
}

/*
 * \brief
 *    Upsample the picture, and we will get more warped pixels in the synthesized picture
 *
 * \input
 * \param Yi
 *    The input image.  Resolution is Width, Height
 * \param Width, Height
 *    The resolution of the input picture
 *
 * \output
 * \param Yo
 *    The output image. Resolution is Width*2, Height
 *
 * \return
 *    None
 */
void CPictureResample::DoUpsample(unsigned char* Yo, unsigned char* Yi, int Width, int Height)
{
  int i, j, pel[6];
  int WidthMinus1;
  unsigned char* out;
  unsigned char* in;

  WidthMinus1 = Width-1;

  for (j = 0; j < Height; j++)
  {
    out = &Yo[j*Width*2];
    in  = &Yi[j*Width];
    for(i = 0; i < Width; i++)
    {
      pel[0] = CLIP(i-2, 0, WidthMinus1);
      pel[1] = CLIP(i-1, 0, WidthMinus1);
      pel[2] = CLIP(i  , 0, WidthMinus1);
      pel[3] = CLIP(i+1, 0, WidthMinus1);
      pel[4] = CLIP(i+2, 0, WidthMinus1);
      pel[5] = CLIP(i+3, 0, WidthMinus1);

      out[ (i)<<1   ] = in[pel[2]];
      out[((i)<<1)+1] = CLIP( (20*(in[pel[2]]+in[pel[3]]) - 5*(in[pel[1]]+in[pel[4]]) + (in[pel[0]]+in[pel[5]]) +16)>>5, 0, 255 );
    }
  }
}

/*
 * \brief
 *    Downsample the picture, and we will get more warped pixels in the synthesized picture
 *
 * \input
 * \param Yi
 *    The input image.  Resolution is Width, Height
 * \param Width, Height
 *    The resolution of the input picture
 *
 * \output
 * \param Yo
 *    The output image. Resolution is Width*2, Height
 *
 * \return
 *    None
 */
void CPictureResample::DoDownsample(unsigned char* Yo, unsigned char* Yi, int Width, int Height)
{
  int i, j, pel[6];
  int WidthMinus1;
  unsigned char* out;
  unsigned char* in;

  WidthMinus1 = Width-1;

  for (j = 0; j < Height; j++)
  {
    out = &Yo[j*Width/2];
    in  = &Yi[j*Width];
    for(i = 0; i < Width; i+=2)
    {
      pel[0] = CLIP(i-1, 0, WidthMinus1);
      pel[1] = CLIP(i  , 0, WidthMinus1);
      pel[2] = CLIP(i+1, 0, WidthMinus1);

      out[(i >> 1)] = CLIP ( (in[pel[0]] + 2*in[pel[1]] + in[pel[2]]) >> 2, 0, 255);
    }
  }
}

/*
 * \brief
 *    Upsample the picture, and we will get more warped pixels in the synthesized picture
 *
 * \input
 * \param Yi
 *    The input image.  Resolution is Width, Height
 * \param Width, Height
 *    The resolution of the input picture
 * \param Factor
 *    Must be 2 or 4. Upsample to 2 times or 4 times
 *
 * \output
 * \param Yo
 *    The output image. Resolution is Width*2, Height
 *
 * \return
 *    0: success
 *    -1: fail
 */
int CPictureResample::UpsampleView(unsigned char* Yo, unsigned char* Yi, int Width, int Height, int Factor)
{
  unsigned char* Ym;

  if (Factor == 2)
    DoUpsample(Yo, Yi, Width, Height);
  else
  {
    Ym = (unsigned char*) malloc(Width*2*Height*sizeof(unsigned char));
    if (Ym == NULL)
    {
      printf("No mem for upsample ref view.\n");
      return -1;
    }
    DoUpsample(Ym, Yi, Width, Height);
    DoUpsample(Yo, Ym, Width*2, Height);
    free(Ym);
  }

  return 0;
}

/*
 * \brief
 *    Downsample the picture, and we will get more warped pixels in the synthesized picture
 *
 * \input
 * \param Yi
 *    The input image.  Resolution is Width, Height
 * \param Width, Height
 *    The resolution of the input picture
 * \param Factor
 *    Must be 2 or 4. Downsample to 2 times or 4 times
 *
 * \output
 * \param Yo
 *    The output image. Resolution is Width*2, Height
 *
 * \return
 *    None
 */
int CPictureResample::DownsampleView(unsigned char* Yo, unsigned char* Yi, int Width, int Height, int Factor)
{
  unsigned char* Ym;

  if (Factor == 2)
    DoDownsample(Yo, Yi, Width, Height);
  else
  {
    Ym = (unsigned char*) malloc(Width/2 * Height * sizeof(unsigned char));
    if (Ym == NULL)
    {
      printf("No mem for downsample ref view.\n");
      return -1;
    }
    DoDownsample(Ym, Yi, Width, Height);
    DoDownsample(Yo, Ym, Width/2, Height);
    free(Ym);
  }

  return 0;
}

