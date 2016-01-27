/********************************
Header file for view synthesis
Distortion Estimation
********************************/
#ifndef VSDE_H
#define VSDE_H

#define ENABLE_VSDE 1

/*Decided to go for a look up table approach for speed in sacrifice of memory*/
typedef struct
{
   int numberOutcomes;
   double *prob;
   unsigned char *values;  /*contains a list of values this pixel maps to. It is for fast access*/
   double noMapProb;       /*probability with no nodes mapped is a special case*/
}PROBABILITY_OUTCOME;

typedef struct
{
   int width;
   int height;
   PROBABILITY_OUTCOME **outcomes;
}PROBABILITY_MAP;

#endif