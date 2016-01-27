#ifndef _INCLUDE_VIEW_INTERPOLATION_H_
#define _INCLUDE_VIEW_INTERPOLATION_H_

#include <highgui.h>


class CParameterViewInterpolation;      // Class for view synthesis parameters
class CViewInterpolationGeneral;        // Class for general mode
class CViewInterpolation1D;             // Class for 1-D view synthesis
class CBoundaryNoiseRemoval;            // Class for boundary noise removal

/*
 * Interface class between application and sysnthesis objects
 */

class CViewInterpolation
{
public:
  CViewInterpolation();
  virtual ~CViewInterpolation();

  bool    Init(CParameterViewInterpolation& cParameter);
  bool    SetReferenceImage(int iLeft, CIYuv *pcYuv);// Set up the reference pictures
  bool    DoViewInterpolation(CIYuv* pYuvBuffer);    // The main interface function to be called to perform view interpolation

  CIYuv*    getDepthBufferLeft      ()    { return m_pcDepthMapLeft; }
  CIYuv*    getDepthBufferRight      ()    { return m_pcDepthMapRight; }
  unsigned int  getBoundaryNoiseRemoval () { return  m_uiBoundary;}
  Int       getFrameNumber()               { return m_iFrameNumber;} //Zhejiang
  void      setFrameNumber(int frame_number) { m_iFrameNumber = frame_number;}

private:
  bool    xViewInterpolationGeneralMode(CIYuv* pYuvBuffer);
  bool    xViewInterpolation1DMode( CIYuv* pSynYuvBuffer );
  bool    xBoundaryNoiseRemoval(CIYuv* pRefLeft, CIYuv* pRefRight, CIYuv* pRefDepthLeft, CIYuv* pRefDepthRight, CIYuv* pRefHoleLeft, CIYuv* pRefHoleRight, CIYuv* pSynYuvBuffer, bool SynthesisMode);
  void    xFileConvertingforGeneralMode(CIYuv* pRefLeft, CIYuv* pRefRight, CIYuv* pRefDepthLeft, CIYuv* pRefDepthRight, CIYuv* pRefHoleLeft, CIYuv* pRefHoleRight);
  void    xFileConvertingfor1DMode     (CIYuv* pRefLeft, CIYuv* pRefRight, CIYuv* pRefDepthLeft, CIYuv* pRefDepthRight, CIYuv* pRefHoleLeft, CIYuv* pRefHoleRight);

private:

  Int     m_iSynthesisMode;      //!> 0: General mode, 1: 1-D mode
  Int     m_iFrameNumber;          //used in TIM, Zhejiang

  unsigned int    m_uiBoundary;  //!> 0: No Boundary Noise Removal, 1: Use Boundary Noise Removal
  unsigned int    m_uiColorSpace;
  unsigned int    m_uiViewBlending;

  CIYuv*          m_pcDepthMapLeft;       //!> To store the depth map of the left reference view
  CIYuv*          m_pcDepthMapRight;      //!> To store the depth map of the right reference view
  CIYuv*          m_pcImageLeft;          //!> To store the image of the left reference view ???
  CIYuv*          m_pcImageRight;
  CIYuv*          m_pcTempYuvLeft;
  CIYuv*          m_pcTempYuvRight;

  CViewInterpolationGeneral* m_pViewSynthesisGeneral; //!> The object to do 3D warping view synthesis
  CViewInterpolation1D*      m_pViewSynthesis1D;      //!> The object to do 1D view synthesis
  CBoundaryNoiseRemoval*     m_pBoundaryNoiseRemoval; //!> The object to do boundary noise removal

  Int            m_iWidth;
  Int            m_iHeight;
  unsigned char* m_pSynColorLeft;
  unsigned char* m_pSynColorRight;
  unsigned char* m_pSynDepthLeft;
  unsigned char* m_pSynDepthRight;

#ifdef _DEBUG
  unsigned char    m_ucSetup;
#endif

};

#endif
