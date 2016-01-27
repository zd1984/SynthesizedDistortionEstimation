/*
 * This software module ViSBD (View Synthesis Based on Disparity /Depth was originally developed by
 * THOMSON INC. in the course of development of the ISO/IEC JTC1/SC29 WG 11 (MPEG) 3D Video for reference
 * purposes and its performance may not have been optimized.
 *
 * Those intending to use this software module in products are advised that its use may infringe
 * existing patents. ISO/IEC have no liability for use of this software module or modifications thereof.
 *
 * Assurance that the originally developed software module can be used
 *   (1) in the ISO/IEC JTC1/SC29 WG 11 (MPEG) 3D Video once the it is adopted to be used as reference
 *       software; and
 *   (2) to develop the codec for ISO/IEC JTC1/SC29 WG 11 (MPEG) 3D Video.
 *
 * To the extent that THOMSON INC. OR ANY OF ITS AFFILIATES owns patent rights that would be required to
 * make, use, or sell the originally developed software module or portions thereof included in the ISO/IEC
 * JTC1/SC29 WG 11 (MPEG) 3D Video in a conforming product, THOMSON INC. will assure the ISO/IEC that it
 * is willing to negotiate licenses under reasonable and non-discriminatory terms and conditions with
 * applicants throughout the world.
 *
 * THOMSON INC. retains full right to modify and use the code for its own purpose, assign or donate the
 * code to a third party and to inhibit third parties from using the code for products that do not conform
 * to MPEG-related and/or ISO/IEC International Standards.
 *
 * This copyright notice must be included in all copies or derivative works.
 * Copyright (c) ISO/IEC 2008.
 *
 * Authors:
 *      Dong Tian,  dong.tian@thomson.net
 *      Zefeng Ni,  zefeng.ni@thomson.net
 */

/*
 * In addition to the orignal authors, this software was further modified by the following parties:
 *
 * Authors:
 *      Lu  Yu,    yul@zju.edu.cn (Zhejiang University)
 *      Yin Zhao,  zhao87099664@163.com  (Zhejiang University)
 *      <new contributor name>, <email>, <affiliation>
 *
 * The related parities retain full right to their code for their own purpose, assign or donate the cooresponding
 * code to another party and to inhibit third parties from using the code for products that do not conform
 * to MPEG-related and/or ISO/IEC International Standards.
 *
 */

#ifdef WIN32
#pragma warning(disable : 4996)
#endif


#include "Visbd.h"

CViewInterpolation1D::CViewInterpolation1D()
{
  int i;

  Width = 1024;
  Height = 768;
  SubPelOption = 2;
  UpsampleRefs = 2;
  MergingOption = 2;
  SplattingOption = 2;

  FocalLength = 1800;
  LTranslation[LEFTVIEW] = 50;
  LTranslation[RGHTVIEW] = -50;

  duPrincipal[LEFTVIEW] = duPrincipal[RGHTVIEW] = 0;
  Znear[LEFTVIEW] = 40;
  Zfar[LEFTVIEW] = 120;
  Znear[RGHTVIEW] = 40;
  Zfar[RGHTVIEW] = 120;
  DepthThreshold = 75;
  HoleCountThreshold = 30;
  BoundaryGrowth = 40;
  TemporalImprovementOption = 1; // Zhejiang, May, 4
  WarpEnhancementOption = 1;
  CleanNoiseOption = 1;
  SplattingDepthThreshold = 255;

  for (i = 0; i < 2; i++)
  {
    BoundaryMask[i] = NULL;
    RefView[i] = NULL;
    RefDepth[i] = NULL;
    RefViewLast[i] = NULL; //Zhejiang
    RefDepthLast[i] = NULL;
  }
  for (i = 0; i < 3; i++)
  {
    Mask[i] = NULL;
    Dmap[i] = NULL;
#if USE_LDV
    OMask [i] = NULL;
    ODmap[i] = NULL;
#endif
#if USE_ZMAP
    Zmap[i] = NULL;
    #if USE_LDV
    OccZmap[i] = NULL;
    #endif
#endif
  }
  for (i = 0; i < 5; i++)
  {
    SynY[i] = NULL;
    SynU[i] = NULL;
    SynV[i] = NULL;
#if USE_LDV
    OccY[i] = NULL;
    OccU[i] = NULL;
    OccV[i] = NULL;
#endif
  }
#if ENABLE_VSDE
  m_textureProbMap[0].outcomes=NULL;
  m_textureProbMap[1].outcomes=NULL;
  m_mergedPixelMoments.firstMoment=NULL;
  m_mergedPixelMoments.secondMoment=NULL;
#endif
}

CViewInterpolation1D::~CViewInterpolation1D()
{
  int i;
#if ENABLE_VSDE
  int j;
  int k;
#endif
  for (i = 0; i < 2; i++)
  {
    if (BoundaryMask[i]) free(BoundaryMask[i]);
    if (UpsampleRefs != 1)
    {
      if (RefView[i]) free(RefView[i]);
      if (RefDepth[i]) free(RefDepth[i]);
      if (RefViewLast[i]) free(RefViewLast[i]); //Zhejiang
       if (RefDepthLast[i]) free(RefDepthLast[i]);
    }
  }
  for (i = 0; i < 3; i++)
  {
    if (Mask[i]) free(Mask [i]);
    if (Dmap[i]) free(Dmap[i]);
#if USE_LDV
    if (OMask[i]) free(OMask [i]);
    if (ODmap[i]) free(ODmap[i]);
#endif
#if USE_ZMAP
    if (Zmap[i]) free(Zmap[i]);
  #if USE_LDV
    if (OccZmap[i]) free(OccZmap[i]);
  #endif
#endif
  }
  for (i = 0; i < 5; i++)
  {
    if (SynY[i]) free(SynY[i]);
    if (SynU[i]) free(SynU[i]);
    if (SynV[i]) free(SynV[i]);
#if USE_LDV
    if (OccY[i]) free(OccY[i]);
    if (OccU[i]) free(OccU[i]);
    if (OccV[i]) free(OccV[i]);
#endif
  }

#if ENABLE_VSDE
  /*free the 2D access strucutre*/
  if(texturePMF[0]!=NULL)
  {
     free(texturePMF[0]);
     texturePMF[0] = NULL;
  }
  if(texturePMF[1]!=NULL)
  {
     free(texturePMF[1]);
     texturePMF[1] = NULL;
  }
  if(depthPMF[0]!=NULL)
  {
     free(depthPMF[0]);
     depthPMF[0] = NULL;
  }
  if(depthPMF[1]!=NULL)
  {
     free(depthPMF[1]);
     depthPMF[1] = NULL;
  }
  /*release the shared memory handles*/
  if(texturePMF1D[0]!=NULL)
  {
     UnmapViewOfFile(texturePMF1D[0]);
     texturePMF1D[0] = NULL;
  }
  if(texturePMF1D[1]!=NULL)
  {
     UnmapViewOfFile(texturePMF1D[1]);
     texturePMF1D[1] = NULL;
  }
  if(depthPMF1D[0]!=NULL)
  {
     UnmapViewOfFile(depthPMF1D[0]);
     depthPMF1D[0] = NULL;
  }
  if(depthPMF1D[1]!=NULL)
  {
     UnmapViewOfFile(depthPMF1D[1]);
     depthPMF1D[1] = NULL;
  }

  /*release depth probability map*/
  for(k=0; k<2; k++)
  {
     if(m_textureProbMap[k].outcomes!=NULL)
     {
        for(i=0; i<m_textureProbMap[k].height; i++)
        {
          for(j=0; j<m_textureProbMap[k].width; j++)
          {
            if(m_textureProbMap[k].outcomes[i][j].prob!=NULL)
            {
               free(m_textureProbMap[k].outcomes[i][j].prob);
            }
            if(m_textureProbMap[k].outcomes[i][j].Dvalues!=NULL)
            {
               free(m_textureProbMap[k].outcomes[i][j].Dvalues);
            }
            if(m_textureProbMap[k].outcomes[i][j].x!=NULL)
            {
               free(m_textureProbMap[k].outcomes[i][j].x);
            }
            if(m_textureProbMap[k].outcomes[i][j].y!=NULL)
            {
               free(m_textureProbMap[k].outcomes[i][j].y);
            }
          }
          free(m_textureProbMap[k].outcomes[i]);
       }

       free(m_textureProbMap[k].outcomes);
       m_textureProbMap[k].outcomes = NULL;
     }
  }

  /*release pixel moment buffer*/
  if(m_mergedPixelMoments.firstMoment!=NULL)
  {
     free(m_mergedPixelMoments.firstMoment);
     m_mergedPixelMoments.firstMoment = NULL;
  }
  if(m_mergedPixelMoments.secondMoment!=NULL)
  {
     free(m_mergedPixelMoments.secondMoment);
     m_mergedPixelMoments.secondMoment = NULL;
  }
#endif
}

void CViewInterpolation1D::SetSubPelOption(int sSubPelOption)
{
  if (sSubPelOption == 1)
  {
    SubPelOption = 1;
    UpsampleRefs = 1;
  }
  else if (sSubPelOption == 2)
  {
    SubPelOption = 2;
    UpsampleRefs = 1;
  }
  else if (sSubPelOption == 4)
  {
    SubPelOption = 2;
    UpsampleRefs = 2;
  }
  else
  {
    SubPelOption = 1;
    UpsampleRefs = 1;
    printf("Warning: the precision is not supported yet: %d\n", sSubPelOption);
  }
}

int  CViewInterpolation1D::AllocMem()
{
  int i;
  size_t sz;
#if ENABLE_VSDE
  int h,w;
  int k;
#endif

  Width2 = Width*UpsampleRefs;

  //BoundaryMask[*] is now the size of Width2*Height (previously, Width*Height)
  sz = Width2*Height;
  // When splattingOption==2 or CleanNoiseOption==1
  if (SplattingOption == 2 || CleanNoiseOption == 1)
  {
    for (i = 0; i < 2; i++)
      if (BoundaryMask[i] == NULL)
      {
        BoundaryMask[i] = (unsigned char*) malloc(sz*sizeof(unsigned char));
        if (BoundaryMask[i] == NULL) return -1;
      }
  }

  sz = Width2*Height*SubPelOption;
  //for (i = 0; i < 2; i++)
  //{
  //  if (RefU[i] == NULL)
  //  {
  //    RefU[i]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
  //    if (RefU[i] == NULL) return -1;
  //  }
  //  if (RefV[i] == NULL)
  //  {
  //    RefV[i]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
  //    if (RefV[i] == NULL) return -1;
  //  }
  //}

  for (i = 0; i < 3; i++)
  {
    if (Mask[i] == NULL)
    {
      Mask[i]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
      if (Mask[i] == NULL) return -1;
    }
    if (Dmap[i] == NULL)
    {
      Dmap[i] = (unsigned char*) malloc(sz*sizeof(unsigned char));
      if (Dmap[i] == NULL) return -1;
    }
#if USE_LDV
    if (OMask[i] == NULL)
    {
      OMask[i]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
      if (OMask[i] == NULL) return -1;
    }
    if (ODmap[i] == NULL)
    {
      ODmap[i] = (unsigned char*) malloc(sz*sizeof(unsigned char));
      if (Dmap[i] == NULL) return -1;
    }
#endif
#if USE_ZMAP
    if (Zmap[i] == NULL)
    {
      Zmap[i] = (float*) malloc(sz*sizeof(float));
      if (Zmap[i] == NULL) return -1;
    }
  #if USE_LDV
    if (OccZmap[i] == NULL)
    {
      OccZmap[i] = (float*) malloc(sz*sizeof(float));
      if (OccZmap[i] == NULL) return -1;
    }
  #endif
#endif
  }

  for (i = 0; i < 4; i++)
  {
    if (SynY[i] == NULL)
    {
      SynY[i]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
      if (SynY[i] == NULL) return -1;
    }
    if (SynU[i] == NULL)
    {
      SynU[i]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
      if (SynU[i] == NULL) return -1;
    }
    if (SynV[i] == NULL)
    {
      SynV[i]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
      if (SynV[i] == NULL) return -1;
    }
#if USE_LDV
    if (OccY[i] == NULL)
    {
      OccY[i]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
      if (OccY[i] == NULL) return -1;
    }
    if (OccU[i] == NULL)
    {
      OccU[i]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
      if (OccU[i] == NULL) return -1;
    }
    if (OccV[i] == NULL)
    {
      OccV[i]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
      if (OccV[i] == NULL) return -1;
    }
#endif
}

  // Scaled down version
  sz = Width2*Height;
  if (SynY[FINLVIEW] == NULL)
  {
    SynY[FINLVIEW]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
    if (SynY[FINLVIEW] == NULL) return -1;
  }
  if (SynU[FINLVIEW] == NULL)
  {
    SynU[FINLVIEW]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
    if (SynU[FINLVIEW] == NULL) return -1;
  }
  if (SynV[FINLVIEW] == NULL)
  {
    SynV[FINLVIEW]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
    if (SynV[FINLVIEW] == NULL) return -1;
  }
#if USE_LDV
if (OccY[FINLVIEW] == NULL)
  {
    OccY[FINLVIEW]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
    if (OccY[FINLVIEW] == NULL) return -1;
  }
  if (OccU[FINLVIEW] == NULL)
  {
    OccU[FINLVIEW]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
    if (OccU[FINLVIEW] == NULL) return -1;
  }
  if (OccV[FINLVIEW] == NULL)
  {
    OccV[FINLVIEW]  = (unsigned char*) malloc(sz*sizeof(unsigned char));
    if (OccV[FINLVIEW] == NULL) return -1;
  }
#endif
  if (UpsampleRefs != 1)
  {
    for (i = 0; i < 2; i++)
    {
      if (RefView[i] == NULL)
      {
        RefView[i] = (unsigned char*) malloc(sz*3*sizeof(unsigned char));
        if (RefView[i] == NULL) return -1;
      }
      if (RefDepth[i] == NULL)
      {
        RefDepth[i] = (unsigned char*) malloc(sz*sizeof(unsigned char));  // ???????????
        if (RefDepth[i] == NULL) return -1;
      }
    }
  }

  //Alloc Memory for TIM and cleaning boundary noise, Zhejiang
  sz = Width2 * Height;
  for(i=0; i<2; i++)
  {
     if (RefViewLast[i] == NULL)
     {
        RefViewLast[i] = (unsigned char*) malloc(sz*sizeof(unsigned char));
        if (RefViewLast[i] == NULL) return -1;
     }
     if (RefDepthLast[i] == NULL)
     {
        RefDepthLast[i] = (unsigned char*) malloc(sz*sizeof(unsigned char));  // ???????????
        if (RefDepthLast[i] == NULL) return -1;
     }
  }

  WeightLeft = fabs(LTranslation[RGHTVIEW]/(fabs(LTranslation[LEFTVIEW])+fabs(LTranslation[RGHTVIEW])));
  WeightRight = 1-WeightLeft ;

#if ENABLE_VSDE
  /*create the shared memory handles and allocate the 2D access structure so that we can easily index into the PMF structure*/
  texturePMF1D[0] = (PixelPMF *) MapViewOfFile(textureMemLeft, // handle to map object
              FILE_MAP_ALL_ACCESS,  // read/write permission
              0,
              0,
              sizeof(PixelPMF)*(Height+2*IMG_PAD_SIZE_Y)*(Width2+2*IMG_PAD_SIZE_X));
  if (texturePMF1D[0] == NULL)
  {
     printf("Could not map view of file (%d).\n", GetLastError());
     exit(0);
  }

  texturePMF1D[1] = (PixelPMF *) MapViewOfFile(textureMemRight, // handle to map object
              FILE_MAP_ALL_ACCESS,  // read/write permission
              0,
              0,
              sizeof(PixelPMF)*(Height+2*IMG_PAD_SIZE_Y)*(Width2+2*IMG_PAD_SIZE_X));
  if (texturePMF1D[1] == NULL)
  {
     printf("Could not map view of file (%d).\n", GetLastError());
     exit(0);
  }

  depthPMF1D[0] = (PixelPMF *) MapViewOfFile(depthMemLeft, // handle to map object
              FILE_MAP_ALL_ACCESS,  // read/write permission
              0,
              0,
              sizeof(PixelPMF)*(Height+2*IMG_PAD_SIZE_Y)*(Width2+2*IMG_PAD_SIZE_X));
  if (depthPMF1D[0] == NULL)
  {
     printf("Could not map view of file (%d).\n", GetLastError());
     exit(0);
  }

  depthPMF1D[1] = (PixelPMF *) MapViewOfFile(depthMemRight, // handle to map object
              FILE_MAP_ALL_ACCESS,  // read/write permission
              0,
              0,
              sizeof(PixelPMF)*(Height+2*IMG_PAD_SIZE_Y)*(Width2+2*IMG_PAD_SIZE_X));
  if (depthPMF1D[1] == NULL)
  {
     printf("Could not map view of file (%d).\n", GetLastError());
     exit(0);
  }

  /*allocate the 2D buffer so that we can access the shared memory*/
  {
     texturePMF[0] = (PixelPMF **)malloc(sizeof(PixelPMF *)*Height);
     if(texturePMF[0] == NULL)
     {
        printf("Error %s line %d allocating memory\n",__FUNCTION__,__LINE__);
        exit(0);
     }
     texturePMF[1] = (PixelPMF **)malloc(sizeof(PixelPMF *)*Height);
     if(texturePMF[1] == NULL)
     {
        printf("Error %s line %d allocating memory\n",__FUNCTION__,__LINE__);
        exit(0);
     }
     depthPMF[0] = (PixelPMF **)malloc(sizeof(PixelPMF *)*Height);
     if(depthPMF[0] == NULL)
     {
        printf("Error %s line %d allocating memory\n",__FUNCTION__,__LINE__);
        exit(0);
     }
     depthPMF[1] = (PixelPMF **)malloc(sizeof(PixelPMF *)*Height);
     if(depthPMF[1] == NULL)
     {
        printf("Error %s line %d allocating memory\n",__FUNCTION__,__LINE__);
        exit(0);
     }

     /*assign the 2d array*/
     for(h=0; h<Height; h++)
     {
        texturePMF[0][h] = &texturePMF1D[0][(h+IMG_PAD_SIZE_Y)*(Width2+2*IMG_PAD_SIZE_X)+IMG_PAD_SIZE_X];
        texturePMF[1][h] = &texturePMF1D[1][(h+IMG_PAD_SIZE_Y)*(Width2+2*IMG_PAD_SIZE_X)+IMG_PAD_SIZE_X];
        depthPMF[0][h] = &depthPMF1D[0][(h+IMG_PAD_SIZE_Y)*(Width2+2*IMG_PAD_SIZE_X)+IMG_PAD_SIZE_X];
        depthPMF[1][h] = &depthPMF1D[1][(h+IMG_PAD_SIZE_Y)*(Width2+2*IMG_PAD_SIZE_X)+IMG_PAD_SIZE_X];
     }
  }

  /*allocate the merged pixel moment buffer*/
  m_mergedPixelMoments.width = Width2;
  m_mergedPixelMoments.height = Height;
  m_mergedPixelMoments.firstMoment = (double *)malloc(sizeof(double)*Height*Width2);
  m_mergedPixelMoments.secondMoment = (double *)malloc(sizeof(double)*Height*Width2);
  if( (m_mergedPixelMoments.firstMoment==NULL) || (m_mergedPixelMoments.secondMoment==NULL) )
  {
     printf("Error %s allocating pixel moment memory(Vertical)\n",__FUNCTION__);
     exit(0);
  }
  memset(m_mergedPixelMoments.firstMoment, 0, sizeof(double)*Height*Width2);
  memset(m_mergedPixelMoments.secondMoment, 0, sizeof(double)*Height*Width2);

  for(k=0; k<2; k++)
  {
     /*allocate the probability outcome buffer*/
     m_textureProbMap[k].width = Width2;
     m_textureProbMap[k].height = Height;
     m_textureProbMap[k].outcomes = (PROBABILITY_OUTCOME **)malloc(sizeof(PROBABILITY_OUTCOME *)*Height);

     if(m_textureProbMap[k].outcomes==NULL)
     {
        printf("Error %s allocating texture probability map memory(Vertical)\n",__FUNCTION__);
        exit(0);
     }

     for(h=0; h<Height; h++)
     {
        m_textureProbMap[k].outcomes[h] = (PROBABILITY_OUTCOME *)malloc(sizeof(PROBABILITY_OUTCOME)*Width2);
        if(m_textureProbMap[k].outcomes[h]==NULL)
        {
           printf("Error %s allocating texture or depth probability map memory(Horizontal %d)\n", __FUNCTION__, h);
           exit(0);
        }
        memset(m_textureProbMap[k].outcomes[h], 0, sizeof(PROBABILITY_OUTCOME)*Width2);
        for(w=0; w<Width2; w++)
        {
           m_textureProbMap[k].outcomes[h][w].prob = (double *)malloc(sizeof(double)*LIST_INITIAL_CAPACITY);
           m_textureProbMap[k].outcomes[h][w].Dvalues = (unsigned char *)malloc(sizeof(unsigned char)*LIST_INITIAL_CAPACITY);
           m_textureProbMap[k].outcomes[h][w].x = (unsigned short *)malloc(sizeof(unsigned short)*LIST_INITIAL_CAPACITY);
           m_textureProbMap[k].outcomes[h][w].y = (unsigned short *)malloc(sizeof(unsigned short)*LIST_INITIAL_CAPACITY);
           m_textureProbMap[k].outcomes[h][w].capacity = LIST_INITIAL_CAPACITY;
           if( (m_textureProbMap[k].outcomes[h][w].prob==NULL) || (m_textureProbMap[k].outcomes[h][w].Dvalues==NULL) || (m_textureProbMap[k].outcomes[h][w].x==NULL) || (m_textureProbMap[k].outcomes[h][w].y==NULL) )
           {
              printf("Error %s allocation error\n",__FUNCTION__);
              exit(0);
           }
        }
     }
  }
#endif

  return 0;
}

#if ENABLE_VSDE
void CViewInterpolation1D::probabilityMapMomentMemoryReset()
{
   int i;
   int j;
   int k;

   for(k=0; k<2; k++)
   {
      if(m_textureProbMap[k].outcomes!=NULL)
      {
         for(i=0; i<m_textureProbMap[k].height; i++)
         {
            for(j=0; j<m_textureProbMap[k].width; j++)
            {
               m_textureProbMap[k].outcomes[i][j].numberOutcomes = 0;
               m_textureProbMap[k].outcomes[i][j].noMapProb = 1;
            }
         }
      }
   }
   if(m_mergedPixelMoments.firstMoment!=NULL)
   {
      memset(m_mergedPixelMoments.firstMoment, 0, sizeof(double)*m_mergedPixelMoments.width*m_mergedPixelMoments.height);
   }
   if(m_mergedPixelMoments.secondMoment!=NULL)
   {
      memset(m_mergedPixelMoments.secondMoment, 0, sizeof(double)*m_mergedPixelMoments.width*m_mergedPixelMoments.height);
   }
}
#endif

/*
 * \brief
 *    Map a pixel to the virtual view using the z buffer
 *
 * \input
 * \param x, y
 *    The image cooredinate of the pixel to be mapped
 * \param d
 *    The depth level of the pixel to be mapped
 * \param z
 *    The depth value of the pixel to be mapped
 * \param RefY
 *    The reference view buffer, Y. Note that UV components are stored in RefU[ViewId] and RefV[ViewId]
 * \param ViewId
 *    0 means left view; 1 means right view
 * \param dk
 *    The disparity of the pixel to be mapped
 * \param flooring
 *    Indicate if floor() or ceil() to be used in the mapping process
 *
 * \output
 * \class member SynY[ViewId], SynU[ViewId], SynV[ViewId]
 *    The warped picture is stored. 4:4:4
 *
 * \return
 *    None
 *
 * \Modification history:
 *    \Zhejiang Univ, May 2009
 *        Bug fix on pixel mapping
 *        Must work together with new version ForwardWarpSingleView() function as it need a correct warping direction.
 */
void CViewInterpolation1D::PixelMapping(int x, int y, unsigned char d, float z, unsigned char* RefY, unsigned char* RefU, unsigned char* RefV, int ViewId, double dk, int flooring)
{
  int   x2, y2;
  int   ind, ind2;
  bool  nearToCamera;
  bool  bSplatting;
#if !VSRS3_ORIGINAL
  int   left_pixel, right_pixel, left2_pixel, right2_pixel;
#endif
  int   line_size = Width2*SubPelOption;
  int   enable_warp_flag=2;
  int   depth_diff_th = 10;
  int   average_depth = 0;

  // ind = (x,y) is the pixel position in ref view
  ind = x+y*Width2;

  // Do splatting?
  if (SplattingOption == 0)
    bSplatting = false;
  else if (SplattingOption == 1)
    bSplatting = true;
  else if (SplattingOption == 2)
  {
     // check if the ref pixel is boundary, revised by Yin (SplattingDepthThreshold: a sequence adaptive parameter)
    if (d < SplattingDepthThreshold || IsBoundary(BoundaryMask[ViewId], x, y) || x < 30 || x > Width2-30 )
      bSplatting = true;
    else
      bSplatting = false;
  }

  // Calc the position in syn view
  if (bSplatting)
  {
    if (flooring)
      x2 = (int)(floor( (x - dk) * SubPelOption));
    else
      x2 = (int)(ceil ( (x - dk) * SubPelOption));
  }
  else
    x2 = (int)(floor( (x - dk) * SubPelOption + 0.5));

  y2 = y;

  if ( x2 < line_size && x2 >= 0 &&
       y2<Height              && y2>=0    ) // (x2,y2) must fall within the picture
  {

     ind2 = x2+y2*line_size;
    nearToCamera = false;
    if (Mask[ViewId][ind2]==NOTHOLEPIXEL)
    {

#if USE_ZMAP
      if (Znear[LEFTVIEW] > 0 && Zmap[ViewId][ind2] > z)
        nearToCamera = true;
      if (Znear[LEFTVIEW] < 0 && Zmap[ViewId][ind2] < z)
        nearToCamera = true;

      if (Dmap[ViewId][ind2] < d-3)  //if depth value of current pixel is larger, it covers the previous result
        nearToCamera = true;
#else
      if (Dmap[ViewId][ind2] < d)  //if depth value of current pixel is larger, it covers the previous result
        nearToCamera = true;
#endif
    }

    //pixel mapping
#if VSRS3_ORIGINAL
    if (Mask[ViewId][ind2]==HOLEPIXEL || // (x2,y2) is not filled with any value yet
        nearToCamera) // (x2,y2) was filled, but the new pixel is near to the camera  Mask[ViewId][ind2]==NOTHOLEPIXEL && Zmap[ViewId][ind2] > z
    {
      SynY[ViewId][ind2] = RefY[ind];
      SynU[ViewId][ind2] = RefU[ind];
      SynV[ViewId][ind2] = RefV[ind];
      Mask[ViewId][ind2] = NOTHOLEPIXEL;
      Dmap[ViewId][ind2] = d;
#if USE_ZMAP
      Zmap[ViewId][ind2] = z;
#endif
    }
#else // related to "VSRS3_ORIGINAL", And in current version, LDV data generation is not supported
    //for HOLEPIXEL, revised to clean noise background pixels
     if( Mask[ViewId][ind2] == HOLEPIXEL )// (x2,y2) is not filled with any value yet, should notice it is not a background pixel
    {
      //constrain
       left_pixel = CLIP(x2-1, 0, line_size-1)+y2*line_size;
       left2_pixel= CLIP(x2-2, 0, line_size-1)+y2*line_size;
      right_pixel= CLIP(x2+1, 0, line_size-1)+y2*line_size;
       right2_pixel=CLIP(x2+2, 0, line_size-1)+y2*line_size;

      //filter wrong background pixels caused by per-pixel processing
      //do not warp the pixel, because it is a suspicious pixel in the gap of foreground object
       if( Mask[ViewId][left_pixel] == NOTHOLEPIXEL && Mask[ViewId][right_pixel] == NOTHOLEPIXEL && WarpEnhancementOption == 1)
      {
        average_depth = (Dmap[ViewId][left_pixel]+Dmap[ViewId][right_pixel])/2;
        depth_diff_th = (int)((average_depth-Minz)*0.05+0.5);
          if( abs( Dmap[ViewId][left_pixel]-Dmap[ViewId][right_pixel]) < depth_diff_th && abs(d-average_depth)> depth_diff_th )
          {
             enable_warp_flag = 0;
             SynY[ViewId][ind2] = (int)((SynY[ViewId][left_pixel]+SynY[ViewId][right_pixel])/2);
             SynU[ViewId][ind2] = (int)((SynU[ViewId][left_pixel]+SynU[ViewId][right_pixel])/2);
             SynV[ViewId][ind2] = (int)((SynV[ViewId][left_pixel]+SynV[ViewId][right_pixel])/2);
             Mask[ViewId][ind2] = NOTHOLEPIXEL;
             Dmap[ViewId][ind2] = (int)((Dmap[ViewId][left_pixel]+Dmap[ViewId][right_pixel])/2);
             CountHOLEFILLPIXEL++;
#if USE_ZMAP
          Zmap[ViewId][ind2] = (int)((Zmap[ViewId][left_pixel]+Zmap[ViewId][right_pixel])/2);
#endif
          }
          else
             enable_warp_flag = 1;
      }
       else if( Mask[ViewId][left_pixel] == NOTHOLEPIXEL && Mask[ViewId][right2_pixel] == NOTHOLEPIXEL && WarpEnhancementOption == 1)
      {
        average_depth = (Dmap[ViewId][left_pixel]+Dmap[ViewId][right2_pixel])/2;
        depth_diff_th = (int)((average_depth-Minz)*0.08+0.5);
        if( abs( Dmap[ViewId][left_pixel]-Dmap[ViewId][right2_pixel]) < depth_diff_th && abs(d-average_depth)> depth_diff_th/2)
          {
             enable_warp_flag = 0;
             SynY[ViewId][ind2] = (int)((SynY[ViewId][left_pixel]+SynY[ViewId][right2_pixel])/2);
             SynU[ViewId][ind2] = (int)((SynU[ViewId][left_pixel]+SynU[ViewId][right2_pixel])/2);
             SynV[ViewId][ind2] = (int)((SynV[ViewId][left_pixel]+SynV[ViewId][right2_pixel])/2);
             Mask[ViewId][ind2] = NOTHOLEPIXEL;
             Dmap[ViewId][ind2] = (int)((Dmap[ViewId][left_pixel]+Dmap[ViewId][right2_pixel])/2);
             CountHOLEFILLPIXEL++;
#if USE_ZMAP
          Zmap[ViewId][ind2] = (int)((Zmap[ViewId][left_pixel]+Zmap[ViewId][right2_pixel])/2);
#endif
          }
        else
             enable_warp_flag = 1;
      }
       else if( Mask[ViewId][left2_pixel] == NOTHOLEPIXEL && Mask[ViewId][right_pixel] == NOTHOLEPIXEL && WarpEnhancementOption == 1)
      {
        average_depth = (Dmap[ViewId][left2_pixel]+Dmap[ViewId][right_pixel])/2;
        depth_diff_th = (int)((average_depth-Minz)*0.08+0.5);
        if( abs( Dmap[ViewId][left2_pixel]-Dmap[ViewId][right_pixel]) < depth_diff_th && abs(d-average_depth)> depth_diff_th/2)
          {
             enable_warp_flag = 0;
             SynY[ViewId][ind2] = (int)((SynY[ViewId][left2_pixel]+SynY[ViewId][right_pixel])/2);
             SynU[ViewId][ind2] = (int)((SynU[ViewId][left2_pixel]+SynU[ViewId][right_pixel])/2);
             SynV[ViewId][ind2] = (int)((SynV[ViewId][left2_pixel]+SynV[ViewId][right_pixel])/2);
             Mask[ViewId][ind2] = NOTHOLEPIXEL;
             Dmap[ViewId][ind2] = (int)((Dmap[ViewId][left2_pixel]+Dmap[ViewId][right_pixel])/2);
             CountHOLEFILLPIXEL++;
#if USE_ZMAP
          Zmap[ViewId][ind2] = (int)((Zmap[ViewId][left2_pixel]+Zmap[ViewId][right_pixel])/2);
#endif
          }
        else
             enable_warp_flag = 1;
      }
       else
          enable_warp_flag = 1;

      //enable to warp current pixel if the pixel is not belong to the area around strong depth edge
      //aiming to clean boundary noise causing by slow depth change or depth edge be a little inside of texture edge
       if( enable_warp_flag == 1)
       {
         //if the pixel is at left side of a RISE depth edge or at right side of a FALL depth edge, do not warp the pixel
        if( SplattingOption != 2 && CleanNoiseOption != 1)  // bug fixed July 16th: when SplattingOption == 0 or 1 && CleanNoiseOption == 0, no boundary detection was made
        {
          SynY[ViewId][ind2] = RefY[ind];
             SynU[ViewId][ind2] = RefU[ind];
             SynV[ViewId][ind2] = RefV[ind];
             Mask[ViewId][ind2] = NOTHOLEPIXEL;
             Dmap[ViewId][ind2] = d;
#if USE_ZMAP
          Zmap[ViewId][ind2] = z;
#endif
        }
        else  //Boundary aware splatting
        {
        if( BoundaryMask[ViewId][ind] == SUSPECT && CleanNoiseOption == 1)
              ;//do nothing
          else
    {
             SynY[ViewId][ind2] = RefY[ind];
             SynU[ViewId][ind2] = RefU[ind];
             SynV[ViewId][ind2] = RefV[ind];
             Mask[ViewId][ind2] = NOTHOLEPIXEL;
             Dmap[ViewId][ind2] = d;
#if USE_ZMAP
          Zmap[ViewId][ind2] = z;
#endif
    }
       }
       }
    }//end of if( Mask[ViewId][ind2] == HOLEPIXEL )
     else if( nearToCamera) // (x2,y2) was filled, but the new pixel is near to the camera  Mask[ViewId][ind2]==NOTHOLEPIXEL && Zmap[ViewId][ind2] > z )
     {
       if( SplattingOption != 2 && CleanNoiseOption != 1)  // bug fixed July 16th: when SplattingOption == 0 or 1 && CleanNoiseOption == 0 , no boundary detection was made
      {
        SynY[ViewId][ind2] = RefY[ind];
           SynU[ViewId][ind2] = RefU[ind];
           SynV[ViewId][ind2] = RefV[ind];
           Mask[ViewId][ind2] = NOTHOLEPIXEL;
           Dmap[ViewId][ind2] = d;
#if USE_ZMAP
        Zmap[ViewId][ind2] = z;
#endif
      }
      else
      {
       if( BoundaryMask[ViewId][ind] == SUSPECT && CleanNoiseOption == 1) //
            ;//do nothing
       else
       {
          SynY[ViewId][ind2] = RefY[ind];
          SynU[ViewId][ind2] = RefU[ind];
           SynV[ViewId][ind2] = RefV[ind];
           Mask[ViewId][ind2] = NOTHOLEPIXEL;
           Dmap[ViewId][ind2] = d;
#if USE_ZMAP
        Zmap[ViewId][ind2] = z;
#endif
        }
  }
}
#endif
  }//end of if ( x2<line_size && x2>=0 ...)
}

#if ENABLE_VSDE
/*A connection is made to node*/
/*Value: value propagated over*/
/*prob: probability propagated over*/
void addToProbabilityMap(PROBABILITY_OUTCOME *node, unsigned char Dvalue, unsigned short x, unsigned short y, double prob)
{
   if(node->numberOutcomes == node->capacity)
   {
      double *newProb;
      unsigned char *newDValues;
      unsigned short *newX;
      unsigned short *newY;

      node->capacity = node->capacity + LIST_INITIAL_CAPACITY;

      newProb = (double *)malloc(sizeof(double)*node->capacity);
      newDValues = (unsigned char*)malloc(sizeof(unsigned char)*node->capacity);
      newX = (unsigned short*)malloc(sizeof(unsigned short)*node->capacity);
      newY = (unsigned short*)malloc(sizeof(unsigned short)*node->capacity);
      if((newProb==NULL) || (newDValues==NULL) || (newX==NULL) || (newY==NULL))
      {
         printf("Error allocation memory %s %s\n",__FUNCTION__,__LINE__);
         exit(0);
      }
      if(node->prob!=NULL)
      {
         memcpy(newProb, node->prob, sizeof(double)*node->numberOutcomes);
      }
      if(node->Dvalues!=NULL)
      {
         memcpy(newDValues, node->Dvalues, sizeof(unsigned char)*node->numberOutcomes);
      }
      if(node->x!=NULL)
      {
         memcpy(newX, node->x, sizeof(unsigned short)*node->numberOutcomes);
      }
      if(node->y!=NULL)
      {
         memcpy(newY, node->y, sizeof(unsigned short)*node->numberOutcomes);
      }
      newProb[node->numberOutcomes] = prob;
      newDValues[node->numberOutcomes] = Dvalue;
      newX[node->numberOutcomes] = x;
      newY[node->numberOutcomes] = y;
      if(node->prob!=NULL)
      {
         free(node->prob);
      }
      if(node->Dvalues!=NULL)
      {
         free(node->Dvalues);
      }
      if(node->x!=NULL)
      {
         free(node->x);
      }
      if(node->y!=NULL)
      {
        free(node->y);
      }

      node->prob = newProb;
      node->Dvalues = newDValues;
      node->x = newX;
      node->y = newY;
      node->numberOutcomes++;
   }
   else
   {
      node->prob[node->numberOutcomes] = prob;
      node->Dvalues[node->numberOutcomes] = Dvalue;
      node->x[node->numberOutcomes] = x;
      node->y[node->numberOutcomes] = y;
      node->numberOutcomes++;
   }
}




typedef struct
{
   double prob;
   unsigned char Dvalue;
   unsigned short x;
   unsigned short y;
   unsigned int ID;
}PROB_VAL_PAIR;

int compareID(const void* p1, const void* p2)/*comparison based on ID*/
{
   PROB_VAL_PAIR *temp1 = (PROB_VAL_PAIR *)p1;
   PROB_VAL_PAIR *temp2 = (PROB_VAL_PAIR *)p2;

   if(temp1->ID==temp2->ID)
   {
      return 0;
   }
   if(temp1->ID<temp2->ID)
   {
      return -1;
   }
   if(temp1->ID>temp2->ID)
   {
      return 1;
   }

   printf("ERROR %s\n",__FUNCTION__);
   exit(0);
   return 0;
}

void processProbabilityMap(PROBABILITY_OUTCOME *node, int width, int height, bool debugFlag)
{
   if(node->numberOutcomes!=0)
   {
      int i;
      int j;

      PROB_VAL_PAIR *pairsID;    /*nodes sorted based on ID*/
      double tempNoMapProb;
      double tempMapProb;
      int numberNodesConnected;
      int numberNodesRemoved; //some nodes have 0 probability so we remove them

      /*put the depth and texture values and corresponding probabilities in a structure so we can qsort them based on certain rules*/
      numberNodesConnected = node->numberOutcomes;
      pairsID = (PROB_VAL_PAIR *)malloc(sizeof(PROB_VAL_PAIR)*node->numberOutcomes);
      if(pairsID==NULL)
      {
         printf("Error allocation memory %s %s\n",__FUNCTION__,__LINE__);
         exit(0);
      }
      for(i=0; i<node->numberOutcomes; i++)
      {
         pairsID[i].prob = node->prob[i];
         pairsID[i].Dvalue = node->Dvalues[i];
         pairsID[i].x = node->x[i];
         pairsID[i].y = node->y[i];
         pairsID[i].ID = width*((int)node->y[i]) + node->x[i];
      }
      qsort(pairsID, node->numberOutcomes, sizeof(PROB_VAL_PAIR), compareID);

      /*probability will be reassigned so we reset the number of outcomes*/
      node->numberOutcomes=0;
      numberNodesRemoved=0;

      /*Used to find the probabilities when no mapping occurs*/
      node->noMapProb = 1;
      tempNoMapProb = pairsID[0].prob;
      for(i=0; i<numberNodesConnected; i++)
      {
         unsigned int lastID;
         double backwardProb;

         backwardProb = pairsID[i].prob;
         tempMapProb = 0;
         lastID = 0xFFFFFFFF;
         for(j=0; j<numberNodesConnected; j++)
         {
            if(pairsID[j].ID != pairsID[i].ID)
            {
               if(pairsID[j].ID > pairsID[i].ID) /*we are looking at the nodes added to the graph later than node i*/
               {
                  if(pairsID[j].Dvalue > pairsID[i].Dvalue)/*excluded nodes*/
                  {
                     if(pairsID[j].ID != lastID)
                     {
                        backwardProb = backwardProb*(1-tempMapProb);
                        tempMapProb = pairsID[j].prob;
                     }
                     else
                     {
                        tempMapProb = tempMapProb + pairsID[j].prob;
                     }
                     lastID = pairsID[j].ID;
                  }
               }
               else /*we are looking at the nodes added to the graph earlier than node i*/
               {
                  if(pairsID[j].Dvalue >= pairsID[i].Dvalue)/*excluded nodes*/
                  {
                     if(pairsID[j].ID != lastID)
                     {
                        backwardProb = backwardProb*(1-tempMapProb);
                        tempMapProb = pairsID[j].prob;
                     }
                     else
                     {
                        tempMapProb = tempMapProb + pairsID[j].prob;
                     }
                     lastID = pairsID[j].ID;
                  }
               }
            }
         }
         backwardProb = backwardProb*(1-tempMapProb);

         /*record the backward mapping probability and mapping position*/
         if(backwardProb!=0)
         {
         node->prob[node->numberOutcomes] = backwardProb;
         node->x[node->numberOutcomes] = pairsID[i].x;
         node->y[node->numberOutcomes] = pairsID[i].y;
         node->numberOutcomes++;
         }
         else
         {
            numberNodesRemoved++;
         }

         /*accumulate the no map probability*/
         if(i>0)
         {
            if(pairsID[i].ID != pairsID[i-1].ID)
            {
               node->noMapProb = node->noMapProb*(1-tempNoMapProb);
               tempNoMapProb = pairsID[i].prob;
            }
            else
            {
               tempNoMapProb = tempNoMapProb + pairsID[i].prob;
            }
         }
      }
      node->noMapProb = node->noMapProb*(1-tempNoMapProb);


      /*do an error checking*/
      if(numberNodesConnected!=(node->numberOutcomes+numberNodesRemoved))
      {
         printf("Error %s we could be missing nodes\n",__FUNCTION__);
         exit(0);
      }
#if ENABLE_PROB_CHECKING
      {
         /*debug*/
         double total=0;
         for(i=0; i<node->numberOutcomes; i++)
         {
            total = total + node->prob[i];
         }
         total = total + node->noMapProb;
         if((total>=1.00001) || (total<=0.99999) || debugFlag)
         {
            printf("\nError %s prob distribution invalid %f %d nodes connected\n", __FUNCTION__, total, numberNodesConnected);
            for(i=0; i<node->numberOutcomes; i++)
            {
               printf("x:%u y:&u prob:%f\n",node->x[i],node->y[i],node->prob[i]);
            }
            printf("NoMapProb:%f\n",node->noMapProb);

            for(i=0; i<numberNodesConnected; i++)
            {
               printf("ID:%d x:%d y:%d Prob:%f\n", pairsID[i].ID, pairsID[i].x, pairsID[i].y, pairsID[i].prob);
            }
            exit(0);
         }
      }
#endif
      free(pairsID);
   }
   else
   {
      if(node->noMapProb!=1)
      {
         printf("Error %s Node with no connection has noMapProb:%f initialization was not properly done\n", __FUNCTION__, node->noMapProb);
         exit(0);
      }
   }
}
#endif


/*
 * \brief
 *    Warp one view
 *
 * \input
 * \param Ref
 *    The reference view buffer
 * \param RefDepth
 *    The reference depth view buffer
 * \param ViewId
 *    0 means left view; 1 means right view
 *
 * \output
 * \class member SynY[ViewId], SynU[ViewId], SynV[ViewId]
 *    The warped picture is stored. 4:4:4
 *
 * \return
 *    None
 *
 * \Modification history:
 *    \Zhejiang Univ, May 2009
 *      Warping direction consideration
 *      Change the warping order: if Warp to right side, pixel mapping from left to right; otherwise, the opposite.
 */
void CViewInterpolation1D::ForwardWarpSingleView(int ViewId)
{
  int x, y;
  int ind;
  int depthLevel;
  double dk;
  double z;
  unsigned char*  RefY = &RefView[ViewId][0];
  unsigned char*  RefU = &RefView[ViewId][Width2 * Height];
  unsigned char*  RefV = &RefView[ViewId][Width2 * Height * 2];

#if VSRS3_ORIGINAL
  for (y = 0; y < Height; y++)
  for (x = 0; x < Width2;  x++)
  {
    // ind = (x,y) is the pixel position in ref view
    ind = x+y*Width2;
    // Calc real depth z from depth image
    depthLevel = RefDepth[ViewId][ind];
    z = 1.0 / ( (depthLevel/255.0) * (1/Znear[ViewId] - 1/Zfar[ViewId]) + (1/Zfar[ViewId]) );
    //if (1)
    //{
    //  // get the depth value relative to cameras
    //  z = z + 2730.850523;
    //}

    dk = (FocalLength * LTranslation[ViewId] / z) - duPrincipal[ViewId];

    if (UpsampleRefs == 2)
      dk = dk + dk;
    else if (UpsampleRefs == 4)
      dk = dk * 4;

    PixelMapping(x, y, RefDepth[ViewId][ind], (float)z, RefY, RefU, RefV, ViewId, dk, 1);
    if (SplattingOption)
      PixelMapping(x, y, RefDepth[ViewId][ind], (float)z, RefY, RefU, RefV, ViewId, dk, 0);

#if ENABLE_VSDE
      {
         int i;
         double newz;
         double newdk;
         int x2;

         for(i=0; i<256; i++)
         {
            if(depthPMF[ViewId][y][x].map[i]!=0)
            {
               int newDepthVal = i;

               newz = 1.0 / ( (newDepthVal/255.0) * (1/Znear[ViewId] - 1/Zfar[ViewId]) + (1/Zfar[ViewId]) );
               newdk = (FocalLength * LTranslation[ViewId] / newz) - duPrincipal[ViewId];
               x2 = (int)(floor( (x - newdk) * SubPelOption + 0.5));

               if( x2 < Width2 && x2 >= 0 )
               {
                  addToProbabilityMap(&m_textureProbMap[ViewId].outcomes[y][x2], (unsigned char)newDepthVal, (unsigned short)x, (unsigned short)y, depthPMF[ViewId][y][x].prob[i]);
               }
            }
         }
      }
#endif
  }
#if ENABLE_VSDE
  {
     for(y=0; y<Height; y++)
     {
       for(x=0; x<Width2; x++)
       {
          processProbabilityMap(&m_textureProbMap[ViewId].outcomes[y][x], Width2, Height, false/*(h==754)&&(w==188)*/ );
       }
     }
  }
#endif

#else // related to "if VSRS3_ORIRINAL"
  CountHOLEFILLPIXEL = 0;
  //determine warp direction
  if(WarpEnhancementOption == 0)
    WarpToRight = 0;
  else
  {
    if(ViewId == LEFTVIEW)
       WarpToRight = 1;
    else
       WarpToRight = 0;
  }

  //start to warp each pixel
  if( WarpToRight == 0) // left view-- from left 2 right
    for (y = 0; y < Height; y++)
      for (x = 0; x < Width2;  x++)
      {
        // ind = (x,y) is the pixel position in ref view
        ind = x+y*Width2;
        // Calc real depth z from depth image
        depthLevel = RefDepth[ViewId][ind];
        z = 1.0 / ( (depthLevel/255.0) * (1/Znear[ViewId] - 1/Zfar[ViewId]) + (1/Zfar[ViewId]) );

        // Cacl dk; round to integer pixel postionns;
        dk = (FocalLength * LTranslation[ViewId] / z) - duPrincipal[ViewId];

        if (UpsampleRefs == 2)
          dk = dk + dk;
        else if (UpsampleRefs == 4)
          dk = dk * 4;

         PixelMapping(x, y, RefDepth[ViewId][ind], (float)z, RefY, RefU, RefV, ViewId, dk, 1); //09-02-25
        if (SplattingOption)
          PixelMapping(x, y, RefDepth[ViewId][ind], (float)z, RefY, RefU, RefV, ViewId, dk, 0);
      }
  else  //if(WarpToRight == 1) // right view-- from right 2 left
    for (y = 0; y < Height; y++)
      for (x = Width2-1; x>=0; x--)
      {
        // ind = (x,y) is the pixel position in ref view
        ind = x+y*Width2;
        // Calc real depth z from depth image
        depthLevel = RefDepth[ViewId][ind];
        z = 1.0 / ( (depthLevel/255.0) * (1/Znear[ViewId] - 1/Zfar[ViewId]) + (1/Zfar[ViewId]) );

        // Cacl dk; round to integer pixel postionns;
         dk = (FocalLength * LTranslation[ViewId] / z) - duPrincipal[ViewId];

        if (UpsampleRefs == 2)
          dk = dk + dk;
        else if (UpsampleRefs == 4)
          dk = dk * 4;

         PixelMapping(x, y, RefDepth[ViewId][ind], (float)z, RefY, RefU, RefV, ViewId, dk, 0);  //Note: here must be flooring == 0 first
        if (SplattingOption)
          PixelMapping(x, y, RefDepth[ViewId][ind], (float)z, RefY, RefU, RefV, ViewId, dk, 1);
      }

#if DEBUG_ZHEJIANG
  if( WarpEnhancementOption == 1)
  {
    if(ViewId == LEFTVIEW)
      fprintf(stderr, "\n Count Holefill pixel in LEFT View 1D warp = %d", CountHOLEFILLPIXEL);
    else
      fprintf(stderr, "\n Count Holefill pixel in RIGHT View 1D warp = %d", CountHOLEFILLPIXEL);
  }
#endif

#endif // related to "if VSRS3_ORIRINAL"

}

/*
 * \brief
 *    Perform forward warping
 *
 * \input
 *    The images are arranged in a one dimension array
 * \param RefLeft
 *    The left ref view image
 * \param RefRight
 *    The right ref view image
 * \param RefDepthLeft
 *    The left ref depth view image
 * \param RefDepthRight
 *    The right ref depth view image
 *
 * \output
 * \class member: SynY[i], SynU[i], SynV[i]
 *    Warped view
 *    i = 0 is from left view
 *    i = 1 is from right view
 *  \class member: Mask[i]
 *    Hole positions
 *    i = 0 is from left view
 *    i = 1 is from right view
 *  \class member: Zmap[i]
 *    The depth value of warped pixels
 *    i = 0 is from left view
 *    i = 1 is from right view
 *  \class member: Dmap[i]
 *    The depth level of warped pixels
 *    i = 0 is from left view
 *    i = 1 is from right view
 *
 * \return
 *    None
 */
void CViewInterpolation1D::ForwardWarp() // unsigned char* RefLeft, unsigned char* RefRight, unsigned char* RefDepthLeft, unsigned char* RefDepthRight
{
  int i;

  for (i = 0; i < 3; i++)
  {
    memset(SynY[i], 0,   Width2*Height*SubPelOption);
    memset(SynU[i], 128, Width2*Height*SubPelOption);
    memset(SynV[i], 128, Width2*Height*SubPelOption);
    memset(Mask[i], HOLEPIXEL, Width2*Height*SubPelOption);
    memset(Dmap[i], 0,   Width2*Height*SubPelOption*sizeof(unsigned char));
#if USE_LDV
    memset(OccY[i], 0,   Width2*Height*SubPelOption);
    memset(OccU[i], 128, Width2*Height*SubPelOption);
    memset(OccV[i], 128, Width2*Height*SubPelOption);
    memset(OMask[i], HOLEPIXEL, Width2*Height*SubPelOption);
    memset(ODmap[i], 0,   Width2*Height*SubPelOption*sizeof(unsigned char));
#endif
#if USE_ZMAP
    memset(Zmap[i], 0,   Width2*Height*SubPelOption*sizeof(float));
  #if USE_LDV
    memset(OccZmap[i], 0,   Width2*Height*SubPelOption*sizeof(float));
  #endif
#endif
  }


  //find out the max and min z value in current frame
  FindDepthMaxMin(RefDepth[LEFTVIEW], LEFTVIEW); //left view must have a similar depth structure as that of the right view

  //depth map edge detection before forward warping if SplattingOption==2 or CleanNoiseOption==1
  if(SplattingOption == 2 || CleanNoiseOption == 1)
  {
    double es = 0.15; //EdgeStrength
    int diff_z= Maxz-Minz;
    int th=(int)(es*diff_z /UpsampleRefs+1); //depth edge threshold = 15% * z_value_range / UpsampleRefs

    for(i=0; i<2; i++)
    {
       memset( BoundaryMask[i],  NONE, Width2*Height);
      DetectBoundary( BoundaryMask[i],  RefDepth[i], Width2, Height, th);
    }
  }

  //forward warping left and right view
  // From Left View
  ForwardWarpSingleView(LEFTVIEW); // RefLeft,  RefDepthLeft,

  // From Right View
  ForwardWarpSingleView(RGHTVIEW); // RefRight, RefDepthRight,
}

/*
 * \brief
 *    Count the number of hole pixels around the specified position
 *
 * \input
 * \param MaskMap
 *    The hole mask tabl
 * \param x, y
 *    Specify the position to count number of holes
 * \param width
 *    The width of the picture
 *
 * \return
 *    Number of hole pixels
 */
int CViewInterpolation1D::CountHolePixels(unsigned char* MaskMap, int x, int y, int width)
{
  int blksizey = 3;
  int blksizex = blksizey*SubPelOption*UpsampleRefs;
  int startx, endx;
  int starty, endy;
  int i, j;
  int counter = 0;
  int ind;

  startx = x - blksizex;
  endx   = x + blksizex;
  starty = y - blksizey;
  endy   = y + blksizey;

  if (startx < 0) startx = 0;
  if (endx   > width) endx = width;
  if (starty < 0) starty = 0;
  if (endy   > Height)  endy = Height;

  for (j = starty; j < endy; j++)
  {
    ind = j*width + startx;
    for (i = startx; i < endx; i++)
    {
      counter += MaskMap[ind];
      ind++;
    }
  }

  return counter;
}
/*
 * \brief
 *    Merge the two synthesized view into one
 *
 * \input
 * \class member SynY[i], SynU[i], SynV[i]
 *    The synthesized view, in 4:4:4
 *    i = 0 from left view
 *    i = 1 from right view
 * \class member Mask[i]
 *    The hole map
 *    i = 0 from left view
 *    i = 1 from right view
 * \class member Zmap[i]
 *    The depth map (real depth values stored) warped from
 *    i = 0 from left view
 *    i = 1 from right view
 * \class member Dmap[i]
 *    The depth map (depth levels stored) warped from
 *    i = 0 from left view
 *    i = 1 from right view
 *
 * \output
 * \class member SynY[MERGVIEW], SynU[MERGVIEW], SynV[MERGVIEW]
 *    The unique synthesized picture, in 4:4:4
 * \class member Mask[MERGVIEW]
 *    The unique hole mask
 * \class member Zmap[MERGVIEW]
 *    The unique synthesized depth map (real depth values stored)
 * \class member Dmap[MERGVIEW]
 *    The unique synthesized depth map (depth levels stored)
 *
 * \return
 *    None
 */
void CViewInterpolation1D::Merge()
{
  int x, y;
  int ind;
  double Y, U, V, D;
#if USE_ZMAP
  double Z;
#endif
  int HoleCountLeft, HoleCountRight;
  bool isLeftNearToCamera = true;  // just to avoid compile warning
  int WidthSyn = Width2*SubPelOption;

  ind = 0;
  for (y = 0; y < Height; y++)
  for (x = 0; x < WidthSyn;  x++)
  {
    // The current pixel has two versions
    if (Mask[LEFTVIEW][ind] == NOTHOLEPIXEL && Mask[RGHTVIEW][ind] == NOTHOLEPIXEL)
    {
      if(MergingOption == 0)
      { // Z-buffer only
#if USE_ZMAP
        if (Znear[LEFTVIEW] > 0)
        {
          if (Zmap[LEFTVIEW][ind] <= Zmap[RGHTVIEW][ind])
            isLeftNearToCamera = true;
          else
            isLeftNearToCamera = false;
        }
        else
        {
          if (Zmap[LEFTVIEW][ind] >= Zmap[RGHTVIEW][ind])
            isLeftNearToCamera = true;
          else
            isLeftNearToCamera = false;
        }
#else
        if (Dmap[LEFTVIEW][ind] >= Dmap[RGHTVIEW][ind])
          isLeftNearToCamera = true;
        else
          isLeftNearToCamera = false;
#endif
        if (isLeftNearToCamera)
        {    // Left view is near to the camera
          SynY[MERGVIEW][ind] = SynY[LEFTVIEW][ind];
          SynU[MERGVIEW][ind] = SynU[LEFTVIEW][ind];
          SynV[MERGVIEW][ind] = SynV[LEFTVIEW][ind];
          Dmap[MERGVIEW][ind] = Dmap[LEFTVIEW][ind];
          Mask[MERGVIEW][ind] = NOTHOLEPIXEL;
#if USE_ZMAP
          Zmap[MERGVIEW][ind] = Zmap[LEFTVIEW][ind];
#endif
        }
        // Right view is near to the camera
        else
        {
          SynY[MERGVIEW][ind] = SynY[RGHTVIEW][ind];
          SynU[MERGVIEW][ind] = SynU[RGHTVIEW][ind];
          SynV[MERGVIEW][ind] = SynV[RGHTVIEW][ind];
          Dmap[MERGVIEW][ind] = Dmap[RGHTVIEW][ind];
          Mask[MERGVIEW][ind] = NOTHOLEPIXEL;
#if USE_ZMAP
          Zmap[MERGVIEW][ind] = Zmap[RGHTVIEW][ind];
#endif
        }
      }
      else if(MergingOption==1)
      {// Camera distance as weighting factor
        Y = WeightLeft*(double)SynY[LEFTVIEW][ind]+WeightRight*(double)SynY[RGHTVIEW][ind];
        U = WeightLeft*(double)SynU[LEFTVIEW][ind]+WeightRight*(double)SynU[RGHTVIEW][ind];
        V = WeightLeft*(double)SynV[LEFTVIEW][ind]+WeightRight*(double)SynV[RGHTVIEW][ind];
#if USE_ZMAP
        Z = WeightLeft*(double)Zmap[LEFTVIEW][ind]+WeightRight*(double)Zmap[RGHTVIEW][ind];
#endif
        D = WeightLeft*(double)Dmap[LEFTVIEW][ind]+WeightRight*(double)Dmap[RGHTVIEW][ind];
        SynY[MERGVIEW][ind] = (unsigned char)Y;
        SynU[MERGVIEW][ind] = (unsigned char)U;
        SynV[MERGVIEW][ind] = (unsigned char)V;
        Dmap[MERGVIEW][ind] = (unsigned char)D;
        Mask[MERGVIEW][ind] = NOTHOLEPIXEL;
#if USE_ZMAP
          Zmap[MERGVIEW][ind] = (unsigned char)Z;
#endif
      }

      else if(MergingOption == 2)
      {// Hole counting + Z-buffer

        // Note that because the DE s/w currently output diff depth range for diff views, the performance may be a little worse than expected

        // When the two pixel depthes are close enough, we may do average
        if(abs(Dmap[LEFTVIEW][ind] - Dmap[RGHTVIEW][ind]) <=  DepthThreshold) //camera weighting         Dmap[RGHTVIEW][ind]
        {
          //hole counting
          HoleCountLeft  = CountHolePixels(Mask[LEFTVIEW], x, y, WidthSyn);
          HoleCountRight = CountHolePixels(Mask[RGHTVIEW], x, y, WidthSyn);
          bool One_view = (abs(HoleCountLeft - HoleCountRight) >= HoleCountThreshold*SubPelOption*UpsampleRefs);
          //One_view = false;

          //bool One_view;
          //if (IsBoundary(BoundaryMask[LEFTVIEW], x, y) || IsBoundary(BoundaryMask[RGHTVIEW], x, y))
          // If it is obvious that one synthesized result contains much less holes, it is assumed to be more reliable
          if(One_view)
          {
           if(HoleCountLeft <= HoleCountRight)
           { //pick left, sine less holes from left
             SynY[MERGVIEW][ind] = SynY[LEFTVIEW][ind];
             SynU[MERGVIEW][ind] = SynU[LEFTVIEW][ind];
             SynV[MERGVIEW][ind] = SynV[LEFTVIEW][ind];
             Dmap[MERGVIEW][ind] = Dmap[LEFTVIEW][ind];
             Mask[MERGVIEW][ind] = NOTHOLEPIXEL;
#if USE_ZMAP
             Zmap[MERGVIEW][ind] = Zmap[LEFTVIEW][ind];
#endif
           }
           else
           {//pick right, since less holes from right
              SynY[MERGVIEW][ind] = SynY[RGHTVIEW][ind];
              SynU[MERGVIEW][ind] = SynU[RGHTVIEW][ind];
              SynV[MERGVIEW][ind] = SynV[RGHTVIEW][ind];
              Dmap[MERGVIEW][ind] = Dmap[RGHTVIEW][ind];
              Mask[MERGVIEW][ind] = NOTHOLEPIXEL;
#if USE_ZMAP
              Zmap[MERGVIEW][ind] = Zmap[RGHTVIEW][ind];
#endif
           }
          }

          // Otherwise, do average
          else //camera distance weighting
          {
            Y = WeightLeft*(double)SynY[LEFTVIEW][ind]+WeightRight*(double)SynY[RGHTVIEW][ind];
            U = WeightLeft*(double)SynU[LEFTVIEW][ind]+WeightRight*(double)SynU[RGHTVIEW][ind];
            V = WeightLeft*(double)SynV[LEFTVIEW][ind]+WeightRight*(double)SynV[RGHTVIEW][ind];
#if USE_ZMAP
            Z = WeightLeft*(double)Zmap[LEFTVIEW][ind]+WeightRight*(double)Zmap[RGHTVIEW][ind];
#endif
            D = WeightLeft*(double)Dmap[LEFTVIEW][ind]+WeightRight*(double)Dmap[RGHTVIEW][ind];
            SynY[MERGVIEW][ind] = (unsigned char)Y;
            SynU[MERGVIEW][ind] = (unsigned char)U;
            SynV[MERGVIEW][ind] = (unsigned char)V;
            Dmap[MERGVIEW][ind]= (unsigned char)D;
            Mask[MERGVIEW][ind] = NOTHOLEPIXEL;
#if USE_ZMAP
            Zmap[MERGVIEW][ind]= (float)Z;
#endif
          }
        }
        // When the two pixel depthes are far away enough, we always select one winner
        else
        {
          // find out which is near to the camera
#if USE_ZMAP
          if (Znear[LEFTVIEW] > 0)
          {
            if (Zmap[LEFTVIEW][ind] <= Zmap[RGHTVIEW][ind])
              isLeftNearToCamera = true;
            else
              isLeftNearToCamera = false;
          }
          else
          {
            if (Zmap[LEFTVIEW][ind] >= Zmap[RGHTVIEW][ind])
              isLeftNearToCamera = true;
            else
              isLeftNearToCamera = false;
          }
#else
          if (Dmap[LEFTVIEW][ind] >= Dmap[RGHTVIEW][ind])
            isLeftNearToCamera = true;
          else
            isLeftNearToCamera = false;
#endif
          if (isLeftNearToCamera)
          {    // Left view is near to the camera
            SynY[MERGVIEW][ind] = SynY[LEFTVIEW][ind];
            SynU[MERGVIEW][ind] = SynU[LEFTVIEW][ind];
            SynV[MERGVIEW][ind] = SynV[LEFTVIEW][ind];
#if USE_ZMAP
            Zmap[MERGVIEW][ind] = Zmap[LEFTVIEW][ind];
#endif
            Dmap[MERGVIEW][ind] = Dmap[LEFTVIEW][ind];
            Mask[MERGVIEW][ind] = NOTHOLEPIXEL;
          }
          else
          {// Right view is near to the camera
            SynY[MERGVIEW][ind] = SynY[RGHTVIEW][ind];
            SynU[MERGVIEW][ind] = SynU[RGHTVIEW][ind];
            SynV[MERGVIEW][ind] = SynV[RGHTVIEW][ind];
            Dmap[MERGVIEW][ind] = Dmap[RGHTVIEW][ind];
            Mask[MERGVIEW][ind] = NOTHOLEPIXEL;
#if USE_ZMAP
            Zmap[MERGVIEW][ind] = Zmap[RGHTVIEW][ind];
#endif
          }
        }
      }
#if USE_ZMAP
#if USE_LDV
        if (Znear[LEFTVIEW] > 0)
        {
          if (OccZmap[LEFTVIEW][ind] <= OccZmap[RGHTVIEW][ind])
            isLeftNearToCamera = true;
          else
            isLeftNearToCamera = false;
        }
        else
        {
          if (OccZmap[LEFTVIEW][ind] >= OccZmap[RGHTVIEW][ind])
            isLeftNearToCamera = true;
          else
            isLeftNearToCamera = false;
        }
#endif
#else
#if USE_LDV
        if (ODmap[LEFTVIEW][ind] >= ODmap[RGHTVIEW][ind])
          isLeftNearToCamera = true;
        else
          isLeftNearToCamera = false;
#endif
#endif

#if USE_LDV
        if (isLeftNearToCamera)
        {    // Right view is far from the camera
          OccY[MERGVIEW][ind] = OccY[RGHTVIEW][ind];
          OccU[MERGVIEW][ind] = OccU[RGHTVIEW][ind];
          OccV[MERGVIEW][ind] = OccV[RGHTVIEW][ind];
          ODmap[MERGVIEW][ind] = ODmap[RGHTVIEW][ind];
          OMask[MERGVIEW][ind] = NOTHOLEPIXEL;
#if USE_ZMAP
          OccZmap[MERGVIEW][ind] = OccZmap[RGHTVIEW][ind];
#endif
        }
        // Left view is far from the camera
        else
        {
          OccY[MERGVIEW][ind] = OccY[LEFTVIEW][ind];
          OccU[MERGVIEW][ind] = OccU[LEFTVIEW][ind];
          OccV[MERGVIEW][ind] = OccV[LEFTVIEW][ind];
          ODmap[MERGVIEW][ind] = ODmap[LEFTVIEW][ind];
          OMask[MERGVIEW][ind] = NOTHOLEPIXEL;
#if USE_ZMAP
          OccZmap[MERGVIEW][ind] = OccZmap[LEFTVIEW][ind];
#endif
        }
#endif
    }
    // The current pixel only has one version from the left view
    else if (Mask[LEFTVIEW][ind] == NOTHOLEPIXEL)
    {
        SynY[MERGVIEW][ind] = SynY[LEFTVIEW][ind];
        SynU[MERGVIEW][ind] = SynU[LEFTVIEW][ind];
        SynV[MERGVIEW][ind] = SynV[LEFTVIEW][ind];
        Dmap[MERGVIEW][ind] = Dmap[LEFTVIEW][ind];
        Mask[MERGVIEW][ind] = NOTHOLEPIXEL;
#if USE_ZMAP
        Zmap[MERGVIEW][ind] = Zmap[LEFTVIEW][ind];
#endif

#if USE_LDV
        OccY[MERGVIEW][ind] = OccY[LEFTVIEW][ind];
        OccU[MERGVIEW][ind] = OccU[LEFTVIEW][ind];
        OccV[MERGVIEW][ind] = OccV[LEFTVIEW][ind];
        ODmap[MERGVIEW][ind] = ODmap[LEFTVIEW][ind];
        OMask[MERGVIEW][ind] = NOTHOLEPIXEL;
#if USE_ZMAP
        OccZmap[MERGVIEW][ind] = OccZmap[LEFTVIEW][ind];
#endif
#endif
    }
    // The current pixel only has one version from the right view
    else if (Mask[RGHTVIEW][ind] == NOTHOLEPIXEL)
    {
        SynY[MERGVIEW][ind] = SynY[RGHTVIEW][ind];
        SynU[MERGVIEW][ind] = SynU[RGHTVIEW][ind];
        SynV[MERGVIEW][ind] = SynV[RGHTVIEW][ind];
        Dmap[MERGVIEW][ind] = Dmap[RGHTVIEW][ind];
        Mask[MERGVIEW][ind] = NOTHOLEPIXEL;
#if USE_ZMAP
        Zmap[MERGVIEW][ind] = Zmap[RGHTVIEW][ind];
#endif
#if USE_LDV
        OccY[MERGVIEW][ind] = OccY[RGHTVIEW][ind];
        OccU[MERGVIEW][ind] = OccU[RGHTVIEW][ind];
        OccV[MERGVIEW][ind] = OccV[RGHTVIEW][ind];
        ODmap[MERGVIEW][ind] = ODmap[RGHTVIEW][ind];
        OMask[MERGVIEW][ind] = NOTHOLEPIXEL;
#if USE_ZMAP
        OccZmap[MERGVIEW][ind] = OccZmap[RGHTVIEW][ind];
#endif
#endif
    }

#if USE_LDV
    if(ODmap[MERGVIEW][ind]>Dmap[MERGVIEW][ind])
    {
        if(Dmap[MERGVIEW][ind]>5)
            ODmap[MERGVIEW][ind] = Dmap[MERGVIEW][ind]-5;
        else
            ODmap[MERGVIEW][ind] = Dmap[MERGVIEW][ind];
    }
#if USE_ZMAP
    if(OccZmap[MERGVIEW][ind]<Zmap[MERGVIEW][ind]) OccZmap[MERGVIEW][ind] = Zmap[MERGVIEW][ind];
#endif
#endif
    ind++; // Move to the next pixel
  }

}

/*
 * \brief
 *    Check if the background is on the right of the hole
 *
 * \input
 * \param xleft, y
 *    The position of the pixel that is on the left of the hole
 * \param xright, y
 *    The position of the pixel that is on the right of the hole
 * \param blksz
 *    Size of the block that is used to check depth value
 * \class member Zmap[MERGVIEW]
 *    The depth image (real depth values stroed) warped to virtual view
 * \class member Dmap[MERGVIEW]
 *    The depth image (depth levels stored) warped to virtual view
 *
 * \return
 *    1: background is on the right of the hole
 *    0: background is on the left of the hole
 */
int CViewInterpolation1D::IsBackgroundOnRight(int xleft, int xright, int y, int blksize)
{
  int i, j;
  int blksizeyMinus = 0;
  int blksizeyPlus  = 0;
  int startx, starty;
  int endx, endy;
  double dleft, dright;
  int dn;

  // Calc the left side
  startx = xleft - blksize + 1;
  starty = y - blksizeyMinus;
  endx = xleft;
  endy = y + blksizeyPlus;
  if (startx < 0)
    startx = 0;
  if (starty < 0)
    starty = 0;
  if (endy >= Height)
    endy = Height - 1;

  dleft = 0;
  dn = 0;

  for (j = starty; j <= endy; j++)
  for (i = startx; i <= endx; i++)
  {
#if USE_ZMAP
    dleft += (double)Zmap[MERGVIEW][i+j*Width2*SubPelOption];
#else
    dleft += (double)Dmap[MERGVIEW][i+j*Width2*SubPelOption];
#endif
    dn++;
  }
  if (dn != 0)
    dleft = dleft / dn;

  // Calc the right side
  startx = xright;
  starty = y - blksizeyMinus;
  endx = xright + blksize - 1;
  endy = y + blksizeyPlus;

  if (endx >= Width2*SubPelOption)
      endx = Width2*SubPelOption - 1;
  if (starty < 0)
      starty = 0;
  if (endy >= Height)
      endy = Height - 1;

  dright = 0;
  dn = 0;
  for (j = starty; j <= endy; j++)
  for (i = startx; i <= endx; i++)
  {
#if USE_ZMAP
    dright += (double) Zmap[MERGVIEW][i+j*Width2*SubPelOption];
#else
    dright += (double) Dmap[MERGVIEW][i+j*Width2*SubPelOption];
#endif
    dn++;
  }
  if (dn != 0)
    dright = dright / dn;

  // when using Dmap as conditions

#if USE_ZMAP
  if (Znear[LEFTVIEW] > 0)
    return (dright >= dleft);
  else
    return (dright <= dleft);
#else
  return (dright <= dleft);
#endif
}
#define WANT_SIMPLE_HOLE_FILLING 1
#define WANT_ZERO_FILL_INPAINTING 0
#if WANT_SIMPLE_HOLE_FILLING
/*Use a simpler hole filling method to eliminate geometric randomness*/
void FillHolesSimple(unsigned char* Yo, unsigned char* Yi, int width, int height, unsigned char *mask)
{
   int x, y, ind;

   memset(Yo, 0, width*height);

   ind = 0;
   for (y = 0; y < height; y++)
   for (x = 0; x < width;  x++)
   {
      if (mask[ind] == HOLEPIXEL)
      {
#if WANT_ZERO_FILL_INPAINTING
         Yo[ind] = 128;
#else
         if((x-1)>=0) /*filling from left takes priority*/
         {
            Yo[ind] = Yo[ind-1];
         }
         else if((y-1)>=0)
         {
            Yo[ind] = Yo[ind-width];
         }
         else
         {
            Yo[ind] = 128;
         }
#endif
      }
      else if(mask[ind] == NOTHOLEPIXEL)
      {
         Yo[ind] = Yi[ind];
      }
      else
      {
         printf("Error unknown mask type\n");
      }
      ind++;
   }
}
#else
/*
 * \brief
 *    Perform hole filling
 *
 * \input
 * \param Yi
 *    The synthesized picture, current have holes. One component.
 * \class member Mask[MERGVIEW]
 *    Show the hole positions
 * \ class member Zmap[MERGVIEW]
 *    the warped depth map.
 *
 * \output
 * \param Yo
 *    The synthesized picture itself is modified due to hole filling
 *
 * \return
 *    none
 */
void CViewInterpolation1D::FillHoles(unsigned char* Yo, unsigned char* Yi)
{
  int pixelValue;
  int x, y, ind, ind2 = 0;
  int xnhole;
  int found;
  int fillDir; // 0: fail to fill; 1: fill from left; 2: fill from right

  memset(Yo, 0, Width2*Height*SubPelOption);

  ind = 0;
  pixelValue = -1;
  for (y = 0; y < Height; y++)
  for (x = 0; x < Width2*SubPelOption;  x++)
  {
    if (Mask[MERGVIEW][ind] == HOLEPIXEL) // This is a starting pixel of a hole
    {
      if (pixelValue < 0) // This is the first pixel of a hole
      {
        found = 0;
        xnhole = x + 1;
        ind2 = ind + 1;
        while (!found && xnhole < Width2*SubPelOption)
        {
          if (Mask[MERGVIEW][ind2] == NOTHOLEPIXEL)
            found = 1;
          else
          { xnhole++; ind2++; }
        }

        // fill from left or from right?
        if (x == 0) // the hole is at leftmost
        {
          if (found)
            fillDir = 2;  // fill from right
          else // the whole line is a hole
            fillDir = 0;  // maybe fill hole from above
        }
        else if (found == 0) // the hole is at rightmost
        {
          fillDir = 1;  // fill from left
        }
        else if (found) // the hole is in the middle
        {
          // fill the hole using the pixel with larger depth, farther away from the camera
          if (IsBackgroundOnRight(x-1, xnhole, y, 1))
            fillDir = 2;
          else
            fillDir = 1;
        }

        // Set the non-hole pixel
        pixelValue = 0;
        if (fillDir == 1) // from the left
          pixelValue = Yi[ind-1];
        else if (fillDir == 2) // from the right
          pixelValue = Yi[ind2];

        Yo[ind] = pixelValue;
      }
      else
      {
        Yo[ind] = pixelValue;
        if (ind >= ind2)
          pixelValue = -1;
      }
     if(x>0)
     {
#if USE_ZMAP
         Zmap[MERGVIEW][ind] = Zmap[MERGVIEW][ind-1];
#endif
#if USE_LDV
         OccZmap[MERGVIEW][ind] = OccZmap[MERGVIEW][ind-1];
         ODmap[MERGVIEW][ind] = ODmap[MERGVIEW][ind-1];
#endif
     }
    }
    else // This is not a hole
    {
      Yo[ind] = Yi[ind];
      pixelValue = -1;
    }

    ind++; // Move to the next pixel
  }
}
#endif
/*
 * \brief
 *    Scale down the synthesized view
 * \param Yo
 *
 * \param Yi
 *
 *
 */
void CViewInterpolation1D::ScaleDownSyn(unsigned char* Yo, unsigned char* Yi)
{
#if 0
  // Using AVC downsampling filter
  int i, j;
  int k, x, idx;
  long int s, t;
#endif

  if (SubPelOption == 1)
  {
    memcpy(Yo, Yi, Width2*Height);
  }
  else if (SubPelOption == 2)
  {
    CPictureResample resmple;
    resmple.DownsampleView(Yo, Yi, Width2*SubPelOption, Height, 2);
  }
  else if (SubPelOption == 4)
  {
    CPictureResample resmple;
    resmple.DownsampleView(Yo, Yi, Width2*SubPelOption, Height, 4);
#if 0
    // Using AVC downsampling filter
    int WidthMinus1 = Width2*SubPelOption-1;
    unsigned char *in, *out;
    int pel[7];

    for (j = 0; j < Height; j++)
    {
      out = &Yo[j*Width2*SubPelOption/4];
      in  = &Yi[j*Width2*SubPelOption];
      for(i = 0; i < Width2*SubPelOption; i+=4)
      {
        pel[0] = CLIP(i-3, 0, WidthMinus1);
        pel[1] = CLIP(i-2, 0, WidthMinus1);
        pel[2] = CLIP(i-1, 0, WidthMinus1);
        pel[3] = CLIP(i  , 0, WidthMinus1);
        pel[4] = CLIP(i+1, 0, WidthMinus1);
        pel[5] = CLIP(i+2, 0, WidthMinus1);
        pel[6] = CLIP(i+3, 0, WidthMinus1);

        out[(i >> 2)] = CLIP ( (16*in[pel[3]] + 9*(in[pel[2]] + in[pel[4]]) - (in[pel[0]] + in[pel[6]])) >> 5, 0, 255);
      }
    }
#endif
  }
  else
    printf("Error: The scale factor is not supported: %d\n", SubPelOption);
}

/*
 * \brief
 *    Perform boundary detection based on the depth image
 *    The org function was replaced in May 2009.
 *    -Zhejiang Univ. May 2009
 *
 * \input
 * \param depth
 *    Depth map
 *
 * \output
 * \param edge
 *    Edge map: 0-NONE, 1-RISE, 2-FALL (left to right), 3-AROUND, 4-SUSPECT
 *    NONE: not boundary
 *    RISE: the top position of a rising edge from left to right
 *    FALL: the top position of a falling edge from left to right
 *    AROUND: boundary growth area around both sides of RISE or FALL edge
 *    SUSPECT: 2-pixel-wide area at left side of RISE edge and right side of FALL edge.
 *
 * \return
 *    none
 */
void CViewInterpolation1D::DetectBoundary( unsigned char* edge, unsigned char* depth, int width, int height, int threshold)
{
   int i,j,k;
   int ind=0;
   int diff=0;
  int edge_growth = BoundaryGrowth;
   unsigned char* mask =(unsigned char*)malloc(width*height);
   memset(mask, 0, width*height);

   for(i=0; i<height; i++)
   {
      k = i*width;
      *(edge+k) = NONE; //0th of each row
      for(j=1; j<width; j++) //x-(x-1)
      {
         ind = k+j;
         diff = *(depth+ind) - *(depth+ind-1);
         if( diff > threshold )
            *(edge+ind) = RISE;
         else if( diff < -threshold)
            *(edge+ind-1) = FALL;
         else
            *(edge+ind) = NONE;
      }
   }

   for(i=0; i<height; i++)
        {
      k = i * width;
      //to make edge at top point and 1-pixel-wide
    for( j=1; j<width-1; j++)
          {
         ind = k +j;
         if( edge[ind]==RISE && edge[ind+1]==NONE && depth[ind+1]-depth[ind]>5 )       mask[ind+1] = 50; // later be changed to RISE
         else if( edge[ind]==FALL && edge[ind-1]==NONE && depth[ind-1]-depth[ind]>5 )  mask[ind-1] = 60; // later be changed to FALL
         else if( edge[ind]==RISE && edge[ind+1]==RISE )                               mask[ind] = 40;
         else if( edge[ind]==FALL && edge[ind-1]==FALL )                               mask[ind] = 40;   // later be changed to NONE
          }

      for(j=0;j<width;j++)
      {
         ind = k+j;
         if( mask[ind] == 50 )
         {  edge[ind] = RISE;  edge[ind-1] = NONE;}
         else if( mask[ind] == 60 )
         {  edge[ind] = FALL;  edge[ind+1] = NONE;}
         else if( mask[ind] == 40)
         {  edge[ind] = NONE;                    }
        }

    //to mark 2-pixel-wide area at left side of RISE edge and at right side of FALL edge
    for(j=2;j<width-2;j++)
    {
      ind = k +j;
      if(edge[ind+1] == RISE || edge[ind+2] == RISE)
        edge[ind] = SUSPECT;
      else if(edge[ind-1] == FALL || edge[ind-2] == FALL)
        edge[ind] = SUSPECT;
      }

    //to mark edge_growth-pixel-wide area around strong depth edge (both sides)
    for(j=edge_growth;j<width-edge_growth;j++)
    {
      ind = k +j;
      //if current pixel is FALL, mask the right-40 pixel as boundary pixel
      if(edge[ind] == FALL || edge[ind] == RISE)
        for(int p = ind - edge_growth; p <= ind + edge_growth; p++)
          if( edge[p] == NONE)
            edge[p] = AROUND;
    }
  }

#if DEBUG_ZHEJIANG
   //debug
   int count1=0;
   int count2=0;
   int sz = width*height;
   ind = 0;
   for(i=0;i<sz; i++)
   {
      if(edge[ind] == RISE) count1++;
      else if(edge[ind] == FALL) count2++;
      ind++;
   }
   fprintf(stderr, "\n Debug Info: Depth edge pixels: RISE = %d; FALL =%d ", count1, count2);
#endif

   free(mask);
}

/*
 * \brief
 *    Check a small block around a specified pixel if there is any boundary pixel
 *
 * \input
 * \param BoundaryArray
 *    A map indicating which pixels are regarded as boundaries. (the mask has the size of Width2*Height)
 * \param x, y
 *    The pixel position
 *
 * \return
 *    true: if there is any boundary pixel around
 *    false: if there is no any boundary pixel around
 */
bool CViewInterpolation1D::IsBoundary(unsigned char* BoundaryArray, int x, int y)
{
   if( BoundaryArray[y*Width2+x] != NONE ) //which means == RISE || FALL || AROUND
    return true;
  else
    return false;
}


/*
 * \brief
 *    Export the hole positions out of the lib
 * \param SynMask
 *    Write the hole mask into this array
 * \return
 *    0
 */
int  CViewInterpolation1D::GetSynMask(unsigned char* SynMask)
{
  int x, y;
  //int i, j;
  //int flag;

  // Mask
  if (SynMask)
  {
    if (SubPelOption == 1 && UpsampleRefs == 1)
    {
      memcpy(SynMask, Mask[MERGVIEW], Width*Height);
    }
    else
    {
      for (y = 0; y < Height; y++)
      for (x = 0; x < Width;  x++)
      {
        // ********
        // Depending how you want to look at the mask, you can change the following code:
        // ********

        //flag = HOLEPIXEL;
        //for (i = - SubPelOption*UpsampleRefs; i <= SubPelOption*UpsampleRefs; i++)
        //{
        //  j = x*SubPelOption*UpsampleRefs + i;
        //  if (j >= 0 && j < Width*SubPelOption*UpsampleRefs)
        //    flag = (flag && Mask[MERGVIEW][j + y*Width*SubPelOption*UpsampleRefs]);
        //}
        //SynMask[x+y*Width] = flag;

        SynMask[x+y*Width] = Mask[MERGVIEW][x*SubPelOption*UpsampleRefs + y*Width*SubPelOption*UpsampleRefs];
      }
    }
  }


  return 0;
}

/*
 * \brief
 *    Get the synthesized depth image. Note that we simply copy the int pixel for output.
 *
 * \param SynDepth
 *    The destination where to write the depth image
 *
 * \return
 *    0
 */
int  CViewInterpolation1D::GetSynDepth(unsigned char* SynDepth)
{
  int i, x, y;

  // Depth
  if (SynDepth)
  {
    if (SubPelOption == 1 && UpsampleRefs == 1)
    {
      for (i = 0; i < Width*Height; i++)
        SynDepth[i] = Dmap[MERGVIEW][i];
    }
    else
    {
      for (y = 0; y < Height; y++)
      for (x = 0; x < Width;  x++)
      {
        // In case of subpel precision, the following code simply pick up the integer pixels.
        SynDepth[x+y*Width] = Dmap[MERGVIEW][x*SubPelOption*UpsampleRefs + y*Width*SubPelOption*UpsampleRefs];
      }
    }
  }

  return 0;
}

#if ENABLE_VSDE
void findMomentsAfterCombining(double ***textureMomentLeft, double ***textureMomentRight, unsigned short width, unsigned short height, PROBABILITY_OUTCOME *left, PROBABILITY_OUTCOME *right, double factorLeft, double factorRight, PIXEL_MOMENTS *momentBuffer, int h, int w, bool debugFlag)
{
#if 1
	int i;
	int j;
	double leftFirst=0;
	double leftSecond=0;
	double rightFirst=0;
	double rightSecond=0;

   for(i=0; i<left->numberOutcomes; i++)
   {
	   if((left->y[i]>=height) || (left->x[i]>=width))
       {
          printf("Error %s index out of bound\n",__FUNCTION__);
          exit(0);
       }
	   leftFirst = leftFirst + textureMomentLeft[0][left->y[i]][left->x[i]]*left->prob[i];
	   leftSecond = leftSecond + textureMomentLeft[1][left->y[i]][left->x[i]]*left->prob[i];
   }

   for(j=0; j<right->numberOutcomes; j++)
   {
	   if((right->y[j]>=height) || (right->x[j]>=width))
       {
          printf("Error %s index out of bound\n",__FUNCTION__);
          exit(0);
       }
	   rightFirst = rightFirst + textureMomentRight[0][right->y[j]][right->x[j]]*right->prob[j];
	   rightSecond = rightSecond + textureMomentRight[1][right->y[j]][right->x[j]]*right->prob[j];
   }

   /*first moment excluding the hole case*/
   momentBuffer->firstMoment[h*momentBuffer->width + w] = factorLeft*(1 - right->noMapProb)*leftFirst + factorRight*(1 - left->noMapProb)*rightFirst;
   momentBuffer->firstMoment[h*momentBuffer->width + w] = momentBuffer->firstMoment[h*momentBuffer->width + w] + right->noMapProb*leftFirst + left->noMapProb*rightFirst;

   /*second moment excluding the hole case*/
   momentBuffer->secondMoment[h*momentBuffer->width + w] = factorLeft*factorLeft*(1-right->noMapProb)*leftSecond + factorRight*factorRight*(1-left->noMapProb)*rightSecond;
   momentBuffer->secondMoment[h*momentBuffer->width + w] = momentBuffer->secondMoment[h*momentBuffer->width + w] + 2*factorLeft*factorRight*leftFirst*rightFirst;
   momentBuffer->secondMoment[h*momentBuffer->width + w] = momentBuffer->secondMoment[h*momentBuffer->width + w] + right->noMapProb*leftSecond + left->noMapProb*rightSecond;

   /*deal with the hole case*/
#if WANT_ZERO_FILL_INPAINTING
   momentBuffer->firstMoment[h*momentBuffer->width + w] = momentBuffer->firstMoment[h*momentBuffer->width + w] + 128*right->noMapProb*left->noMapProb;
   momentBuffer->secondMoment[h*momentBuffer->width + w] = momentBuffer->secondMoment[h*momentBuffer->width + w] + 16384*right->noMapProb*left->noMapProb;
#else
      if((w-1)>=0)/*filling from left takes priority*/
      {
         momentBuffer->firstMoment[h*momentBuffer->width + w] = momentBuffer->firstMoment[h*momentBuffer->width + w] + momentBuffer->firstMoment[h*momentBuffer->width + w-1]*right->noMapProb*left->noMapProb;
         momentBuffer->secondMoment[h*momentBuffer->width + w] = momentBuffer->secondMoment[h*momentBuffer->width + w] + momentBuffer->secondMoment[h*momentBuffer->width + w-1]*right->noMapProb*left->noMapProb;
      }
      else if((h-1)>=0)
      {
         momentBuffer->firstMoment[h*momentBuffer->width + w] = momentBuffer->firstMoment[h*momentBuffer->width + w] + momentBuffer->firstMoment[(h-1)*momentBuffer->width + w]*right->noMapProb*left->noMapProb;
         momentBuffer->secondMoment[h*momentBuffer->width + w] = momentBuffer->secondMoment[h*momentBuffer->width + w] + momentBuffer->secondMoment[(h-1)*momentBuffer->width + w]*right->noMapProb*left->noMapProb;
      }
      else
      {
         momentBuffer->firstMoment[h*momentBuffer->width + w] = momentBuffer->firstMoment[h*momentBuffer->width + w] + 128*right->noMapProb*left->noMapProb;
         momentBuffer->secondMoment[h*momentBuffer->width + w] = momentBuffer->secondMoment[h*momentBuffer->width + w] + 16384*right->noMapProb*left->noMapProb;
      }
#endif
#else /*the slower version*/
   int i;
   int j;

   for(i=0; i<left->numberOutcomes; i++)
   {
      for(j=0; j<right->numberOutcomes; j++)
      {
         double first=0;
         double second=0;

         if((left->y[i]>=height) || (left->x[i]>=width) || (right->y[j]>=height) || (right->x[j]>=width))
         {
            printf("Error %s index out of bound\n",__FUNCTION__);
            exit(0);
         }

         first = factorLeft*textureMomentLeft[0][left->y[i]][left->x[i]] + factorRight*textureMomentRight[0][right->y[j]][right->x[j]];
         second = second + factorLeft*factorLeft*textureMomentLeft[1][left->y[i]][left->x[i]];
         second = second + factorRight*factorRight*textureMomentRight[1][right->y[j]][right->x[j]];
         second = second + 2*factorLeft*factorRight*textureMomentLeft[0][left->y[i]][left->x[i]]*textureMomentRight[0][right->y[j]][right->x[j]];

         momentBuffer->firstMoment[h*momentBuffer->width + w] = momentBuffer->firstMoment[h*momentBuffer->width + w] + first*left->prob[i]*right->prob[j];
         momentBuffer->secondMoment[h*momentBuffer->width + w] = momentBuffer->secondMoment[h*momentBuffer->width + w] + second*left->prob[i]*right->prob[j];
      }
   }

   for(i=0; i<left->numberOutcomes; i++)
   {
      double first=0;
      double second=0;

      if((left->y[i]>=height) || (left->x[i]>=width))
      {
         printf("Error %s index out of bound\n"__FUNCTION__);
         exit(0);
      }

      first = textureMomentLeft[0][left->y[i]][left->x[i]];
      second = textureMomentLeft[1][left->y[i]][left->x[i]];

      momentBuffer->firstMoment[h*momentBuffer->width + w] = momentBuffer->firstMoment[h*momentBuffer->width + w] + first*left->prob[i]*right->noMapProb;
      momentBuffer->secondMoment[h*momentBuffer->width + w] = momentBuffer->secondMoment[h*momentBuffer->width + w] + second*left->prob[i]*right->noMapProb;
   }

   for(j=0; j<right->numberOutcomes; j++)
   {
      double first=0;
      double second=0;

      if((right->y[j]>=height) || (right->x[j]>=width))
      {
         printf("Error %s index out of bound\n", __FUNCTION__);
         exit(0);
      }

      first = textureMomentRight[0][right->y[j]][right->x[j]];
      second = textureMomentRight[1][right->y[j]][right->x[j]];

      momentBuffer->firstMoment[h*momentBuffer->width + w] = momentBuffer->firstMoment[h*momentBuffer->width + w] + first*right->prob[j]*left->noMapProb;
      momentBuffer->secondMoment[h*momentBuffer->width + w] = momentBuffer->secondMoment[h*momentBuffer->width + w] + second*right->prob[j]*left->noMapProb;
   }

   /*take care of the case where it might be a hole*/
   /*We assume the pixels used during the process to be independent*/
#if WANT_ZERO_FILL_INPAINTING
   momentBuffer->firstMoment[h*momentBuffer->width + w] = momentBuffer->firstMoment[h*momentBuffer->width + w] + 128*right->noMapProb*left->noMapProb;
   momentBuffer->secondMoment[h*momentBuffer->width + w] = momentBuffer->secondMoment[h*momentBuffer->width + w] + 16384*right->noMapProb*left->noMapProb;
#else
      if((w-1)>=0)/*filling from left takes priority*/
      {
         momentBuffer->firstMoment[h*momentBuffer->width + w] = momentBuffer->firstMoment[h*momentBuffer->width + w] + momentBuffer->firstMoment[h*momentBuffer->width + w-1]*right->noMapProb*left->noMapProb;
         momentBuffer->secondMoment[h*momentBuffer->width + w] = momentBuffer->secondMoment[h*momentBuffer->width + w] + momentBuffer->secondMoment[h*momentBuffer->width + w-1]*right->noMapProb*left->noMapProb;
      }
      else if((h-1)>=0)
      {
         momentBuffer->firstMoment[h*momentBuffer->width + w] = momentBuffer->firstMoment[h*momentBuffer->width + w] + momentBuffer->firstMoment[(h-1)*momentBuffer->width + w]*right->noMapProb*left->noMapProb;
         momentBuffer->secondMoment[h*momentBuffer->width + w] = momentBuffer->secondMoment[h*momentBuffer->width + w] + momentBuffer->secondMoment[(h-1)*momentBuffer->width + w]*right->noMapProb*left->noMapProb;
      }
      else
      {
         momentBuffer->firstMoment[h*momentBuffer->width + w] = momentBuffer->firstMoment[h*momentBuffer->width + w] + 128*right->noMapProb*left->noMapProb;
         momentBuffer->secondMoment[h*momentBuffer->width + w] = momentBuffer->secondMoment[h*momentBuffer->width + w] + 16384*right->noMapProb*left->noMapProb;
      }
#endif
#endif
}
#endif
/*
 * \brief
 *    Perform view interpolation using two refence views
 *
 * \input
 *    The images are arranged in a one dimension array
 * \param RefLeft
 *    The left ref view image. YUV 444
 * \param RefRight
 *    The right ref view image YUV 444
 * \param RefDepthLeft
 *    The left ref depth view image
 * \param RefDepthRight
 *    The right ref depth view image
 *
 *
 * \output
 * \param Syn
 *    The synthesized view. YUV 420
 * \param SynMask
 *    The mask showing the hole positions
 *
 * \return
 *    0: success
 *    non-zero: fail
 */
#if ENABLE_VSDE
extern double distortionGlobal;
#endif
int CViewInterpolation1D::DoOneFrame(unsigned char* RefLeft, unsigned char* RefRight, unsigned char* RefDepthLeft, unsigned char* RefDepthRight, unsigned char* Syn)
{
  unsigned char* pRefLeft;
  unsigned char* pRefRight;
  unsigned char* pRefDepthLeft;
  unsigned char* pRefDepthRight;
  CPictureResample Resampling;

#if ENABLE_VSDE
  probabilityMapMomentMemoryReset();
#endif

  Width2 = Width*UpsampleRefs;

#if 0
  {
      static int flag = 0;
      FILE *fp;
      if (flag == 0)
        fp = fopen("ref_left_1024x768_16p.yuv", "wb");
      else
        fp = fopen("ref_left_1024x768_16p.yuv", "ab");
      if (fp)
      {
        fwrite(&RefLeft[0],                1, Width*Height,   fp);
        fwrite(&RefLeft[Width*Height],     1, Width*Height,   fp);
        fwrite(&RefLeft[Width*Height*2],   1, Width*Height,   fp);
        fclose(fp);
      }
      if (flag == 0)
        fp = fopen("ref_rght_1024x768_16p.yuv", "wb");
      else
        fp = fopen("ref_rght_1024x768_16p.yuv", "ab");
      if (fp)
      {
        fwrite(&RefRight[0],                1, Width*Height,   fp);
        fwrite(&RefRight[Width*Height],     1, Width*Height,   fp);
        fwrite(&RefRight[Width*Height*2],   1, Width*Height,   fp);
        fclose(fp);
      }
      flag++;
  }
#endif

  //Zhejiang, Temporal Improvement Option
  if(TemporalImprovementOption == 1)
  {
     TemporalImprovementMethod(RefLeft,  RefViewLast[0], RefDepthLeft,  RefDepthLast[0], FrameNumber, Width, Height);
     TemporalImprovementMethod(RefRight, RefViewLast[1], RefDepthRight, RefDepthLast[1], FrameNumber, Width, Height);
  }

  // Upsample ref views, if necessary
  if (UpsampleRefs == 1)
  {
    pRefLeft  = RefView[LEFTVIEW] = RefLeft;
    pRefRight = RefView[RGHTVIEW] = RefRight;
    pRefDepthLeft  = RefDepth[LEFTVIEW] = RefDepthLeft;
    pRefDepthRight = RefDepth[RGHTVIEW] = RefDepthRight;
  }
  else {
    pRefLeft  = RefView[LEFTVIEW];
    pRefRight = RefView[RGHTVIEW];
    pRefDepthLeft  = RefDepth[LEFTVIEW];
    pRefDepthRight = RefDepth[RGHTVIEW];
    Resampling.UpsampleView( pRefLeft,                    RefLeft,                  Width,   Height,   UpsampleRefs);
    Resampling.UpsampleView(&pRefLeft[Width2*Height],    &RefLeft[Width*Height],    Width,   Height,   UpsampleRefs);
    Resampling.UpsampleView(&pRefLeft[Width2*Height*2],  &RefLeft[Width*Height*2],  Width,   Height,   UpsampleRefs);
    Resampling.UpsampleView( pRefRight,                   RefRight,                 Width,   Height,   UpsampleRefs);
    Resampling.UpsampleView(&pRefRight[Width2*Height],   &RefRight[Width*Height],   Width,   Height,   UpsampleRefs);
    Resampling.UpsampleView(&pRefRight[Width2*Height*2], &RefRight[Width*Height*2], Width,   Height,   UpsampleRefs);
    Resampling.UpsampleView( pRefDepthLeft,               RefDepthLeft,             Width,   Height,   UpsampleRefs);
    Resampling.UpsampleView( pRefDepthRight,              RefDepthRight,            Width,   Height,   UpsampleRefs);
  }

  if (Resampling.State() != 0)
  {
    printf("Error happens in the initialization of Resampling()\n");
    return -1;
  }

  // Warp the ref pictures
  ForwardWarp(); // pRefLeft, pRefRight, pRefDepthLeft, pRefDepthRight
#if 0
  {
      static int flag = 0;
      FILE *fp;
      if (flag == 0)
        fp = fopen("syn_left_4096x768_16p.yuv", "wb");
      else
        fp = fopen("syn_left_4096x768_16p.yuv", "ab");
      if (fp)
      {
        fwrite(SynY[LEFTVIEW], 1, Width2*Height*SubPelOption, fp);
        fwrite(SynU[LEFTVIEW], 1, Width2*Height*SubPelOption, fp);
        fwrite(SynV[LEFTVIEW], 1, Width2*Height*SubPelOption, fp);
        fclose(fp);
      }
      if (flag == 0)
        fp = fopen("syn_rght_4096x768_16p.yuv", "wb");
      else
        fp = fopen("syn_rght_4096x768_16p.yuv", "ab");
      if (fp)
      {
        fwrite(SynY[RGHTVIEW], 1, Width2*Height*SubPelOption, fp);
        fwrite(SynU[RGHTVIEW], 1, Width2*Height*SubPelOption, fp);
        fwrite(SynV[RGHTVIEW], 1, Width2*Height*SubPelOption, fp);
        fclose(fp);
      }
      flag++;
  }
#endif
  // Merge the two synthesized view into one
  Merge();

  // Fill holes
#if WANT_SIMPLE_HOLE_FILLING
  FillHolesSimple(SynY[HLFLVIEW], SynY[MERGVIEW], Width2*SubPelOption, Height, Mask[MERGVIEW]);
  FillHolesSimple(SynU[HLFLVIEW], SynU[MERGVIEW], Width2*SubPelOption, Height, Mask[MERGVIEW]);
  FillHolesSimple(SynV[HLFLVIEW], SynV[MERGVIEW], Width2*SubPelOption, Height, Mask[MERGVIEW]);
#else
  FillHoles(SynY[HLFLVIEW], SynY[MERGVIEW]);
  FillHoles(SynU[HLFLVIEW], SynU[MERGVIEW]);
  FillHoles(SynV[HLFLVIEW], SynV[MERGVIEW]);
#endif

#define SAVE_MOMENTS 0
#if ENABLE_VSDE
  {
#if SAVE_MOMENTS
     FILE *left1;
     FILE *left2;
     FILE *right1;
     FILE *right2;
#endif
     double totalDistortion=0;
     unsigned int i;
     unsigned int j;
     unsigned int k;
     double **leftTextureMoments[2]; /*We find the first and second moments for the textures images before calculating the merged moments*/
     double **rightTextureMoments[2];

     leftTextureMoments[0] = (double **)malloc(sizeof(double *)*Height);
     leftTextureMoments[1] = (double **)malloc(sizeof(double *)*Height);
     rightTextureMoments[0] = (double **)malloc(sizeof(double *)*Height);
     rightTextureMoments[1] = (double **)malloc(sizeof(double *)*Height);
     if( leftTextureMoments[0]==NULL || leftTextureMoments[1]==NULL || rightTextureMoments[0]==NULL || rightTextureMoments[1]==NULL)
     {
        printf("Error %s allocating memory",__FUNCTION__);
        exit(0);
     }
     for(i=0; i<Height; i++)
     {
        leftTextureMoments[0][i] = (double *)malloc(sizeof(double)*Width2);
        leftTextureMoments[1][i] = (double *)malloc(sizeof(double)*Width2);
        rightTextureMoments[0][i] = (double *)malloc(sizeof(double)*Width2);
        rightTextureMoments[1][i] = (double *)malloc(sizeof(double)*Width2);
        if( leftTextureMoments[0][i]==NULL || leftTextureMoments[1][i]==NULL || rightTextureMoments[0][i]==NULL || rightTextureMoments[1][i]==NULL)
        {
           printf("Error %s allocating memory",__FUNCTION__);
           exit(0);
        }
     }
#if SAVE_MOMENTS
     left1 = fopen("left1Moments.bin","wb");
     left2 = fopen("left2Moments.bin","wb");
     right1 = fopen("right1Moments.bin","wb");
     right2 = fopen("right2Moments.bin","wb");
     if(!left1 || !left2 || !right1 || !right2)
     {
        printf("Error opening files\n");
        exit(0);
     }
#endif
     /*calculate the texutre image moments*/
     for(i=0; i<Height; i++)
     {
        for(j=0; j<Width2; j++)
        {
           double leftFirst=0;
           double leftSecond=0;
           double rightFirst=0;
           double rightSecond=0;
           int leftDepthCount=0;//debug
           int rightDepthCount=0;//debug
           double depthLeftFirst=0;//debug
           double depthLeftSecond=0;//debug
           double depthRightFirst=0;//debug
           double depthRightSecond=0;//debug
           for(k=0; k<256; k++)
           {
              if(texturePMF[LEFTVIEW][i][j].map[k]!=0)
              {
                 leftFirst = leftFirst + ((double)k)*texturePMF[LEFTVIEW][i][j].prob[k];
                 leftSecond = leftSecond + ((double)(k*k))*texturePMF[LEFTVIEW][i][j].prob[k];
              }
              if(texturePMF[RGHTVIEW][i][j].map[k]!=0)
              {
                 rightFirst = rightFirst + ((double)k)*texturePMF[RGHTVIEW][i][j].prob[k];
                 rightSecond = rightSecond + ((double)(k*k))*texturePMF[RGHTVIEW][i][j].prob[k];
              }
              if(depthPMF[LEFTVIEW][i][j].map[k]!=0)//debug
              {
                 depthLeftFirst = depthLeftFirst + ((double)k)*depthPMF[LEFTVIEW][i][j].prob[k];
                 depthLeftSecond = depthLeftSecond + ((double)(k*k))*depthPMF[LEFTVIEW][i][j].prob[k];
              }
              if(depthPMF[RGHTVIEW][i][j].map[k]!=0)
              {
                 depthRightFirst = depthRightFirst + ((double)k)*depthPMF[RGHTVIEW][i][j].prob[k];
                 depthRightSecond = depthRightSecond + ((double)(k*k))*depthPMF[RGHTVIEW][i][j].prob[k];
              }//debug
           }
           leftTextureMoments[0][i][j] = leftFirst;
           leftTextureMoments[1][i][j] = leftSecond;
           rightTextureMoments[0][i][j] = rightFirst;
           rightTextureMoments[1][i][j] = rightSecond;
#if 0
           for(k=0; k<256; k++)//debug
           {
              if(depthPMF[LEFTVIEW][i][j].map[k]!=0)
              {
                 leftDepthCount++;
              }
              if(depthPMF[RGHTVIEW][i][j].map[k]!=0)
              {
                 rightDepthCount++;
              }
           }//debug
           if(leftDepthCount!=1)//debug
           {
              printf("Randomness in left depth image? count:%d at %d %d\n",leftDepthCount,i,j);
              exit(0);
           }
           if(rightDepthCount!=1)
           {
              printf("Randomness in left depth image? count:%d at %d %d\n",rightDepthCount,i,j);
              exit(0);
           }//debug
#endif
#if SAVE_MOMENTS
           fwrite(&depthLeftFirst, sizeof(double), 1, left1);
           fwrite(&depthLeftSecond, sizeof(double), 1, left2);
           fwrite(&depthRightFirst, sizeof(double), 1, right1);
           fwrite(&depthRightSecond, sizeof(double), 1, right2);
#endif
        }
     }
#if SAVE_MOMENTS
     fclose(left1);
     fclose(left2);
     fclose(right1);
     fclose(right2);
#endif
     for(i=0; i<Height; i++)
     {
        for(j=0; j<Width2; j++)
        {
           double pixelDistortion;
           double actualData = (double)SynY[HLFLVIEW][i*Width2 + j];

           PROBABILITY_MAP *textureMapLeft = &m_textureProbMap[0];
           PROBABILITY_MAP *textureMapRight = &m_textureProbMap[1];
#if 0
if((textureMapLeft->outcomes[i][j].numberOutcomes>1) || (textureMapRight->outcomes[i][j].numberOutcomes>1))//debug
{
   printf("number of outcome %d %d at %d %d\n",textureMapLeft->outcomes[i][j].numberOutcomes,textureMapRight->outcomes[i][j].numberOutcomes,i,j);
   printf("left nomap:%f\n",textureMapLeft->outcomes[i][j].noMapProb);
   for(k=0; k<textureMapLeft->outcomes[i][j].numberOutcomes; k++)
   {
      printf("%f\n",textureMapLeft->outcomes[i][j].prob[k]);
   }
   printf("right nomap:%f\n",textureMapRight->outcomes[i][j].noMapProb);
   for(k=0; k<textureMapRight->outcomes[i][j].numberOutcomes; k++)
   {
      printf("%f\n",textureMapRight->outcomes[i][j].prob[k]);
   }
   exit(0);
}//debug
#endif
           /*find the moments*/
           findMomentsAfterCombining(leftTextureMoments, rightTextureMoments, (unsigned short)Width2, (unsigned short)Height, &textureMapLeft->outcomes[i][j], &textureMapRight->outcomes[i][j], WeightLeft, WeightRight, &m_mergedPixelMoments, i, j, false);

           /*find pixel distortion*/
           pixelDistortion = actualData*actualData;
           pixelDistortion = pixelDistortion + m_mergedPixelMoments.secondMoment[i*m_mergedPixelMoments.width + j];
           pixelDistortion = pixelDistortion - 2*actualData*m_mergedPixelMoments.firstMoment[i*m_mergedPixelMoments.width + j];
#if 0
           if( pixelDistortion!=0 )
           {
              /*note that we do not really account for rounding at the moment so the distortion could be non-zero especially when there are no errors*/
              //printf("WARNING Pixel distortion %f at %d %d\n",pixelDistortion,i,j);
              printf("actual:%f firstMoment:%f at %d %d\n",actualData,m_mergedPixelMoments.firstMoment[i*m_mergedPixelMoments.width + j], i, j);
              printf("LeftViewPixel points to %d %d holeProb:%f rightViewPixel points to %d %d holeProb:%f \n",textureMapLeft->outcomes[i][j].y[0], textureMapLeft->outcomes[i][j].x[0], textureMapLeft->outcomes[i][j].noMapProb, textureMapRight->outcomes[i][j].y[0], textureMapRight->outcomes[i][j].x[0], textureMapRight->outcomes[i][j].noMapProb);
              printf("left moments:%f %f\n",leftTextureMoments[0][textureMapLeft->outcomes[i][j].y[0]][textureMapLeft->outcomes[i][j].x[0]], leftTextureMoments[1][textureMapLeft->outcomes[i][j].y[0]][textureMapLeft->outcomes[i][j].x[0]]);
              printf("right moments:%f %f\n",rightTextureMoments[0][textureMapRight->outcomes[i][j].y[0]][textureMapRight->outcomes[i][j].x[0]], rightTextureMoments[1][textureMapRight->outcomes[i][j].y[0]][textureMapRight->outcomes[i][j].x[0]]);
              //pixelDistortion = 0;
           }
#endif
           totalDistortion = totalDistortion + pixelDistortion;
        }

     }
     printf(" \n***:Dis:%f:***\n ", totalDistortion);
     distortionGlobal = totalDistortion;/*put this in the global variable so that we can write it out*/

     for(i=0; i<Height; i++)
     {
        free(leftTextureMoments[0][i]);
        free(leftTextureMoments[1][i]);
        free(rightTextureMoments[0][i]);
        free(rightTextureMoments[1][i]);
     }
     free(leftTextureMoments[0]);
     free(leftTextureMoments[1]);
     free(rightTextureMoments[0]);
     free(rightTextureMoments[1]);
  }
#endif


  // Prepare the output
  if (Syn)
  {
    // Scale down the output
    ScaleDownSyn(SynY[FINLVIEW], SynY[HLFLVIEW]);
    ScaleDownSyn(SynU[FINLVIEW], SynU[HLFLVIEW]);
    ScaleDownSyn(SynV[FINLVIEW], SynV[HLFLVIEW]);

    if (UpsampleRefs != 1)
    {
#if 0
  {
      static int flag = 0;
      FILE *fp;
      if (flag == 0)
        fp = fopen("syn_4096x768_16p.yuv", "wb");
      else
        fp = fopen("syn_4096x768_16p.yuv", "ab");
      if (fp)
      {
        fwrite(SynY[FINLVIEW], 1, 4096*768, fp);
        fwrite(SynU[FINLVIEW], 1, 4096*768, fp);
        fwrite(SynV[FINLVIEW], 1, 4096*768, fp);
        fclose(fp);
      }
      flag++;
  }
#endif

      Resampling.DownsampleView(SynY[FINLVIEW], SynY[FINLVIEW], Width2, Height, UpsampleRefs);
      Resampling.DownsampleView(SynU[FINLVIEW], SynU[FINLVIEW], Width2, Height, UpsampleRefs);
      Resampling.DownsampleView(SynV[FINLVIEW], SynV[FINLVIEW], Width2, Height, UpsampleRefs);
    }
#if 0
  {
      static int flag = 0;
      FILE *fp;
      char filen[256];
      char tmp = 128;
      int i;
      sprintf(filen, "syn_org_resolution_%dx%d_16p.yuv", Width, Height);
      if (flag == 0)
        fp = fopen(filen, "wb");
      else
        fp = fopen(filen, "ab");
      if (fp)
      {
        fwrite(SynY[FINLVIEW], 1, Width*Height, fp);
        //fwrite(SynU[FINLVIEW], 1, Width*Height, fp);
        //fwrite(SynV[FINLVIEW], 1, Width*Height, fp);
        for (i=0; i<Width*Height/2; i++)
          fwrite(&tmp, 1, 1, fp);
        fclose(fp);
      }
      flag++;
  }
#endif

    // Y
    memcpy(Syn, SynY[FINLVIEW], Width*Height);
    // Downsample the UV
    Resampling.PictureResample444to420(&Syn[Width*Height],     SynU[FINLVIEW], Width/2, Height/2);
    Resampling.PictureResample444to420(&Syn[Width*Height*5/4], SynV[FINLVIEW], Width/2, Height/2);
#if 0
  {
      static int flag = 0;
      FILE *fp;
      char filen[256];
      char tmp = 128;
      int i;
      sprintf(filen, "syn_org_resolution_%dx%d_16p.yuv", Width, Height);
      if (flag == 0)
        fp = fopen(filen, "wb");
      else
        fp = fopen(filen, "ab");
      if (fp)
      {
        fwrite(SynY[FINLVIEW], 1, Width*Height, fp);
        //fwrite(SynU[FINLVIEW], 1, Width*Height, fp);
        //fwrite(SynV[FINLVIEW], 1, Width*Height, fp);
        for (i=0; i<Width*Height/2; i++)
          fwrite(&tmp, 1, 1, fp);
        fclose(fp);
      }
      flag++;
  }
#endif
  }


  //printf("<<<<<<<<<<<<\n");
  return 0;
}


//Zhejiang
//FindDepthMaxMin -- search less point to get the approximate Zmax and Zmin
void CViewInterpolation1D::FindDepthMaxMin( unsigned char* map, int ViewID)
{
   int i, j, k, ind;
   double z;
   Maxdk = Mindk = 0;
   Maxz = 0;
   Minz = 255;

   for(i=0; i<Height; i=i+16)
   {
      k = i * Width;
      for(j=0; j<Width; j=j+16)
      {
        ind = k + j;
         if( map[ind] > Maxz)    Maxz = map[ind];
         else if( map[ind] < Minz)   Minz = map[ind];
      }
   }

   z = (double)1.0 / ( (Maxz/255.0) * (1.0/Znear[ViewID] - 1.0/Zfar[ViewID]) + (1.0/Zfar[ViewID]) );
   Maxdk = (int) ( FocalLength * LTranslation[ViewID] / z - abs(duPrincipal[ViewID]) + 1); //ceiling
   z = (double)1.0 / ( (Minz/255.0) * (1.0/Znear[ViewID] - 1.0/Zfar[ViewID]) + (1.0/Zfar[ViewID]) );
   Mindk = (int) ( FocalLength * LTranslation[ViewID] / z - abs(duPrincipal[ViewID]) + 0); //flooring

   //UpsampleRefs, SubPelOption
   Maxdk = Maxdk * UpsampleRefs;
   Mindk = Mindk * UpsampleRefs;

  SplattingDepthThreshold = (int) ( (Maxz-Minz) *0.6 + Minz );
}

//2009.05.04, improvement, Zhejiang University(a little different from m16041: Temporal Improvement Method in View Synthesis)
//-if a block is moving, then its neigbouring 4*4 blocks are also dertermined to be moving,
//--since the object boundary on depth map is not restrict to depth boundary.
void CViewInterpolation1D::TemporalImprovementMethod( unsigned char *ImageCur, unsigned char *ImageLast, unsigned char *DepthCur, unsigned char *DepthLast,
                                                           int frame, int Width, int Height )
{
   int SAD_TH = 100;     //threshold of sad in 4*4 block motion detection
   double weight = 0.75;  //weight value for depth map weighting equation


   //internal variables
   int diff = 0;
   int ind = 0;
   int s = 0;
   unsigned char *flag=NULL;
   int t = 0;

   flag = (unsigned char*)malloc(Width/4);

   if( 0 == frame ) // first frame need not the weighting, but need to prepare the data
   {
      memcpy(ImageLast, ImageCur, Width*Height);
      memcpy(DepthLast, DepthCur, Width*Height);
   }
   else
   {
     for ( int y = 0; y < Height/4; y++)
      {
         //motion detection by SSE -- do a line
         for ( int x = 0; x < Width/4;  x++)
         {
            ind = y * Width * 4 + x * 4;
            s = 0;
            for( int j = 0; j < 4; j++)
               for(int i = 0; i < 4; i++)
               {
                  diff=( ImageCur[ind+j*Width+i] - ImageLast[ind+j*Width+i]);
                  s +=abs(diff);  //SAD
               }
            if(s<SAD_TH) flag[x]=0;
            else         flag[x]=1;
         }
      //temporal weighting according to motion detection result -- do a line
         for ( int x = 0; x < Width/4;  x++)
         {
            ind = y * Width * 4 + x * 4;

#if TEMPOBLOCK3
            //3 block
            if(x == 0)              t = flag[x] + flag[x+1];
            else if( x == Width/4 -1)   t = flag[x-1] + flag[x];
            else                 t = flag[x-1] + flag[x] + flag[x+1];
#else
            //5 block
            if(x == 0)              t = flag[x]   + flag[x+1] + flag[x+2];
            else if( x == 1)        t = flag[x-1] + flag[x]   + flag[x+1] + flag[x+2];
            else if( x == Width/4-2)    t = flag[x-2] + flag[x-1] + flag[x]   + flag[x+1];
            else if( x == Width/4-1)    t = flag[x-2] + flag[x-1] + flag[x];
            else                 t = flag[x-2] + flag[x-1] + flag[x]   + flag[x+1] + flag[x+2];
#endif

            if( t == 0 )// if not moving
               for( int j = 0; j < 4; j++)
                  for( int i = 0; i < 4; i++)
                     DepthCur[ind+j*Width+i] = (unsigned char)( DepthLast[ind+j*Width+i] * weight + DepthCur[ind+j*Width+i] * ( 1 - weight ) );
            //else, do nothing
         }
      }

      memcpy(ImageLast, ImageCur, Width*Height);
      memcpy(DepthLast, DepthCur, Width*Height);
   }
}
