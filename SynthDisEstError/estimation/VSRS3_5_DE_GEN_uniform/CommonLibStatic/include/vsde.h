/********************************
Header file for view synthesis
Distortion Estimation
********************************/
#ifndef VSDE_H
#define VSDE_H

#define ENABLE_VSDE 1
#define ENABLE_PROB_CHECKING 1

#define WANT_EXTERNAL_DISTRIBUTION 0

#define HISTO_SIZE 21

#define LIST_INITIAL_CAPACITY 8
/*Decided to go for a look up table approach for speed in sacrifice of memory*/
typedef struct
{
   int capacity;
   int numberOutcomes;
   double *prob;
   unsigned char *values;  /*contains a list of values this pixel maps to. It is for fast access*/
   unsigned int *IDs;      /*ID number relating to the pixel mapping to this pixel*/
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

#endif