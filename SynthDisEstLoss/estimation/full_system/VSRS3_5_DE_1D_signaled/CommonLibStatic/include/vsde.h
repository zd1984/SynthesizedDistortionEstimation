/********************************
Header file for view synthesis
Distortion Estimation
********************************/
#ifndef VSDE_H
#define VSDE_H

#define ENABLE_VSDE 1
#define ENABLE_PROB_CHECKING 1

#define IMG_PAD_SIZE_X         32 //!< Number of pixels padded around the reference frame (>=4)
#define IMG_PAD_SIZE_Y         18  //!< Number of pixels padded around the reference frame (>=4)

#define LIST_INITIAL_CAPACITY 8
/*Decided to go for a look up table approach for speed in sacrifice of memory*/
typedef struct
{
   int capacity;
   int numberOutcomes;
   double *prob;
   unsigned char *Dvalues;  /*contains a list of depth values this pixel maps to. It is for fast access*/
   unsigned short *x;
   unsigned short *y;
   double noMapProb;       /*probability with no nodes mapped is a special case*/
}PROBABILITY_OUTCOME;

typedef struct
{
   int width;
   int height;
   PROBABILITY_OUTCOME **outcomes;
}PROBABILITY_MAP;

typedef struct
{
   int width;
   int height;
   double *firstMoment;
   double *secondMoment;
}PIXEL_MOMENTS;

/*Since we used shared memory model, we need to keep the structure definition in sync with the ROPE_PMF application*/
typedef struct
{
  float prob[256];
  unsigned char map[256];
} PixelPMF;
#endif