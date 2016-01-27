/********************************
Header file for view synthesis
Distortion Estimation
********************************/
#ifndef VSDE_H
#define VSDE_H

#define ENABLE_VSDE 1

/*virtual view distortion metrics*/
typedef struct
{
   long double pixelDistortion; /*pixel distortion after an updating operation*/
   long double pixelMean;
   unsigned char updated;
}DISTORTION_DATA;

#endif