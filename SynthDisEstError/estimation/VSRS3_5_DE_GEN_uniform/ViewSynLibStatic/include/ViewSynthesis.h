#ifndef _VIEW_SYNTHESIS_H_
#define _VIEW_SYNTHESIS_H_

#pragma warning(disable:4996)
#pragma warning(disable:4244)
#pragma warning(disable:4819)

#ifndef WIN32
#define BYTE unsigned char
#endif

#include <iostream>
#include <fstream>

#include "yuv.h"
#include "vsde.h" //vsde added

typedef unsigned char uchar;
//using namespace std;

#ifndef UInt
#define UInt    unsigned int
#endif

class CViewInterpolationGeneral
{
public:
  CViewInterpolationGeneral          ();
  virtual ~CViewInterpolationGeneral      ();

  // Member function InitLR was added to class CViewInterpolationGeneral.
#if ENABLE_VSDE
  bool InitLR        (  UInt uiWidth, UInt uiHeight, UInt uiPrecision, UInt uiDepthType,
                                  double dZnearL, double dZfarL, double dZnearR, double dZfarR,
                                  const char *strCamParamFile, const char *strRefLCamID, const char *strRefRCamID, const char *strVirCamID,
                                  double Mat_In_Left[9], double Mat_Ex_Left[9], double Mat_Trans_Left[3],
                                  double Mat_In_Right[9], double Mat_Ex_Right[9], double Mat_Trans_Right[3],
                                  double Mat_In_Virtual[9], double Mat_Ex_Virtual[9], double Mat_Trans_Virtual[3], double stdev, int numberBins);
#else
  bool InitLR        (  UInt uiWidth, UInt uiHeight, UInt uiPrecision, UInt uiDepthType,
                                  double dZnearL, double dZfarL, double dZnearR, double dZfarR,
                                  const char *strCamParamFile, const char *strRefLCamID, const char *strRefRCamID, const char *strVirCamID,
                                  double Mat_In_Left[9], double Mat_Ex_Left[9], double Mat_Trans_Left[3],
                                  double Mat_In_Right[9], double Mat_Ex_Right[9], double Mat_Trans_Right[3],
                                  double Mat_In_Virtual[9], double Mat_Ex_Virtual[9], double Mat_Trans_Virtual[3]);
#endif
  // The number of arguments of Init increased.

  bool Init        (  UInt uiWidth, UInt uiHeight, UInt uiPrecision, UInt uiDepthType,
                                  double dZnear, double dZfar, const char *strCamParamFile, const char *strRefCamID, const char *strVirCamID,
                                  double Mat_In_Ref[9], double Mat_Ex_Ref[9], double Mat_Trans_Ref[3], double Mat_In_Vir[9], double Mat_Ex_Vir[9], double Mat_Trans_Vir[3]);

  void    xReleaseMemory        ();

  bool    xSynthesizeView        (BYTE ***src, BYTE **pDepthMap, int th_same_depth=5);
  bool    xSynthesizeDepth      (BYTE **pDepthMap);
  bool    xSynthesizeView_reverse    (BYTE ***src, BYTE **pDepthMap, int th_same_depth=5);

  double    getBaselineDistance      ()  { return m_dBaselineDistance; }
  double    getLeftBaselineDistance      ()  { return LeftBaselineDistance; }
  double    getRightBaselineDistance      ()  { return RightBaselineDistance; }
  IplImage*  getHolePixels        ()  { return m_imgHoles; }
  IplImage*  getSynthesizedPixels    ()  { return m_imgSuccessSynthesis; }
  IplImage*  getUnstablePixels      ()  { return m_imgMask[0]; }
  IplImage*  getVirtualImage        ()  { return m_imgVirtualImage; }
  IplImage*  getVirtualDepthMap      ()  { return m_imgVirtualDepth; }
  //Nagoya start
  BYTE**    getVirtualImageY       ()  { return m_pVirtualImageY; }
  BYTE**    getVirtualImageU       ()  { return m_pVirtualImageU; }
  BYTE**    getVirtualImageV       ()  { return m_pVirtualImageV; }
  //Nagoya end
  BYTE**    getVirtualDepth        ()  { return m_pVirtualDepth; }
  BYTE**    getNonHoles          ()  { return m_pSuccessSynthesis; }
  void SetWidth (int sWidth ) { Width  = sWidth;  }
  void SetHeight(int sHeight) { Height = sHeight; }
  void SetDepthType(int sDepthType) {DepthType = sDepthType; }
  void SetColorSpace(int sColorSpace) { ColorSpace = sColorSpace; }
  void SetSubPelOption(int sPrecision) { Precision = sPrecision; }
  void SetViewBlending(int sViewBlending) { ViewBlending = sViewBlending; }
  void SetBoundaryNoiseRemoval(int sBoundaryNoiseRemoval) { m_uiBoundaryNoiseRemoval = sBoundaryNoiseRemoval; }
#if ENABLE_VSDE
  void probabilityMapMomentMemoryReset();
  void setProbabilityDistribution(double *distribution, double stdev, int numberBins) {m_distributionClone=distribution; m_stdevClone=stdev; m_numberBinsClone=numberBins;}
  PROBABILITY_MAP *getTextureProbMap() {return &m_textureProbMap;}
#if WANT_EXTERNAL_DISTRIBUTION
  void openDistributionFile(char *fileName) {externalDistribution = fopen(fileName,"rb");}
  void closeDistributionFile() {fclose(externalDistribution);}
#endif
#endif

  int  DoOneFrameGeneral(BYTE*** RefLeft, BYTE*** RefRight, BYTE** RefDepthLeft, BYTE** RefDepthRight, CIYuv* pSynYuvBuffer);
  IplImage*  getImgSynthesizedViewLeft  ();
  IplImage*  getImgSynthesizedViewRight  ();

  // GIST added
  CvMat** GetMatH_V2R() { return m_matH_V2R; }
  CViewInterpolationGeneral* GetInterpolatedLeft ()   { return m_pcViewSynthesisLeft;}
  CViewInterpolationGeneral* GetInterpolatedRight()   { return m_pcViewSynthesisRight;}
  IplImage*  GetSynLeftWithHole      ()    { return m_imgSynLeftforBNR;   }
  IplImage*  GetSynRightWithHole     ()    { return m_imgSynRightforBNR;  }
  IplImage*  GetSynDepthLeftWithHole ()    { return m_imgDepthLeftforBNR; }
  IplImage*  GetSynDepthRightWithHole()    { return m_imgDepthRightforBNR;}
  IplImage*  GetSynHoleLeft          ()    { return m_imgHoleLeftforBNR;  }
  IplImage*  GetSynHoleRight         ()    { return m_imgHoleRightforBNR; }
  int        GetPrecision            ()    { return m_uiPrecision;        }
  void       SetLeftBaselineDistance( double sLeftBaselineDistance)    { LeftBaselineDistance  = sLeftBaselineDistance;  }
  void       SetRightBaselineDistance( double sRightBaselineDistance)  { RightBaselineDistance = sRightBaselineDistance; }
  // GIST end


  //Nagoya start
  unsigned char* GetSynColorLeftY () { return *(m_pcViewSynthesisLeft->getVirtualImageY()); }
  unsigned char* GetSynColorRightY() { return *(m_pcViewSynthesisRight->getVirtualImageY()); }
  unsigned char* GetSynColorLeftU () { return *(m_pcViewSynthesisLeft->getVirtualImageU()); }
  unsigned char* GetSynColorRightU() { return *(m_pcViewSynthesisRight->getVirtualImageU()); }
  unsigned char* GetSynColorLeftV () { return *(m_pcViewSynthesisLeft->getVirtualImageV()); }
  unsigned char* GetSynColorRightV() { return *(m_pcViewSynthesisRight->getVirtualImageV()); }
  unsigned char* GetSynDepthLeft  () { return *(m_pcViewSynthesisLeft->getVirtualDepth()); }
  unsigned char* GetSynDepthRight () { return *(m_pcViewSynthesisRight->getVirtualDepth()); }
  //Nagoya end

protected:

private:
  void    convertCameraParam      (CvMat *exMat_dst, CvMat *exMat_src);

  void    cvexMedian          (IplImage* dst);
  void    cvexBilateral        (IplImage* dst, int sigma_d, int sigma_c);
  void    erodebound          (IplImage* bound, int flag);
  int      median_filter_depth      (IplImage *srcDepth, IplImage *dstDepth, IplImage *srcMask,  IplImage *dstMask, int sizeX, int sizeY, bool bSmoothing);
  int      median_filter_depth_wCheck  (IplImage *srcDepth, IplImage *dstDepth, IplImage *srcMask,  IplImage *dstMask, int sizeX, int sizeY, bool bSmoothing, int th_same_plane=5);

  CViewInterpolationGeneral*  m_pcViewSynthesisLeft;
  CViewInterpolationGeneral*  m_pcViewSynthesisRight;

  bool    init_camera_param(double Mat_In_Ref[9], double Mat_Ex_Ref[9], double Mat_Trans_Ref[3], double Mat_In_Vir[9], double Mat_Ex_Vir[9], double Mat_Trans_Vir[3],
                                       CvMat *mat_in[2], CvMat *mat_ex_c2w[2], CvMat *mat_proj[2]);
  bool    init_3Dwarp(double Z_near, double Z_far, unsigned int uiDepthType,
                                  const char *strCamParamFile, const char *strRefCamID, const char *strVirCamID,
                                  double Mat_In_Ref[9], double Mat_Ex_Ref[9], double Mat_Trans_Ref[3],
                                  double Mat_In_Vir[9], double Mat_Ex_Vir[9], double Mat_Trans_Vir[3]);
  bool    init_shift          (double Z_near, double Z_far, unsigned int uiDepthType, const char *strCamParamFile, const char *strRefCamID, const char *strVirCamID);

  void    image2world_with_z      (CvMat *mat_Rc2w_invIN_from, CvMat *matEX_c2w_from, CvMat *image, CvMat *world);
  void    makeHomography        (CvMat *matH_F2T[256], CvMat *matH_T2F[256], double adTable[256], CvMat *matIN_from, CvMat *matEX_c2w_from, CvMat *matProj_to);

  bool    depthsynthesis_3Dwarp    (BYTE **pDepthMap);
  bool    depthsynthesis_3Dwarp_ipel  (BYTE **pDepthMap);

  bool    viewsynthesis_reverse_3Dwarp    (BYTE ***src, BYTE **pDepthMap, int th_same_depth);
  bool    viewsynthesis_reverse_3Dwarp_ipel  (BYTE ***src, BYTE **pDepthMap, int th_same_depth);

  double        m_dWeightLeft;
  double        m_dWeightRight;
  double        WeightLeft;
  double        WeightRight;

  unsigned int  m_uiViewBlending;
  unsigned int  ViewBlending;

  unsigned int  m_uiWidth;
  unsigned int  m_uiHeight;
  unsigned int  m_uiPicsize;
  unsigned int  m_uiPrecision;
  unsigned int  m_uiDepthType;
  unsigned char  m_ucLeftSide;

  unsigned int  m_uiColorSpace;
  unsigned int  m_uiBoundaryNoiseRemoval;

  std::string   CameraParameterFile;
  std::string   LeftCameraName;
  std::string   RightCameraName;
  std::string   VirtualCameraName;

  unsigned int  Width;
  unsigned int  Height;
  unsigned int  Picsize;
  unsigned int  Precision;
  unsigned int  DepthType;

  unsigned int  ColorSpace;

  double  Mat_Ex_Left[9];
  double  Mat_Ex_Virtual[9];
  double  Mat_Ex_Right[9];
  double  Mat_In_Left[9];
  double  Mat_In_Virtual[9];
  double  Mat_In_Right[9];
  double  Mat_Trans_Left[3];
  double  Mat_Trans_Virtual[3];
  double  Mat_Trans_Right[3];

  IplImage*  m_imgBlended;          //!> Blended image
  IplImage*  m_imgInterpolatedView; //!> The final image buffer to be output
  IplImage*  getImgInterpolatedView    ()    { return m_imgInterpolatedView; }

  double  m_dBaselineDistance;
  double  LeftBaselineDistance;
  double  RightBaselineDistance;

  BYTE**  m_pVirtualDepth;
  BYTE**  m_pSuccessSynthesis;

  //Nagoya start
  BYTE** m_pVirtualImageY;
  BYTE** m_pVirtualImageU;
  BYTE** m_pVirtualImageV;
  //Nagoya end

  IplImage*      m_imgVirtualImage;
  IplImage*      m_imgVirtualDepth;
  IplImage*      m_imgSuccessSynthesis;
  IplImage*      m_imgHoles;
  IplImage*      m_imgBound;
  IplImage*      m_imgMask[5];
  IplImage*      m_imgTemp[5];
  IplConvKernel*    m_pConvKernel;

  CvMat*  m_matH_R2V[256];
  CvMat*  m_matH_V2R[256];

  int*  m_aiTableDisparity_ipel;
  int*  m_aiTableDisparity_subpel;

  bool (CViewInterpolationGeneral::*m_pFunc_ViewSynthesisReverse) (BYTE ***src, BYTE **pDepthMap, int th_same_depth) ;
  bool (CViewInterpolationGeneral::*m_pFunc_DepthSynthesis) (BYTE **pDepthMap) ;

  // GIST added
  IplImage*      m_imgSynLeftforBNR;
  IplImage*      m_imgSynRightforBNR;
  IplImage*      m_imgDepthLeftforBNR;
  IplImage*      m_imgDepthRightforBNR;
  IplImage*      m_imgHoleLeftforBNR;
  IplImage*      m_imgHoleRightforBNR;
  // GIST end
#if ENABLE_VSDE
  /*Who came up with this implementation?!*/
  /*m_pcViewSynthesisLeft and m_pcViewSynthesisRight are declared in the same class as the containing parent class*/
  /*Not logical at all!!! Data ownership is confusing and incorrect*/
  PROBABILITY_MAP m_depthProbMap;           /*used by m_pcViewSynthesisLeft and m_pcViewSynthesisRight. Used by m_pcViewSynthesisLeft and m_pcViewSynthesisRight, allocated by Init, freed by destructor*/
  PROBABILITY_MAP m_textureProbMap;         /*used by m_pcViewSynthesisLeft and m_pcViewSynthesisRight. Used by m_pcViewSynthesisLeft and m_pcViewSynthesisRight, allocated by Init, freed by destructor*/
  PIXEL_MOMENTS m_mergedPixelMoments;   /*used by this class, allocated by InitLR, freed by destructor*/
  double *m_distribution;                     /*depth error prob distribution*/
  double m_stdev;                            /*depth error prob standard deviation*/
  int m_numberBins;                           /*depth error prob distribution number of bins*/
  /*forced to do this kind of crap due to this bad data ownship problem*/
  double *m_distributionClone;                     /*depth error prob distribution*/
  double m_stdevClone;                            /*depth error prob standard deviation*/
  int m_numberBinsClone;                           /*depth error prob distribution number of bins*/
#if WANT_EXTERNAL_DISTRIBUTION
  FILE *externalDistribution;
#endif
#endif
#ifdef _DEBUG
  unsigned char    m_ucSetup;
#endif

};

#endif
