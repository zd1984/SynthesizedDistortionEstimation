/*
 *
 * View Synthesis using general mode
 *
 */

#include "ViewSynthesis.h"

#include <cvaux.h>
#include <c:\opencv\build\include\opencv2\photo\photo_c.h>
//#pragma comment(lib, "cv200.lib")
//#pragma comment(lib, "cxcore200.lib")
//#pragma comment(lib, "highgui200.lib")
//#pragma comment(lib, "cvaux200.lib")


#define ZERO 0.00001

#ifndef SAFE_RELEASE_IMAGE
#define SAFE_RELEASE_IMAGE(p) { if((p)!=NULL){ cvReleaseImage(&(p)); (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE_MAT
#define SAFE_RELEASE_MAT(p) { if((p)!=NULL){ cvReleaseMat(&(p)); (p)=NULL; } }
#endif
#ifndef SAFE_FREE
#define SAFE_FREE(p) { if((p)!=NULL){ free(p); (p)=NULL; } }
#endif

int descending(const BYTE *a, const BYTE *b)
{
  if(*a < *b) return 1;
  if(*a > *b) return -1;
  return 0;
}

int ascending(const BYTE *a, const BYTE *b)
{
  if(*a < *b) return -1;
  if(*a > *b) return 1;
  return 0;
}

  //Class name CViewSynthesis was changed to CViewInterpolationGeneral.

CViewInterpolationGeneral::CViewInterpolationGeneral()
{
  int i;

  m_uiPrecision = 1;

  m_imgVirtualImage = m_imgVirtualDepth = NULL;
  m_imgSuccessSynthesis = m_imgHoles = m_imgBound = NULL;
  m_pConvKernel = NULL;
  m_pVirtualDepth = m_pSuccessSynthesis = NULL;
  //Nagoya start
  m_pVirtualImageY = m_pVirtualImageU = m_pVirtualImageV = NULL;
  //Nagoya end
  m_aiTableDisparity_ipel = m_aiTableDisparity_subpel = NULL;
  m_pcViewSynthesisLeft = NULL;
  m_pcViewSynthesisRight = NULL;

  m_imgBlended = m_imgInterpolatedView = NULL;

#if ENABLE_VSDE
  m_depthProbMap.outcomes=NULL;
  m_textureProbMap.outcomes=NULL;
  m_mergedTextureProbMap.outcomes=NULL;
  m_distribution = NULL;
#endif

  for(i=0; i<5; i++)
  {
    m_imgTemp[i] = m_imgMask[i] = NULL;
  }

  for(i=0; i<256; i++)
  {
    m_matH_R2V[i] = m_matH_V2R[i] = NULL;
  }

  // GIST added
  if (m_imgSynLeftforBNR    != NULL) { m_imgSynLeftforBNR    = NULL;}
  if (m_imgSynRightforBNR   != NULL) { m_imgSynRightforBNR   = NULL;}
  if (m_imgDepthLeftforBNR  != NULL) { m_imgDepthLeftforBNR  = NULL;}
  if (m_imgDepthRightforBNR != NULL) { m_imgDepthRightforBNR = NULL;}
  if (m_imgHoleLeftforBNR   != NULL) { m_imgHoleLeftforBNR   = NULL;}
  if (m_imgHoleRightforBNR  != NULL) { m_imgHoleRightforBNR  = NULL;}

}

CViewInterpolationGeneral::~CViewInterpolationGeneral()
{
  xReleaseMemory();
}

void CViewInterpolationGeneral::xReleaseMemory()
{
  int i;

  SAFE_RELEASE_IMAGE(m_imgVirtualImage);
  SAFE_RELEASE_IMAGE(m_imgVirtualDepth);
  SAFE_RELEASE_IMAGE(m_imgSuccessSynthesis);
  SAFE_RELEASE_IMAGE(m_imgHoles);
  SAFE_RELEASE_IMAGE(m_imgBound);
  SAFE_RELEASE_IMAGE(m_imgBlended);
  SAFE_RELEASE_IMAGE(m_imgInterpolatedView);
  if(m_pcViewSynthesisLeft!=NULL)    delete m_pcViewSynthesisLeft;
  if(m_pcViewSynthesisRight!=NULL)  delete m_pcViewSynthesisRight;

  m_pcViewSynthesisLeft = m_pcViewSynthesisRight = NULL;

  for(i=0; i<5; i++)
  {
    SAFE_RELEASE_IMAGE(m_imgTemp[i]);
    SAFE_RELEASE_IMAGE(m_imgMask[i]);
  }

  if(m_pConvKernel!=NULL)
  {
    cvReleaseStructuringElement(&m_pConvKernel);
    m_pConvKernel = NULL;
  }

  SAFE_FREE(m_pVirtualDepth);
  //Nagoya start
  SAFE_FREE(m_pVirtualImageY);
  SAFE_FREE(m_pVirtualImageU);
  SAFE_FREE(m_pVirtualImageV);
  //Nagoya end
  SAFE_FREE(m_pSuccessSynthesis);

#if ENABLE_VSDE
  /*release depth error probability distribution*/
  SAFE_FREE(m_distribution);
  m_distribution = NULL;
  /*release depth probability map*/
  if((m_depthProbMap.outcomes!=NULL) && (m_textureProbMap.outcomes!=NULL))
  {
     /*error checking for consistency*/
     if(m_depthProbMap.height!=m_textureProbMap.height)
     {
        printf("%s error depth and texture prob map size inconsistent\n",__FUNCTION__);
     }
     for(i=0; i<m_depthProbMap.height; i++)
     {
        SAFE_FREE(m_depthProbMap.outcomes[i]);
        SAFE_FREE(m_textureProbMap.outcomes[i]);
     }
     SAFE_FREE(m_depthProbMap.outcomes);
     SAFE_FREE(m_textureProbMap.outcomes);
     m_depthProbMap.outcomes = NULL;
     m_textureProbMap.outcomes = NULL;
  }
  /*release texture probability map*/
  if(m_mergedTextureProbMap.outcomes!=NULL)
  {
     for(i=0; i<m_mergedTextureProbMap.height; i++)
     {
        SAFE_FREE(m_mergedTextureProbMap.outcomes[i]);
     }
     SAFE_FREE(m_mergedTextureProbMap.outcomes);
     m_mergedTextureProbMap.outcomes = NULL;
  }
#endif

  for(i=0; i<256; i++)
  {
    SAFE_RELEASE_MAT( m_matH_R2V[i] );
    SAFE_RELEASE_MAT( m_matH_V2R[i] );
  }

  SAFE_FREE(m_aiTableDisparity_ipel);
  SAFE_FREE(m_aiTableDisparity_subpel);

  // GIST added
  SAFE_RELEASE_IMAGE(m_imgSynLeftforBNR    );
  SAFE_RELEASE_IMAGE(m_imgSynRightforBNR   );
  SAFE_RELEASE_IMAGE(m_imgDepthLeftforBNR  );
  SAFE_RELEASE_IMAGE(m_imgDepthRightforBNR );
  SAFE_RELEASE_IMAGE(m_imgHoleLeftforBNR   );
  SAFE_RELEASE_IMAGE(m_imgHoleRightforBNR  );

}

void CViewInterpolationGeneral::convertCameraParam(CvMat *exMat_dst, CvMat *exMat_src)
{
  int i, j;
  double val;

  for(i=0; i<3; i++)
  {
    for(j=0; j<3; j++)
    {
      cvmSet(exMat_dst, i, j, cvmGet(exMat_src, j, i));
    }
  }

  for(i=0; i<3; i++)
  {
    val = 0.0;
    for(j=0; j<3; j++)
    {
      val -= cvmGet(exMat_dst, i, j)*cvmGet(exMat_src, j, 3);
    }
    cvmSet(exMat_dst, i, 3, val);
  }
}

void CViewInterpolationGeneral::cvexMedian(IplImage* dst)
{
  IplImage* buf = cvCloneImage(dst);
  cvSmooth(buf, dst, CV_MEDIAN);
  cvReleaseImage(&buf);
}

void CViewInterpolationGeneral::cvexBilateral(IplImage* dst, int sigma_d, int sigma_c)
{
  IplImage* buf = cvCloneImage(dst);
  cvSmooth(buf, dst, CV_BILATERAL, sigma_d, sigma_c);
  cvReleaseImage(&buf);
}

void CViewInterpolationGeneral::erodebound(IplImage* bound, int flag)
{
  int width = bound->width;
  int height = bound->height;
  uchar *ub = (uchar *)bound->imageData;

  if(flag)
  {
    for(int j = 0; j < height; j++)
    {
      for(int i = 0; i < width; i++)
      {
        int l = i + j * width;
        if(ub[l] == 255)
        {
          int ll = l;
          while((ub[ll] == 255) && (i < width))
          {
            ub[ll] = 0;
            ll++;
            i++;
          }
          ub[ll - 1] = 255;
        }
      }
    }
  }
  else
  {
    for(int j = 0; j < height; j++)
    {
      for(int i = 0; i < width; i++)
      {
        int l = i + j * width;
        if(ub[l] == 255)
        {
          int ll = l;
          while((ub[ll] == 255) && (i < width))
          {
            ub[ll] = 0;
            ll++;
            i++;
          }
          ub[l] = 255;
        }
      }
    }
  }
}

#if ENABLE_VSDE
/*This is a Gaussian approximation*/
/*The number of bins is odd. The probability is grown from the bins on the side. In order to make the probability distribution valid, center bin take the rest of the probability*/
void zeroMeanGaussianNoiseInit(double stdev, int numberBins, double *distribution)
{
   double probabilitySum;
   int i;

   double a;
   double b;

   if(numberBins<=0)
   {
      printf("Error numberBins less than 0: %d\n",numberBins);
   }
   if((numberBins%2)==0)
   {
      printf("Error numberBins is even: %d\n",numberBins);
   }

   a=1/sqrt(2*3.14159265);
   a=a/stdev;

   b=-2*stdev*stdev;

   probabilitySum = 0;
   for(i=0; i<(numberBins>>1); i++)
   {
      double x=(double)(i-(numberBins>>1));
      distribution[i] = a*exp(x*x/b);
      distribution[numberBins-1-i] = distribution[i];
      probabilitySum = probabilitySum + distribution[i]*2;
   }
   distribution[numberBins>>1] = 1 - probabilitySum;

   /*checking*/
   probabilitySum = 0;
   printf("Probability Distribution:\n");
   for(i=0; i<numberBins; i++)
   {
      printf("Val:%d Prob:%f\n", i-(numberBins>>1), distribution[i]);
      probabilitySum = probabilitySum + distribution[i];
   }
   printf("Total probability %f\n",probabilitySum);
   if(probabilitySum!=1)
   {
      printf("Error distribution does not sum up to 1: %f\n",probabilitySum);
   }

   return;
}
#endif
  // Member function InitLR was added to class CViewInterpolationGeneral.
  // Intrinsic and Extrinsic parameters of left, right, and virtual cameras are set throught this function.
  // Mat_In_Left[9], Mat_In_Right[9], Mat_In_Virtual[9] : Intrinsic matrices of left, right, and virtual cameras
  // Mat_Ex_Left[9], Mat_Ex_Right[9], Mat_Ex_Virtual[9] : Extrinsic matrices of left, right, and virtual cameras
  // Mat_Trans_Left[3], Mat_Trans_Right[3], Mat_Trans_Virtual[3] : Translation vectors of left, right, and virtual cameras
#if ENABLE_VSDE
bool CViewInterpolationGeneral::InitLR(  UInt uiWidth, UInt uiHeight, UInt uiPrecision,
               UInt uiDepthType, double dZnearL, double dZfarL, double dZnearR, double dZfarR,
               const char *strCamParamFile, const char *strRefLCamID, const char *strRefRCamID, const char *strVirCamID,
               double Mat_In_Left[9], double Mat_Ex_Left[9], double Mat_Trans_Left[3],
               double Mat_In_Right[9], double Mat_Ex_Right[9], double Mat_Trans_Right[3],
               double Mat_In_Virtual[9], double Mat_Ex_Virtual[9], double Mat_Trans_Virtual[3], double stdev, int numberBins)
#else
bool CViewInterpolationGeneral::InitLR(  UInt uiWidth, UInt uiHeight, UInt uiPrecision,
               UInt uiDepthType, double dZnearL, double dZfarL, double dZnearR, double dZfarR,
               const char *strCamParamFile, const char *strRefLCamID, const char *strRefRCamID, const char *strVirCamID,
               double Mat_In_Left[9], double Mat_Ex_Left[9], double Mat_Trans_Left[3],
               double Mat_In_Right[9], double Mat_Ex_Right[9], double Mat_Trans_Right[3],
               double Mat_In_Virtual[9], double Mat_Ex_Virtual[9], double Mat_Trans_Virtual[3])
#endif
{
#if ENABLE_VSDE
  int h;
#endif
  m_pcViewSynthesisLeft = new CViewInterpolationGeneral();
  m_pcViewSynthesisRight = new CViewInterpolationGeneral();
  if(m_pcViewSynthesisLeft==NULL || m_pcViewSynthesisRight==NULL) return false;

  // The number of arguments of Init increased.
  // Intrinsic and Extrinsic parameters of (left or right) and virtual cameras are set throught this function.

  if(!m_pcViewSynthesisLeft->Init(uiWidth, uiHeight, uiPrecision, uiDepthType,
                                    dZnearL, dZfarL, strCamParamFile, strRefLCamID, strVirCamID,
                                    Mat_In_Left, Mat_Ex_Left, Mat_Trans_Left,
                                    Mat_In_Virtual, Mat_Ex_Virtual, Mat_Trans_Virtual
                                    ))  return false;
  if(!m_pcViewSynthesisRight->Init(uiWidth, uiHeight, uiPrecision, uiDepthType,
                                    dZnearR, dZfarR, strCamParamFile, strRefRCamID, strVirCamID,
                                    Mat_In_Right, Mat_Ex_Right, Mat_Trans_Right,
                                    Mat_In_Virtual, Mat_Ex_Virtual, Mat_Trans_Virtual
                                    ))  return false;

  double dTotalBaseline;
    m_dWeightLeft = m_pcViewSynthesisRight->getBaselineDistance();
    m_dWeightRight = m_pcViewSynthesisLeft->getBaselineDistance();
    dTotalBaseline = m_dWeightLeft + m_dWeightRight;
    m_dWeightLeft /= dTotalBaseline;
    m_dWeightRight /= dTotalBaseline;

  m_imgBlended  = cvCreateImage(cvSize(uiWidth, uiHeight), 8, 3);
  m_imgInterpolatedView = cvCreateImage(cvSize(uiWidth, uiHeight), 8, 3);
  m_imgMask[2]  = cvCreateImage(cvSize(uiWidth, uiHeight), 8, 1);
  m_imgMask[3]  = cvCreateImage(cvSize(uiWidth, uiHeight), 8, 1);
  m_imgMask[4]  = cvCreateImage(cvSize(uiWidth, uiHeight), 8, 1);

#if ENABLE_VSDE
  /*create the depth error distribution*/
  m_numberBins = numberBins;
  m_stdev = stdev;
  m_distribution = (double *)malloc(sizeof(double)*m_numberBins);
  if(m_distribution==NULL)
  {
     printf("Error %s allocating probability distribution array\n",__FUNCTION__);
     exit(0);
  }
  zeroMeanGaussianNoiseInit(stdev, numberBins, m_distribution);
  m_pcViewSynthesisLeft->setProbabilityDistribution(m_distribution, stdev, numberBins);
  m_pcViewSynthesisRight->setProbabilityDistribution(m_distribution, stdev, numberBins);

  /*allocate the merged texture probability map*/
  m_uiHeight = uiHeight;
  m_uiWidth = uiWidth;
  m_mergedTextureProbMap.width = m_uiWidth;
  m_mergedTextureProbMap.height = m_uiHeight;
  m_mergedTextureProbMap.outcomes = (PROBABILITY_OUTCOME **)malloc(sizeof(PROBABILITY_OUTCOME *)*m_uiHeight);
  if(m_mergedTextureProbMap.outcomes==NULL)
  {
     printf("Error %s allocating texture probability map memory(Vertical)\n",__FUNCTION__);
     exit(0);
  }
  for(h=0; h<m_uiHeight; h++)
  {
     m_mergedTextureProbMap.outcomes[h] = (PROBABILITY_OUTCOME *)malloc(sizeof(PROBABILITY_OUTCOME)*m_uiWidth);

     if(m_mergedTextureProbMap.outcomes[h]==NULL)
     {
        printf("Error %s allocating texture or depth probability map memory(Horizontal %d)\n", __FUNCTION__, h);
        exit(0);
     }
     memset(m_mergedTextureProbMap.outcomes[h], 0, sizeof(PROBABILITY_OUTCOME)*m_uiWidth);
  }
#endif

    return true;
}

bool CViewInterpolationGeneral::Init(  UInt uiWidth, UInt uiHeight, UInt uiPrecision,
               UInt uiDepthType, double dZnear, double dZfar,
               const char *strCamParamFile, const char *strRefCamID, const char *strVirCamID,
               double Mat_In_Ref[9], double Mat_Ex_Ref[9], double Mat_Trans_Ref[3],
               double Mat_In_Vir[9], double Mat_Ex_Vir[9], double Mat_Trans_Vir[3])
{
  double Z_near, Z_far;
  unsigned int h, pos;
  //Nagoya start
  unsigned w;
  //Nagoya end

  xReleaseMemory();

  Z_near   = dZnear;    //z_near value from camera or the origin of 3D space
  Z_far    = dZfar;    //z_far value from camera or the origin of 3D space

  m_uiPrecision = uiPrecision;
  if(m_uiPrecision!=1 && m_uiPrecision!=2 && m_uiPrecision!=4)
  {
    fprintf(stderr, "Illegal Precision setting\n");
    return false;
  }

  m_uiHeight = uiHeight;
  m_uiWidth = uiWidth;
  m_uiPicsize = m_uiHeight*m_uiWidth;
  m_uiDepthType = uiDepthType;

  m_imgVirtualImage = cvCreateImage(cvSize(m_uiWidth, m_uiHeight), 8, 3);
  m_imgVirtualDepth = cvCreateImage(cvSize(m_uiWidth, m_uiHeight), 8, 1);
  m_imgSuccessSynthesis = cvCreateImage(cvSize(m_uiWidth, m_uiHeight), 8, 1);
  m_imgHoles    = cvCreateImage(cvSize(m_uiWidth, m_uiHeight), 8, 1);
  m_imgBound    = cvCreateImage(cvSize(m_uiWidth, m_uiHeight), 8, 1);
  m_imgMask[0]  = cvCreateImage(cvSize(m_uiWidth, m_uiHeight), 8, 1);
  m_imgMask[1]  = cvCreateImage(cvSize(m_uiWidth, m_uiHeight), 8, 1);
  m_imgTemp[0]  = cvCreateImage(cvSize(m_uiWidth, m_uiHeight), 8, 1);
  m_imgTemp[1]  = cvCreateImage(cvSize(m_uiWidth, m_uiHeight), 8, 1);

  if((m_pVirtualDepth = (BYTE **)malloc(m_uiHeight*sizeof(BYTE *)))==NULL) return false;
  for(h=pos=0; h<m_uiHeight; h++, pos+=m_imgVirtualDepth->widthStep)
  {
    m_pVirtualDepth[h] = (BYTE *) &(m_imgVirtualDepth->imageData[pos]);
  }

  // Nagoya start
  if((m_pVirtualImageY = (BYTE **)malloc(m_uiHeight*sizeof(BYTE *)))==NULL
     || (m_pVirtualImageU = (BYTE **)malloc(m_uiHeight*sizeof(BYTE *)))==NULL
     || (m_pVirtualImageV = (BYTE **)malloc(m_uiHeight*sizeof(BYTE *)))==NULL ) return false;

  for (int j=0;j<m_uiHeight;j++) {
     if((m_pVirtualImageY[j] = (BYTE *)malloc(m_imgSuccessSynthesis->widthStep*sizeof(BYTE)))==NULL
        || (m_pVirtualImageU[j] = (BYTE *)malloc(m_imgSuccessSynthesis->widthStep*sizeof(BYTE)))==NULL
        || (m_pVirtualImageV[j] = (BYTE *)malloc(m_imgSuccessSynthesis->widthStep*sizeof(BYTE)))==NULL) return false;
  }

  for(h=0; h<m_uiHeight; h++)
     for(w=0; w<m_uiWidth; w++)
  {
   m_pVirtualImageY[h][w] = (BYTE) (m_imgVirtualImage->imageData[(h*m_uiWidth+w)*3]);
   m_pVirtualImageU[h][w] = (BYTE) (m_imgVirtualImage->imageData[(h*m_uiWidth+w)*3+1]);
   m_pVirtualImageV[h][w] = (BYTE) (m_imgVirtualImage->imageData[(h*m_uiWidth+w)*3+2]);
  }
  // Nagoya end

  if((m_pSuccessSynthesis = (BYTE **)malloc(m_uiHeight*sizeof(BYTE *)))==NULL) return false;
  for(h=pos=0; h<m_uiHeight; h++, pos+=m_imgSuccessSynthesis->widthStep)
  {
    m_pSuccessSynthesis[h] = (BYTE *) &(m_imgSuccessSynthesis->imageData[pos]);
  }

#if ENABLE_VSDE
  m_depthProbMap.width = m_uiWidth;
  m_depthProbMap.height = m_uiHeight;
  m_depthProbMap.outcomes = (PROBABILITY_OUTCOME **)malloc(sizeof(PROBABILITY_OUTCOME *)*m_uiHeight);

  m_textureProbMap.width = m_uiWidth;
  m_textureProbMap.height = m_uiHeight;
  m_textureProbMap.outcomes = (PROBABILITY_OUTCOME **)malloc(sizeof(PROBABILITY_OUTCOME *)*m_uiHeight);

  if((m_depthProbMap.outcomes==NULL) || (m_textureProbMap.outcomes==NULL))
  {
     printf("Error %s allocating texture or depth probability map memory(Vertical)\n",__FUNCTION__);
     exit(0);
  }

  for(h=0; h<m_uiHeight; h++)
  {
     m_depthProbMap.outcomes[h] = (PROBABILITY_OUTCOME *)malloc(sizeof(PROBABILITY_OUTCOME)*m_uiWidth);
     m_textureProbMap.outcomes[h] = (PROBABILITY_OUTCOME *)malloc(sizeof(PROBABILITY_OUTCOME)*m_uiWidth);
     if((m_depthProbMap.outcomes[h]==NULL) || (m_textureProbMap.outcomes[h]==NULL))
     {
        printf("Error %s allocating texture or depth probability map memory(Horizontal %d)\n", __FUNCTION__, h);
        exit(0);
     }
     memset(m_depthProbMap.outcomes[h], 0, sizeof(PROBABILITY_OUTCOME)*m_uiWidth);
     memset(m_textureProbMap.outcomes[h], 0, sizeof(PROBABILITY_OUTCOME)*m_uiWidth);
  }
#endif

  if(uiPrecision==1)
  {
    m_pFunc_DepthSynthesis = &CViewInterpolationGeneral::depthsynthesis_3Dwarp_ipel;
    m_pFunc_ViewSynthesisReverse = &CViewInterpolationGeneral::viewsynthesis_reverse_3Dwarp_ipel;
  }
  else
  {
    m_pFunc_DepthSynthesis = &CViewInterpolationGeneral::depthsynthesis_3Dwarp;
    m_pFunc_ViewSynthesisReverse = &CViewInterpolationGeneral::viewsynthesis_reverse_3Dwarp;

  }
  return init_3Dwarp(  Z_near, Z_far, uiDepthType, strCamParamFile, strRefCamID, strVirCamID,
                        Mat_In_Ref, Mat_Ex_Ref, Mat_Trans_Ref, Mat_In_Vir, Mat_Ex_Vir, Mat_Trans_Vir);

  return true;
}

bool CViewInterpolationGeneral::init_camera_param(double Mat_In_Ref[9], double Mat_Ex_Ref[9], double Mat_Trans_Ref[3], double Mat_In_Vir[9], double Mat_Ex_Vir[9], double Mat_Trans_Vir[3],
                                       CvMat *mat_in[2], CvMat *mat_ex_c2w[2], CvMat *mat_proj[2])
{
  int i, j;
  CvMat *mat_ex_w2c;

  for(i=0; i<3; i++)
  {
    for(j=0; j<3;j++)
    {
      mat_in[0]->data.db[j+3*i] = *(Mat_In_Ref+3*i+j);
      mat_in[1]->data.db[j+3*i] = *(Mat_In_Vir+3*i+j);
    }
  }
  for(i=0; i<3; i++)
  {
    for(j=0; j<3;j++)
    {
      mat_ex_c2w[0]->data.db[j+4*i] = *(Mat_Ex_Ref+3*i+j);
      mat_ex_c2w[1]->data.db[j+4*i] = *(Mat_Ex_Vir+3*i+j);
    }
  }
  for(i=0; i<3; i++)
  {
    mat_ex_c2w[0]->data.db[3+4*i] = *(Mat_Trans_Ref+i);
    mat_ex_c2w[1]->data.db[3+4*i] = *(Mat_Trans_Vir+i);
  }

  if(mat_proj!=NULL)
  {
    mat_ex_w2c = cvCreateMat(3, 4, CV_64F);
    for(i=0; i<2; i++)
    {
      convertCameraParam(mat_ex_w2c, mat_ex_c2w[i]);
      cvmMul(mat_in[i], mat_ex_w2c, mat_proj[i]);  // Proj = inMat_c2i * exMat_w2c
    }
    cvReleaseMat(&mat_ex_w2c);
  }

  return true;
}

void CViewInterpolationGeneral::image2world_with_z(CvMat *mat_Rc2w_invIN_from, CvMat *matEX_c2w_from, CvMat *image, CvMat *world)
{
  CvMat *temp;
  double s;

  temp = cvCreateMat(3, 1, CV_64F);

  cvmMul(mat_Rc2w_invIN_from, image, temp);

  s = ( cvmGet(world, 2, 0) - cvmGet(matEX_c2w_from, 2, 3) ) / cvmGet(temp, 2, 0);

  cvmSet(world, 0, 0, s*cvmGet(temp, 0, 0) + cvmGet(matEX_c2w_from, 0, 3));
  cvmSet(world, 1, 0, s*cvmGet(temp, 1, 0) + cvmGet(matEX_c2w_from, 1, 3));

  cvReleaseMat(&temp);
}

void CViewInterpolationGeneral::makeHomography(CvMat *matH_F2T[256], CvMat *matH_T2F[256], double adTable[256],
                                    CvMat *matIN_from, CvMat *matEX_c2w_from, CvMat *matProj_to)
{
  int i, j, k;
  double val, u, v;
  CvMat *matIN_inv_from, *mat_Rc2w_invIN_from;

  CvMat *src_points  = cvCreateMat(4, 2, CV_64F);
  CvMat *dst_points  = cvCreateMat(4, 2, CV_64F);
  CvMat *image      = cvCreateMat(3, 1, CV_64F);
  CvMat *world      = cvCreateMat(4, 1, CV_64F);

  matIN_inv_from      = cvCreateMat(3, 3, CV_64F);
  mat_Rc2w_invIN_from  = cvCreateMat(3, 3, CV_64F);

  cvInvert(matIN_from, matIN_inv_from, CV_LU);
  for(i=0; i<3; i++)
  {
    for(j=0; j<3; j++)
    {
      val=0.0;
      for(k=0; k<3; k++)
      {
        val += cvmGet(matEX_c2w_from, i, k)*cvmGet(matIN_inv_from, k, j);
      }
      cvmSet(mat_Rc2w_invIN_from, i, j, val);
    }
  }

  for(i=0; i<256; i++)
  {
    matH_F2T[i] = cvCreateMat(3, 3, CV_64F);
    matH_T2F[i] = cvCreateMat(3, 3, CV_64F);

    cvmSet(world, 2, 0, adTable[i]);
    cvmSet(world, 3, 0, 1.0);

    // (u, v) = (0, 0)
    u = 0.0; v= 0.0;
    cvmSet(image, 0, 0, u);
    cvmSet(image, 1, 0, v);
    cvmSet(image, 2, 0, 1.0);
    image2world_with_z(mat_Rc2w_invIN_from, matEX_c2w_from, image, world);
    cvmMul(matProj_to, world, image);
    cvmSet(src_points, 0, 0, u);
    cvmSet(src_points, 0, 1, v);
    cvmSet(dst_points, 0, 0, cvmGet(image, 0, 0) / cvmGet(image, 2, 0));
    cvmSet(dst_points, 0, 1, cvmGet(image, 1, 0) / cvmGet(image, 2, 0));

    // (u, v) = (0, height-1)
    u = 0.0; v = m_uiHeight - 1;
    cvmSet(image, 0, 0, u);
    cvmSet(image, 1, 0, v);
    cvmSet(image, 2, 0, 1.0);
    image2world_with_z(mat_Rc2w_invIN_from, matEX_c2w_from, image, world);
    cvmMul(matProj_to, world, image);
    cvmSet(src_points, 1, 0, u);
    cvmSet(src_points, 1, 1, v);
    cvmSet(dst_points, 1, 0, cvmGet(image, 0, 0) / cvmGet(image, 2, 0));
    cvmSet(dst_points, 1, 1, cvmGet(image, 1, 0) / cvmGet(image, 2, 0));

    // (u, v) = (width-1, height-1)
    u = m_uiWidth - 1; v = m_uiHeight - 1;
    cvmSet(image, 0, 0, u);
    cvmSet(image, 1, 0, v);
    cvmSet(image, 2, 0, 1.0);
    image2world_with_z(mat_Rc2w_invIN_from, matEX_c2w_from, image, world);
    cvmMul(matProj_to, world, image);
    cvmSet(src_points, 2, 0, u);
    cvmSet(src_points, 2, 1, v);
    cvmSet(dst_points, 2, 0, cvmGet(image, 0, 0) / cvmGet(image, 2, 0));
    cvmSet(dst_points, 2, 1, cvmGet(image, 1, 0) / cvmGet(image, 2, 0));

    // (u, v) = (width-1, 0)
    u = m_uiWidth - 1; v = 0.0;
    cvmSet(image, 0, 0, u);
    cvmSet(image, 1, 0, v);
    cvmSet(image, 2, 0, 1.0);
    image2world_with_z(mat_Rc2w_invIN_from, matEX_c2w_from, image, world);
    cvmMul(matProj_to, world, image);
    cvmSet(src_points, 3, 0, u);
    cvmSet(src_points, 3, 1, v);
    cvmSet(dst_points, 3, 0, cvmGet(image, 0, 0) / cvmGet(image, 2, 0));
    cvmSet(dst_points, 3, 1, cvmGet(image, 1, 0) / cvmGet(image, 2, 0));

    cvFindHomography(src_points, dst_points, matH_F2T[i]);
    cvInvert(matH_F2T[i], matH_T2F[i], CV_LU);

  }

  cvReleaseMat(&src_points);
  cvReleaseMat(&dst_points);
  cvReleaseMat(&image);
  cvReleaseMat(&world);
  cvReleaseMat(&matIN_inv_from);
  cvReleaseMat(&mat_Rc2w_invIN_from);
}

bool CViewInterpolationGeneral::init_3Dwarp(double Z_near, double Z_far, unsigned int uiDepthType,
                                  const char *strCamParamFile, const char *strRefCamID, const char *strVirCamID,
                                  double Mat_In_Ref[9], double Mat_Ex_Ref[9], double Mat_Trans_Ref[3],
                                  double Mat_In_Vir[9], double Mat_Ex_Vir[9], double Mat_Trans_Vir[3])
{
  int i;

  CvMat* lMat_in[2]; //intrinsic parameter of left camera 3x3 matrix
  CvMat* lMat_ex_c2w[2]; //extrinsic parameter of left camera 3x4 matrix
  CvMat* lMat_proj_w2i[2]; // projection matrix from world to image

  double tableD2Z[256];
  double distance, temp1, temp2;

  for(i=0; i<2; i++)
  {
    lMat_in[i] = cvCreateMat(3, 3, CV_64F); //intrinsic parameter of camera (3x3 matrix)
    lMat_ex_c2w[i] = cvCreateMat(3, 4, CV_64F); //extrinsic parameter of camera (3x4 matrix)
    lMat_proj_w2i[i] = cvCreateMat(3, 4, CV_64F); // projection matrix
  }

  if( !init_camera_param(Mat_In_Ref, Mat_Ex_Ref, Mat_Trans_Ref, Mat_In_Vir, Mat_Ex_Vir, Mat_Trans_Vir, lMat_in, lMat_ex_c2w, lMat_proj_w2i) )
  {
    for(i=0; i<2; i++)
    {
      cvReleaseMat(&lMat_in[i]);
      cvReleaseMat(&lMat_ex_c2w[i]);
      cvReleaseMat(&lMat_proj_w2i[i]);
    }
    return false;
  }

  int kernel[49]={0,0,1,1,1,0,0,
                  0,1,1,1,1,1,0,
                  1,1,1,1,1,1,1,
                  1,1,1,1,1,1,1,
                  1,1,1,1,1,1,1,
                  0,1,1,1,1,1,0,
                  0,0,1,1,1,0,0};
  m_pConvKernel = cvCreateStructuringElementEx(7, 7, 3, 3, CV_SHAPE_CUSTOM, kernel);

  temp1 = 1.0/Z_near - 1.0/Z_far;
  temp2 = 1.0/Z_far;

  switch(uiDepthType)
  {
  case 0:
    for(i=0; i<256; i++)
    {
      distance = 1.0 / ( double(i)*temp1/255.0 + temp2 );
      tableD2Z[i] = cvmGet(lMat_ex_c2w[0], 2, 2) * distance + cvmGet(lMat_ex_c2w[0], 2, 3);
    }
    break;
  case 1:
    for(i=0; i<256; i++)
    {
      tableD2Z[i] = 1.0 / ( double(i)*temp1/255.0 + temp2 );
    }
    break;
  default:
    return false;
  }

  makeHomography(m_matH_R2V, m_matH_V2R, tableD2Z, lMat_in[0], lMat_ex_c2w[0], lMat_proj_w2i[1]);

  // need to be modify (currently only the special case is supported)
  // All cameras must be placed on the x-axis and their direction be the z-axis (either positive or negative)
  if( cvmGet(lMat_ex_c2w[0], 0, 3) < cvmGet(lMat_ex_c2w[1], 0, 3) )
    m_ucLeftSide = 1;
  else
    m_ucLeftSide = 0;

  m_dBaselineDistance =  pow( (lMat_ex_c2w[0]->data.db[ 3] - lMat_ex_c2w[1]->data.db[ 3]), 2.0 ) +
                        pow( (lMat_ex_c2w[0]->data.db[ 7] - lMat_ex_c2w[1]->data.db[ 7]), 2.0 ) +
                        pow( (lMat_ex_c2w[0]->data.db[11] - lMat_ex_c2w[1]->data.db[11]), 2.0 );
  m_dBaselineDistance = sqrt(m_dBaselineDistance);

  for(i=0; i<2; i++)
  {
    cvReleaseMat(&lMat_in[i]);
    cvReleaseMat(&lMat_ex_c2w[i]);
    cvReleaseMat(&lMat_proj_w2i[i]);
  }

  return true;
}

bool CViewInterpolationGeneral::xSynthesizeView(BYTE ***src, BYTE **pDepthMap, int th_same_depth)
{
  if( (this->*m_pFunc_DepthSynthesis)(pDepthMap) )
  {
    return (this->*m_pFunc_ViewSynthesisReverse)(src, pDepthMap, th_same_depth);
  }

  return false;
}

bool CViewInterpolationGeneral::xSynthesizeDepth(BYTE **pDepthMap)
{
  return (this->*m_pFunc_DepthSynthesis)(pDepthMap);
}

bool CViewInterpolationGeneral::xSynthesizeView_reverse(BYTE ***src, BYTE **pDepth, int th_same_depth)
{
  return (this->*m_pFunc_ViewSynthesisReverse)(src, pDepth, th_same_depth);
}

bool CViewInterpolationGeneral::depthsynthesis_3Dwarp(BYTE **pDepthMap)
{
  int h, w, u, v;
  int window_size = 3;

  CvMat* m = cvCreateMat(3, 1, CV_64F);
  CvMat* mv = cvCreateMat(3, 1, CV_64F);

  cvZero(m_imgVirtualDepth);
  cvZero(m_imgSuccessSynthesis);

  for(h=0; h<m_uiHeight; h++)
  {
    for(w=0; w<m_uiWidth; w++)
    {
      cvmSet(m, 0, 0, w);
      cvmSet(m, 1, 0, h);
      cvmSet(m, 2, 0, 1);
      cvmMul(m_matH_R2V[pDepthMap[h][w]], m, mv);
      u = mv->data.db[0] / mv->data.db[2] + 0.5;
      v = mv->data.db[1] / mv->data.db[2] + 0.5;
      if(u>=0 && u<m_uiWidth && v>=0 && v<m_uiHeight && m_pVirtualDepth[v][u] <= pDepthMap[h][w])
      {
        m_pVirtualDepth[v][u] = pDepthMap[h][w];
        m_pSuccessSynthesis[v][u] = 255;
      }
    }
  }

#if 0 // original median filter
  median_filter_depth(m_imgVirtualDepth, m_imgTemp[0], m_imgSuccessSynthesis, m_imgMask[0], 1, 1, true);
  median_filter_depth(m_imgTemp[0], m_imgVirtualDepth, m_imgMask[0], m_imgSuccessSynthesis, 1, 1, true);
#else
  cvNot(m_imgSuccessSynthesis, m_imgHoles); // m_imgHoles express holes before smoothing
  cvSmooth(m_imgHoles, m_imgTemp[0], CV_MEDIAN, window_size); // m_imgTemp[0] express holes after smoothing
  cvAnd(m_imgSuccessSynthesis, m_imgTemp[0], m_imgMask[0]); // holes which were not holes before smoothing
  cvCopy(m_imgHoles, m_imgTemp[0], m_imgMask[0]); // m_imgTemp[0] express holes before 2nd smoothing

  cvNot(m_imgTemp[0], m_imgSuccessSynthesis); // m_imgSuccessSynthesis express non-holes before 2nd smoothing
  cvSmooth(m_imgTemp[0], m_imgHoles, CV_MEDIAN, window_size); // m_imgHoles express holes after 2nd smoothing
  cvAnd(m_imgSuccessSynthesis, m_imgHoles, m_imgMask[1]); // holes which were not holes before 2nd smoothing
  cvCopy(m_imgTemp[0], m_imgHoles, m_imgMask[1]); // m_imgHoles express holes after 2nd smoothing

//cvSaveImage("1.bmp", m_imgVirtualDepth);
  cvSmooth(m_imgVirtualDepth, m_imgTemp[1], CV_MEDIAN, window_size); // 1st 3x3 median
  cvCopy(m_imgVirtualDepth, m_imgTemp[1], m_imgMask[0]);
//cvSaveImage("2.bmp", m_imgTemp[1]);
  cvSmooth(m_imgTemp[1], m_imgVirtualDepth, CV_MEDIAN, window_size); // 2nd 3x3 median
  cvCopy(m_imgTemp[1], m_imgVirtualDepth, m_imgMask[1]);
//cvSaveImage("3.bmp", m_imgVirtualDepth);

  cvSmooth(m_imgHoles, m_imgTemp[0], CV_MEDIAN, window_size); // m_imgTemp[0] express holes after smoothing
  cvAnd(m_imgSuccessSynthesis, m_imgTemp[0], m_imgMask[0]); // holes which were not holes before smoothing
  cvCopy(m_imgHoles, m_imgTemp[0], m_imgMask[0]); // m_imgTemp[0] express holes before 2nd smoothing

  cvNot(m_imgTemp[0], m_imgSuccessSynthesis); // m_imgSuccessSynthesis express non-holes before 2nd smoothing
  cvSmooth(m_imgTemp[0], m_imgHoles, CV_MEDIAN, window_size); // m_imgHoles express holes after 2nd smoothing
  cvAnd(m_imgSuccessSynthesis, m_imgHoles, m_imgMask[1]); // holes which were not holes before 2nd smoothing
  cvCopy(m_imgTemp[0], m_imgHoles, m_imgMask[1]); // m_imgHoles express holes after 2nd smoothing

  cvSmooth(m_imgVirtualDepth, m_imgTemp[1], CV_MEDIAN, window_size); // 3rd 3x3 median
  cvCopy(m_imgVirtualDepth, m_imgTemp[1], m_imgMask[0]);
//cvSaveImage("4.bmp", m_imgTemp[1]);
  cvSmooth(m_imgTemp[1], m_imgVirtualDepth, CV_MEDIAN, window_size); // 4th 3x3 median
  cvCopy(m_imgTemp[1], m_imgVirtualDepth, m_imgMask[1]);
//cvSaveImage("5.bmp", m_imgVirtualDepth);


  cvNot(m_imgHoles, m_imgSuccessSynthesis);
#endif

  cvReleaseMat(&m);
  cvReleaseMat(&mv);

  return true;
}

bool CViewInterpolationGeneral::viewsynthesis_reverse_3Dwarp(BYTE ***src, BYTE **pDepthMap, int th_same_depth)
{
  int ptv, u, v;
  int h, w;
  int maxWidth = m_uiWidth*m_uiPrecision;

  CvMat* m = cvCreateMat(3, 1, CV_64F);
  CvMat* mv = cvCreateMat(3, 1, CV_64F);

  cvZero(m_imgVirtualImage);

  for(h=0; h<m_uiHeight; h++)
  {
    for(w=0; w<m_uiWidth; w++)
    {
      if(m_pSuccessSynthesis[h][w]==0) continue;

      ptv = w + h * m_uiWidth;
      cvmSet(mv, 0, 0, w);
      cvmSet(mv, 1, 0, h);
      cvmSet(mv, 2, 0, 1);

      cvmMul(m_matH_V2R[m_pVirtualDepth[h][w]], mv, m);
      u = m->data.db[0] * m_uiPrecision / m->data.db[2] + 0.5;
      v = m->data.db[1] / m->data.db[2] + 0.5;
//      if(u>=0 && u<maxWidth && v>=0 && v<height && byte_abs[pDepthMap[v][u/m_uiPrecision]-m_pVirtualDepth[h][w]]<th_same_depth)
      if(u>=0 && u<maxWidth && v>=0 && v<m_uiHeight)
      {
        m_imgVirtualImage->imageData[ptv * 3     ] = src[0][v][u];
        m_imgVirtualImage->imageData[ptv * 3 + 1 ] = src[1][v][u];
        m_imgVirtualImage->imageData[ptv * 3 + 2 ] = src[2][v][u];
      }
      else
      {
        m_pSuccessSynthesis[h][w]=0;
      }
    }
  }
  cvReleaseMat(&m);
  cvReleaseMat(&mv);

  cvNot(m_imgSuccessSynthesis, m_imgHoles); // pixels which couldn't be synthesized
  cvCopy(m_imgHoles, m_imgBound);
  erodebound(m_imgBound, m_ucLeftSide);              // background-side boundary of holes
  cvDilate(m_imgBound, m_imgBound, m_pConvKernel, 1);  // dilate by using circle-shape kernel
  cvOr(m_imgHoles, m_imgBound, m_imgMask[0]);    // pixels which want to be modified by other synthesized images


  return true;
}

int CViewInterpolationGeneral::median_filter_depth(IplImage *srcDepth, IplImage *dstDepth,
                                        IplImage *srcMask,  IplImage *dstMask, int sizeX, int sizeY, bool bSmoothing)
{
  int num;
  int h, w, i, j, ret=0;
  int size = (2*sizeX+1)*(2*sizeY+1);
  int th = size/2;
  BYTE buf[25];
  BYTE *pSrcDepth[5], *pDstDepth[5], *pSrcMask[5], *pDstMask[5];

#ifdef _DEBUG
  if(sizeY>2 || size>25) return -1;
#endif

  for(h=0; h<m_uiHeight; h++)
  {
    for(i=-sizeY; i<=sizeY; i++)
    {
      if(h+i<0 || h+i>=m_uiHeight) continue;
      pSrcDepth[i+sizeY] = (BYTE *) &(srcDepth->imageData[ (h+i)*srcDepth->widthStep ]);
      pDstDepth[i+sizeY] = (BYTE *) &(dstDepth->imageData[ (h+i)*srcDepth->widthStep ]);
      pSrcMask[i+sizeY] = (BYTE *) &(srcMask->imageData[ (h+i)*srcDepth->widthStep ]);
      pDstMask[i+sizeY] = (BYTE *) &(dstMask->imageData[ (h+i)*srcDepth->widthStep ]);
    }

    for(w=0; w<m_uiWidth; w++)
    {
      pDstDepth[sizeY][w] = pSrcDepth[sizeY][w];
      pDstMask[sizeY][w] = pSrcMask[sizeY][w];

      if(pSrcMask[sizeY][w]!=0 && !bSmoothing) continue;

      num=0;
      for(i=-sizeY; i<=sizeY; i++)
      {
        if(h+i<0 || h+i>=m_uiHeight) continue;
        for(j=-sizeX; j<=sizeX; j++)
        {
          if(w+j<0 || w+j>=m_uiWidth) continue;
          if(pSrcMask[i+sizeY][w+j]==0) continue;

          buf[num] = pSrcDepth[i+sizeY][w+j];
          num++;
        }
      }
      if(num>th)
      {
        qsort(buf, num, sizeof(BYTE), (int (*)(const void*, const void*))descending);
        num /= 2;
        pDstDepth[sizeY][w] = buf[num];
        pDstMask[sizeY][w] = 255;
        ret++;
      }
    }
  }
  return ret;
}

int CViewInterpolationGeneral::median_filter_depth_wCheck(  IplImage *srcDepth, IplImage *dstDepth,
                                                IplImage *srcMask,  IplImage *dstMask,
                                                int sizeX, int sizeY, bool bSmoothing, int th_same_plane)
{
  int num;
  int h, w, i, j, ret=0;
  int size = (2*sizeX+1)*(2*sizeY+1);
  int th = size/2;
  BYTE buf[25];
  BYTE *pSrcDepth[5], *pDstDepth[5], *pSrcMask[5], *pDstMask[5];

#ifdef _DEBUG
  if(sizeY>2 || size>25) return -1;
#endif

  for(h=0; h<m_uiHeight; h++)
  {
    for(i=-sizeY; i<=sizeY; i++)
    {
      if(h+i<0 || h+i>=m_uiHeight) continue;
      pSrcDepth[i+sizeY] = (BYTE *) &(srcDepth->imageData[ (h+i)*srcDepth->widthStep ]);
      pDstDepth[i+sizeY] = (BYTE *) &(dstDepth->imageData[ (h+i)*srcDepth->widthStep ]);
      pSrcMask[i+sizeY] = (BYTE *) &(srcMask->imageData[ (h+i)*srcDepth->widthStep ]);
      pDstMask[i+sizeY] = (BYTE *) &(dstMask->imageData[ (h+i)*srcDepth->widthStep ]);
    }

    for(w=0; w<m_uiWidth; w++)
    {
      pDstDepth[sizeY][w] = pSrcDepth[sizeY][w];
      pDstMask[sizeY][w] = pSrcMask[sizeY][w];

      if(pSrcMask[sizeY][w]!=0 && !bSmoothing) continue;

      num=0;
      for(i=-sizeY; i<=sizeY; i++)
      {
        if(h+i<0 || h+i>=m_uiHeight) continue;
        for(j=-sizeX; j<=sizeX; j++)
        {
          if(w+j<0 || w+j>=m_uiWidth) continue;
          if(pSrcMask[i+sizeY][w+j]==0) continue;

          buf[num] = pSrcDepth[i+sizeY][w+j];
          num++;
        }
      }
      if(num>th)
      {
        qsort(buf, num, sizeof(BYTE), (int (*)(const void*, const void*))descending);
        num /= 2;

        if( abs(int(pDstDepth[sizeY][w]) - int(buf[num])) < th_same_plane )
        {
          pDstDepth[sizeY][w] = buf[num];
          pDstMask[sizeY][w] = 255;
          ret++;
        }
      }
    }
  }
  return ret;
}

#if ENABLE_VSDE
/*A connection is made to node*/
/*Value: value propagated over*/
/*prob: probability propagated over*/
void addToProbabilityMap(PROBABILITY_OUTCOME *node, BYTE value, double prob)
{
   double *newProb;
   unsigned char *newValues;
   int i;

   newProb = (double *)malloc(sizeof(double)*(node->numberOutcomes+1));
   newValues = (unsigned char*)malloc(sizeof(unsigned char)*(node->numberOutcomes+1));
   if(node->prob!=NULL)
   {
      memcpy(newProb, node->prob, sizeof(double)*node->numberOutcomes);
   }
   if(node->values!=NULL)
   {
      memcpy(newValues, node->values, sizeof(unsigned char)*node->numberOutcomes);
   }
   newProb[node->numberOutcomes] = prob;
   newValues[node->numberOutcomes] = value;
   if(node->prob!=NULL)
   {
      free(node->prob);
   }
   if(node->values!=NULL)
   {
      free(node->values);
   }

   node->prob = newProb;
   node->values = newValues;
   node->numberOutcomes++;
}

typedef struct
{
   double prob;
   unsigned char value;
}PROB_VAL_PAIR;
int compareFunction(const void* p1, const void* p2)
{
   PROB_VAL_PAIR *temp1 = (PROB_VAL_PAIR *)p1;
   PROB_VAL_PAIR *temp2 = (PROB_VAL_PAIR *)p2;

   if(temp1->value==temp2->value)
   {
      return 0;
   }
   if(temp1->value<temp2->value)
   {
      return -1;
   }
   if(temp1->value>temp2->value)
   {
      return 1;
   }
}
double tempProb[256];
unsigned int uniqueValues[256];
void processProbabilityMap(PROBABILITY_OUTCOME *node, bool debugFlag)
{
   int numberUniqueValues;
   PROB_VAL_PAIR *pairs;
   int i;
   int j;

   /*put the value and corresponding probabilities in a structure so we can qsort them*/
   pairs = (PROB_VAL_PAIR *)malloc(sizeof(PROB_VAL_PAIR)*node->numberOutcomes);
   memset(pairs, 0, sizeof(PROB_VAL_PAIR)*node->numberOutcomes);
   for(i=0; i<node->numberOutcomes; i++)
   {
      if(debugFlag)//debug
      {
         printf("%u %f\n",node->values[i], node->prob[i]);
      }
      pairs[i].prob = node->prob[i];
      pairs[i].value = node->values[i];
   }
   qsort(pairs, node->numberOutcomes, sizeof(PROB_VAL_PAIR), compareFunction);
      if(debugFlag)//debug
      {
         printf("\n\n");
      }
   numberUniqueValues = 0;
   memset(tempProb, 0, sizeof(double)*256);
   /*figure out the probabilities*/
   for(i=0; i<node->numberOutcomes; i++)
   {
      double backwardProb = pairs[i].prob;
      for(j=i+1; j<node->numberOutcomes; j++)
      {
         backwardProb = backwardProb*(1-pairs[j].prob);
      }
      if(tempProb[pairs[i].value]==0)
      {
         uniqueValues[numberUniqueValues] = pairs[i].value;
         numberUniqueValues++;
      }
      tempProb[pairs[i].value] = tempProb[pairs[i].value] + backwardProb;
      if(backwardProb==0)//debug
      {
         printf("Error\n");
         exit(0);
      }//debug

      /*update the hole probability*/
      node->noMapProb = node->noMapProb * (1 - node->prob[i]);
      if(debugFlag)//debug
      {
         printf("%u %f\n",pairs[i].value, pairs[i].prob);
      }
   }
   free(pairs);
      if(debugFlag)//debug
      {
         printf("\n\n");
      }
   free(node->prob);
   free(node->values);
   node->prob = (double *)malloc(sizeof(double)*numberUniqueValues);
   node->values = (unsigned char *)malloc(sizeof(unsigned char)*numberUniqueValues);
   if((node->prob==NULL) || (node->values==NULL))
   {
      printf("Error %s allocation error\n",__FUNCTION__);
      exit(0);
   }
   for(i=0; i<numberUniqueValues; i++)
   {
      node->values[i] = uniqueValues[i];
      node->prob[i] = tempProb[node->values[i]];
      if(debugFlag)//debug
      {
         printf("%u %f\n",node->values[i], node->prob[i]);
      }
   }
   node->numberOutcomes = numberUniqueValues;
      if(debugFlag)//debug
      {
         printf("hole %f\n",node->noMapProb);
      }
#if 0
   /*debug*/
   double total=0;
   for(i=0; i<node->numberOutcomes; i++)
   {
      total = total + node->prob[i];
   }
   total = total + node->noMapProb;
   if((total>=1.001) || (total<=0.999))
   {
      printf("Error %s prob distribution invalid %f\n", __FUNCTION__, total);
      exit(0);
   }
#endif
}
#endif

#define WANT_DEPTH_FILTERING 0
#define WANT_DEPTH_DILATION 0
#define WANT_ZERO_FILL_INPAINTING 1
bool CViewInterpolationGeneral::depthsynthesis_3Dwarp_ipel(BYTE **pDepthMap)
{
  int h, w, u, v;
  int sigma_d = 20;
  int sigma_c = 50;

  CvMat* m = cvCreateMat(3, 1, CV_64F);
  CvMat* mv = cvCreateMat(3, 1, CV_64F);

  cvZero(m_imgVirtualDepth);
  cvZero(m_imgSuccessSynthesis);

  for(h=0; h<m_uiHeight; h++)
  {
    for(w=0; w<m_uiWidth; w++)
    {
      cvmSet(m, 0, 0, w);
      cvmSet(m, 1, 0, h);
      cvmSet(m, 2, 0, 1);
      cvmMul(m_matH_R2V[pDepthMap[h][w]], m, mv);
      u = mv->data.db[0] / mv->data.db[2] + 0.5;
      v = mv->data.db[1] / mv->data.db[2] + 0.5;
      if(u>=0 && u<m_uiWidth && v>=0 && v<m_uiHeight && m_pVirtualDepth[v][u] <= pDepthMap[h][w])
      {
        m_pVirtualDepth[v][u] = pDepthMap[h][w];
        m_pSuccessSynthesis[v][u] = 255;
      }
#if ENABLE_VSDE
      {
         int i;
         double centerProb;

         centerProb = m_distributionClone[m_numberBinsClone/2];
         cvmSet(m, 0, 0, w);
         cvmSet(m, 1, 0, h);
         cvmSet(m, 2, 0, 1);
         for(i=0; i<m_numberBinsClone; i++)
         {
            /*deal with center value at the end*/
            if(i!=(m_numberBinsClone/2))
            {
               int newDepthVal = i - (m_numberBinsClone/2) + (int)pDepthMap[h][w];

               if((newDepthVal<0) || (newDepthVal>255))
               {
                  /*if the new depth value is out of range, no error is introduced, add the probability to the center bin*/
                  centerProb = centerProb + m_distributionClone[i];
               }
               else
               {
                  cvmMul(m_matH_R2V[newDepthVal], m, mv);
                  u = mv->data.db[0] / mv->data.db[2] + 0.5;
                  v = mv->data.db[1] / mv->data.db[2] + 0.5;
                  if(u>=0 && u<m_uiWidth && v>=0 && v<m_uiHeight )
                  {
                     addToProbabilityMap(&m_depthProbMap.outcomes[v][u], (BYTE)newDepthVal, m_distributionClone[i]);
                  }
               }
            }
         }
         /*take care of the center value*/
         cvmMul(m_matH_R2V[pDepthMap[h][w]], m, mv);
         u = mv->data.db[0] / mv->data.db[2] + 0.5;
         v = mv->data.db[1] / mv->data.db[2] + 0.5;
         if(u>=0 && u<m_uiWidth && v>=0 && v<m_uiHeight )
         {
            addToProbabilityMap(&m_depthProbMap.outcomes[v][u], pDepthMap[h][w], centerProb);
         }
      }
#endif
    }
  }

#if ENABLE_VSDE
  for(h=0; h<m_uiHeight; h++)
  {
    for(w=0; w<m_uiWidth; w++)
    {
       processProbabilityMap(&m_depthProbMap.outcomes[h][w], (h==0)&&(w==0));
    }
  }
  /*debug*/
  printf("\n\n");
  for(h=0; h<m_uiHeight; h++)
  {
    for(w=0; w<m_uiWidth; w++)
    {
       double depthAverage=0;
       int k;
       for(k=0; k<m_depthProbMap.outcomes[h][w].numberOutcomes; k++)
       {
          depthAverage = depthAverage + ((double)m_depthProbMap.outcomes[h][w].values[k])*m_depthProbMap.outcomes[h][w].prob[k];
       }
       printf("%f ",depthAverage);
    }
    printf("\n");
  }
  printf("\n\n");
#endif

#if WANT_DEPTH_FILTERING
  cvexMedian(m_imgVirtualDepth);
  cvexBilateral(m_imgVirtualDepth, sigma_d, sigma_c);
  cvexMedian(m_imgSuccessSynthesis);
#endif
  cvNot(m_imgSuccessSynthesis, m_imgHoles); // pixels which couldn't be synthesized
  cvReleaseMat(&m);
  cvReleaseMat(&mv);

  return true;
}

bool CViewInterpolationGeneral::viewsynthesis_reverse_3Dwarp_ipel(BYTE ***src, BYTE **pDepthMap, int th_same_depth)
{
  int ptv, u, v;
  int h, w;

  CvMat* m = cvCreateMat(3, 1, CV_64F);
  CvMat* mv = cvCreateMat(3, 1, CV_64F);

  cvZero(m_imgVirtualImage);

  for(h=0; h<m_uiHeight; h++)
  {
    for(w=0; w<m_uiWidth; w++)
    {
      cvmSet(mv, 0, 0, w);
      cvmSet(mv, 1, 0, h);
      cvmSet(mv, 2, 0, 1);

      cvmMul(m_matH_V2R[m_pVirtualDepth[h][w]], mv, m);
      u = m->data.db[0] / m->data.db[2] + 0.5;
      v = m->data.db[1] / m->data.db[2] + 0.5;
      if(u>=0 && u<m_uiWidth && v>=0 && v<m_uiHeight)
      {
        ptv = w + h * m_uiWidth;
        m_imgVirtualImage->imageData[ptv * 3     ] = src[0][v][u];
        m_imgVirtualImage->imageData[ptv * 3 + 1 ] = src[1][v][u];
        m_imgVirtualImage->imageData[ptv * 3 + 2 ] = src[2][v][u];
      }
#if ENABLE_VSDE
      {
         int i;
         for(i=0; i<m_depthProbMap.outcomes[h][w].numberOutcomes; i++)
         {
            cvmSet(mv, 0, 0, w);
            cvmSet(mv, 1, 0, h);
            cvmSet(mv, 2, 0, 1);
            cvmMul(m_matH_V2R[m_depthProbMap.outcomes[h][w].values[i]], mv, m);
            u = m->data.db[0] / m->data.db[2] + 0.5;
            v = m->data.db[1] / m->data.db[2] + 0.5;
            if(u>=0 && u<m_uiWidth && v>=0 && v<m_uiHeight)
            {
               addToProbabilityMap(&m_textureProbMap.outcomes[h][w], src[0][v][u], m_depthProbMap.outcomes[h][w].prob[i]);
            }
            else
            {
               addToProbabilityMap(&m_textureProbMap.outcomes[h][w], 0, m_depthProbMap.outcomes[h][w].prob[i]);
            }
            m_textureProbMap.outcomes[h][w].noMapProb = m_depthProbMap.outcomes[h][w].noMapProb;
         }
#if 0
         /*debug*/
         double total=0;
         for(i=0; i<m_textureProbMap.outcomes[h][w].numberOutcomes; i++)
         {
            total = total + m_textureProbMap.outcomes[h][w].prob[i];
         }
         total = total + m_textureProbMap.outcomes[h][w].noMapProb;
         if((total>=1.001) || (total<=0.999))
         {
            printf("Error %s prob distribution invalid %f\n", __FUNCTION__, total);
            exit(0);
         }
#endif
      }
#endif
    }
  }
  cvReleaseMat(&m);
  cvReleaseMat(&mv);

  cvCopy(m_imgHoles, m_imgBound);
#if WANT_DEPTH_DILATION
  erodebound(m_imgBound, m_uiDepthType);
  cvDilate(m_imgBound, m_imgBound);
  cvDilate(m_imgBound, m_imgBound);
#endif
  cvOr(m_imgHoles, m_imgBound, m_imgMask[0]);

  return true;
}


#if ENABLE_VSDE
void CViewInterpolationGeneral::probabilityMapMemoryReset()
{
   int i;
   int j;

   if(m_depthProbMap.outcomes!=NULL)
   {
      for(i=0; i<m_depthProbMap.height; i++)
      {
         for(j=0; j<m_depthProbMap.width; j++)
         {
            if(m_depthProbMap.outcomes[i][j].prob!=NULL)
            {
               free(m_depthProbMap.outcomes[i][j].prob);
            }
            if(m_depthProbMap.outcomes[i][j].values!=NULL)
            {
               free(m_depthProbMap.outcomes[i][j].values);
            }
         }
         memset(m_depthProbMap.outcomes[i], 0, sizeof(PROBABILITY_OUTCOME)*m_depthProbMap.width);
         for(j=0; j<m_depthProbMap.width; j++)
         {
            m_depthProbMap.outcomes[i][j].noMapProb = 1;
         }
      }
   }
   if(m_textureProbMap.outcomes!=NULL)
   {
      for(i=0; i<m_textureProbMap.height; i++)
      {
         for(j=0; j<m_textureProbMap.width; j++)
         {
            if(m_textureProbMap.outcomes[i][j].prob!=NULL)
            {
               free(m_textureProbMap.outcomes[i][j].prob);
            }
            if(m_textureProbMap.outcomes[i][j].values!=NULL)
            {
               free(m_textureProbMap.outcomes[i][j].values);
            }
         }
         memset(m_textureProbMap.outcomes[i], 0, sizeof(PROBABILITY_OUTCOME)*m_textureProbMap.width);
         for(j=0; j<m_textureProbMap.width; j++)
         {
            m_textureProbMap.outcomes[i][j].noMapProb = 1;
         }
      }
   }
   if(m_mergedTextureProbMap.outcomes!=NULL)
   {
      for(i=0; i<m_mergedTextureProbMap.height; i++)
      {
         for(j=0; j<m_mergedTextureProbMap.width; j++)
         {
            if(m_mergedTextureProbMap.outcomes[i][j].prob!=NULL)
            {
               free(m_mergedTextureProbMap.outcomes[i][j].prob);
            }
            if(m_mergedTextureProbMap.outcomes[i][j].values!=NULL)
            {
               free(m_mergedTextureProbMap.outcomes[i][j].values);
            }
         }
         memset(m_mergedTextureProbMap.outcomes[i], 0, sizeof(PROBABILITY_OUTCOME)*m_mergedTextureProbMap.width);
         for(j=0; j<m_mergedTextureProbMap.width; j++)
         {
            m_mergedTextureProbMap.outcomes[i][j].noMapProb = 1;
         }
      }
   }
}

/*for now hole pixels are zero filled*/
void findMoments(PROBABILITY_OUTCOME *left, PROBABILITY_OUTCOME *right, double factorLeft, double factorRight, double *firstMoment, double *secondMoment)
{
   int i;
   int j;
   double factor = factorLeft*factorRight;

   *firstMoment=0;
   *secondMoment=0;
   for(i=0; i<left->numberOutcomes; i++)
   {
      for(j=0; j<right->numberOutcomes; j++)
      {
         double temp;

         temp = factorLeft*((double)left->values[i]) + factorRight*((double)right->values[i]);

         *firstMoment = *firstMoment + temp*left->prob[i]*right->prob[j];
         *secondMoment = *secondMoment + temp*temp*left->prob[i]*right->prob[j];
      }
   }

   for(i=0; i<left->numberOutcomes; i++)
   {
      double temp;

      temp = ((double)left->values[i]);

      *firstMoment = *firstMoment + temp*left->prob[i]*right->noMapProb;
      *secondMoment = *secondMoment + temp*temp*left->prob[i]*right->noMapProb;
   }

   for(j=0; j<right->numberOutcomes; j++)
   {
      double temp;

      temp = ((double)right->values[j]);

      *firstMoment = *firstMoment + temp*right->prob[j]*left->noMapProb;
      *secondMoment = *secondMoment + temp*temp*right->prob[j]*left->noMapProb;
   }
}

/*debug function*/
unsigned int getHoleCount(IplImage* mask)
{
   int i;
   int j;
   unsigned int holeCount=0;
   for(i=0; i<mask->height; i++)
   {
      char *datap = &mask->imageData[i*mask->widthStep];
      for(j=0; j<mask->width; j++)
      {
         if(datap[j]!=0)
         {
            holeCount++;
         }
      }
   }

   return holeCount;
}
#endif

int CViewInterpolationGeneral::DoOneFrameGeneral(BYTE*** RefLeft, BYTE*** RefRight, BYTE** RefDepthLeft, BYTE** RefDepthRight, CIYuv *pSynYuvBuffer)
{
//#ifdef _DEBUG
//  if(m_ucSetup!=3) return false;
//#endif
  BYTE*** pRefLeft = RefLeft;
  BYTE*** pRefRight = RefRight;
  BYTE** pRefDepthLeft = RefDepthLeft;
  BYTE** pRefDepthRight = RefDepthRight;

#if ENABLE_VSDE
  probabilityMapMemoryReset();
  m_pcViewSynthesisLeft->probabilityMapMemoryReset();
  m_pcViewSynthesisRight->probabilityMapMemoryReset();
#endif

  if(!m_pcViewSynthesisLeft ->xSynthesizeView(pRefLeft, pRefDepthLeft))  return false;
  if(!m_pcViewSynthesisRight->xSynthesizeView(pRefRight, pRefDepthRight)) return false;

  // GIST added
  if (m_imgSynLeftforBNR    == NULL) { m_imgSynLeftforBNR    = cvCreateImage(cvSize(Width, Height), 8, 3);  }
  if (m_imgSynRightforBNR   == NULL) { m_imgSynRightforBNR   = cvCreateImage(cvSize(Width, Height), 8, 3);  }
  if (m_imgDepthLeftforBNR  == NULL) { m_imgDepthLeftforBNR  = cvCreateImage(cvSize(Width, Height), 8, 1);  }
  if (m_imgDepthRightforBNR == NULL) { m_imgDepthRightforBNR = cvCreateImage(cvSize(Width, Height), 8, 1);  }
  if (m_imgHoleLeftforBNR   == NULL) { m_imgHoleLeftforBNR   = cvCreateImage(cvSize(Width, Height), 8, 1);  }
  if (m_imgHoleRightforBNR  == NULL) { m_imgHoleRightforBNR  = cvCreateImage(cvSize(Width, Height), 8, 1);  }
  cvCopy(m_pcViewSynthesisLeft ->getVirtualImage(),    m_imgSynLeftforBNR    );
  cvCopy(m_pcViewSynthesisRight->getVirtualImage(),    m_imgSynRightforBNR   );
  cvCopy(m_pcViewSynthesisLeft ->getVirtualDepthMap(), m_imgDepthLeftforBNR  );
  cvCopy(m_pcViewSynthesisRight->getVirtualDepthMap(), m_imgDepthRightforBNR );
  cvCopy(m_pcViewSynthesisLeft ->getHolePixels(),      m_imgHoleLeftforBNR   );
  cvCopy(m_pcViewSynthesisRight->getHolePixels(),      m_imgHoleRightforBNR  );
  // GIST end

  // pixels which will be replaced by pixels synthesized from right view
   cvAnd(m_pcViewSynthesisLeft->getUnstablePixels(), m_pcViewSynthesisRight->getSynthesizedPixels(), m_imgMask[3]);

  if(ViewBlending==1)
  {
    if(m_dWeightLeft>=m_dWeightRight)
    {
      cvCopy(m_pcViewSynthesisRight->getVirtualImage(), m_pcViewSynthesisLeft->getVirtualImage(), m_imgMask[3]);
    }
    else
    {
      cvCopy(m_pcViewSynthesisRight->getVirtualImage(), m_pcViewSynthesisLeft->getVirtualImage(), m_pcViewSynthesisRight->getSynthesizedPixels());
    }
  } else {
    cvCopy(m_pcViewSynthesisRight->getVirtualImage(), m_pcViewSynthesisLeft->getVirtualImage(), m_imgMask[3]);
  }

  // pixels which will be replaced by pixels synthesized from left view
  cvAnd(m_pcViewSynthesisRight->getUnstablePixels(), m_pcViewSynthesisLeft->getSynthesizedPixels(), m_imgMask[4]);

  if(ViewBlending==1)
  {
    if(m_dWeightLeft<=m_dWeightRight)
    {
      cvCopy(m_pcViewSynthesisLeft->getVirtualImage(), m_pcViewSynthesisRight->getVirtualImage(), m_imgMask[4]);
    }
    else
    {
      cvCopy(m_pcViewSynthesisLeft->getVirtualImage(), m_pcViewSynthesisRight->getVirtualImage(), m_pcViewSynthesisLeft->getSynthesizedPixels());
    }
  } else {
    cvCopy(m_pcViewSynthesisLeft->getVirtualImage(), m_pcViewSynthesisRight->getVirtualImage(), m_imgMask[4]);
  }

  // pixels which couldn't be synthesized from both left and right -> inpainting
  cvAnd(m_pcViewSynthesisLeft->getHolePixels(), m_pcViewSynthesisRight->getHolePixels(), m_imgMask[2]);
  cvAddWeighted(m_pcViewSynthesisLeft->getVirtualImage(), m_dWeightLeft, m_pcViewSynthesisRight->getVirtualImage(), m_dWeightRight, 0, m_imgBlended);

  cvSet(m_imgBlended, CV_RGB(0, 128, 128), m_imgMask[2]);
  cvInpaint(m_imgBlended, m_imgMask[2], m_imgInterpolatedView, 5, CV_INPAINT_NS);

#if ENABLE_VSDE
  {
     double totalDistortion=0;
     int i;
     int j;
     for(i=0; i<m_uiHeight; i++)
     {
        for(j=0; j<m_uiWidth; j++)
        {
           double pixelDistortion;
           double firstMoment;
           double secondMoment;
           unsigned char actualData = m_imgBlended->imageData[i*m_imgBlended->widthStep + j*3];

           PROBABILITY_MAP *textureMapLeft = m_pcViewSynthesisLeft->getTextureProbMap();
           PROBABILITY_MAP *textureMapRight = m_pcViewSynthesisRight->getTextureProbMap();

           /*find the moments*/
           findMoments(&textureMapLeft->outcomes[i][j], &textureMapRight->outcomes[i][j], m_dWeightLeft, m_dWeightRight, &firstMoment, &secondMoment);

           /*find pixel distortion*/
           pixelDistortion = ((double)actualData)*((double)actualData);
           pixelDistortion = pixelDistortion + secondMoment;
           pixelDistortion = pixelDistortion - 2*((double)actualData)*firstMoment;
           if(pixelDistortion<0)
           {
              printf("error Pixel distortion %f\n",pixelDistortion);
              exit(0);
           }

           totalDistortion = totalDistortion + pixelDistortion;
        }
     }
     printf(" Dis:%f ", totalDistortion);
  }
#endif

  if(ColorSpace) {
#if WANT_ZERO_FILL_INPAINTING
    pSynYuvBuffer->setDataFromImgYUV(m_imgBlended);
#else
    pSynYuvBuffer->setDataFromImgBGR(m_imgInterpolatedView);
#endif
  }
  else
#if WANT_ZERO_FILL_INPAINTING
    pSynYuvBuffer->setDataFromImgYUV(m_imgBlended);
#else
    pSynYuvBuffer->setDataFromImgYUV(m_imgInterpolatedView);
#endif

//#ifdef _DEBUG
//  m_ucSetup-=2;
//#endif

  return 0;

}
IplImage*  CViewInterpolationGeneral::getImgSynthesizedViewLeft()
{
#ifdef _DEBUG
  if(m_ucSetup!=3) return NULL;
#endif

  return m_pcViewSynthesisLeft->getVirtualImage();
}

IplImage*  CViewInterpolationGeneral::getImgSynthesizedViewRight()
{
#ifdef _DEBUG
  if(m_ucSetup!=3) return NULL;
#endif

  return m_pcViewSynthesisRight->getVirtualImage();
}
