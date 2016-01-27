/*!
 ***********************************************************************
 *  \mainpage
 *     This is the H.264/AVC encoder reference software. For detailed documentation
 *     see the comments in each file.
 *
 *     The JM software web site is located at:
 *     http://iphome.hhi.de/suehring/tml
 *
 *     For bug reporting and known issues see:
 *     https://ipbt.hhi.fraunhofer.de
 *
 *  \author
 *     The main contributors are listed in contributors.h
 *
 *  \version
 *     JM 18.0 (FRExt)
 *
 *  \note
 *     tags are used for document system "doxygen"
 *     available at http://www.doxygen.org
 */
/*!
 *  \file
 *     lencod.c
 *  \brief
 *     H.264/AVC reference encoder project main()
 *  \author
 *   Main contributors (see contributors.h for copyright, address and affiliation details)
 *   - Inge Lille-Langoy               <inge.lille-langoy@telenor.com>
 *   - Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *   - Stephan Wenger                  <stewe@cs.tu-berlin.de>
 *   - Jani Lainema                    <jani.lainema@nokia.com>
 *   - Byeong-Moon Jeon                <jeonbm@lge.com>
 *   - Yoon-Seong Soh                  <yunsung@lge.com>
 *   - Thomas Stockhammer              <stockhammer@ei.tum.de>
 *   - Detlev Marpe
 *   - Guido Heising
 *   - Valeri George
 *   - Karsten Suehring
 *   - Alexis Michael Tourapis         <alexismt@ieee.org>
 *   - Athanasios Leontaris            <aleon@dolby.com>
 ***********************************************************************
 */

#include "contributors.h"

#include <time.h>
#include <math.h>

#include "win32.h"
#include "global.h"
#include "cconv_yuv2rgb.h"
#include "configfile.h"
#include "conformance.h"
#include "context_ini.h"
#include "explicit_gop.h"
#include "explicit_seq.h"
#include "filehandle.h"
#include "image.h"
#include "input.h"
#include "img_io.h"
#include "slice.h"
#include "intrarefresh.h"
#include "leaky_bucket.h"
#include "mc_prediction.h"
#include "memalloc.h"
#include "me_epzs_common.h"
#include "me_epzs_int.h"
#include "me_umhex.h"
#include "me_umhexsmp.h"
#include "output.h"
#include "parset.h"
#include "q_matrix.h"
#include "q_offsets.h"
#include "ratectl.h"
#include "report.h"
#include "rdoq.h"
#include "errdo.h"
#include "rdopt.h"
#include "wp_mcprec.h"
#include "mv_search.h"
#include "img_process.h"
#include "q_offsets.h"
#include "pred_struct.h"
#include "blk_prediction.h"
#include "img_luma.h"
#include "img_chroma.h"
#include "mc_prediction.h"
#include "me_distortion.h"
#include "md_distortion.h"
#include "mode_decision.h"
#include "transform8x8.h"
#include "intra16x16.h"
#include "intra4x4.h"
#include "intra8x8.h"
//#include "resize.h"
#include "md_common.h"
#include "macroblock.h"

#include "wp.h"

#if WANT_ROPE_PMF /*for shared memory*/
#include <windows.h>
#include <stdio.h>
#include <strsafe.h>
#endif

//check the scaling factor to avoid overflow;
#if !IMGTYPE
#if JCOST_CALC_SCALEUP && (LAMBDA_ACCURACY_BITS>8)
#error "LAMBDA_ACCURACY_BITS is greater than 8. Overflow!"
#endif
#endif
static const int mb_width_cr[4] = {0, 8, 8,16};
static const int mb_height_cr[4]= {0, 8,16,16};

EncoderParams   *p_Enc = NULL;

static void SetLevelIndices     (VideoParameters *p_Vid);
static void chroma_mc_setup     (VideoParameters *p_Vid);
static void init_img            (VideoParameters *p_Vid);
static void init_encoder        (VideoParameters *p_Vid, InputParameters *p_Inp);
static int  init_global_buffers (VideoParameters *p_Vid, InputParameters *p_Inp);
static void free_global_buffers (VideoParameters *p_Vid, InputParameters *p_Inp);
static void free_img            (VideoParameters *p_Vid, InputParameters *p_Inp);
static void free_params         (InputParameters *p_Inp);

static void encode_sequence     (VideoParameters *p_Vid, InputParameters *p_Inp, char mailBoxNameIn[STR_SIZE], char mailBoxNameOut[STR_SIZE]);


static void generate_encode_parameters(VideoParameters *p_Vid);
static void free_encode_parameters(VideoParameters *p_Vid);


void init_stats (InputParameters *p_Inp, StatParameters *p_Stats)
{
  memset(p_Stats, 0, sizeof(StatParameters));
  p_Stats->NumberBFrames = p_Inp->NumberBFrames;
}

void init_dstats (DistortionParams *p_Dist)
{
  p_Dist->frame_ctr = 0;
  memset(p_Dist->metric, 0, TOTAL_DIST_TYPES * sizeof(DistMetric));
}

/*!
 ***********************************************************************
 * \brief
 *    Allocate the Video Parameters structure
 * \par  Output:
 *    Video Parameters VideoParameters *p_Vid
 ***********************************************************************
 */
static void alloc_video_params( VideoParameters **p_Vid)
{
  if ((*p_Vid = (VideoParameters *) calloc(1, sizeof(VideoParameters)))==NULL)
    no_mem_exit("alloc_video_params: p_Vid");
  if ((((*p_Vid)->p_Dist)  = (DistortionParams *) calloc(1, sizeof(DistortionParams)))==NULL)
    no_mem_exit("alloc_video_params: p_Dist");
  if ((((*p_Vid)->p_Stats) = (StatParameters *) calloc(1, sizeof(StatParameters)))==NULL)
    no_mem_exit("alloc_video_params: p_Stats");
  if (((*p_Vid)->p_Dpb_layer[0]     = (DecodedPictureBuffer *) calloc(MAX_NUM_DPB_LAYERS, sizeof(DecodedPictureBuffer)))==NULL)
    no_mem_exit("alloc_video_params: p_Dpb_layer");
  {
    int i;
    for(i=1; i<MAX_NUM_DPB_LAYERS; i++)
    {
      (*p_Vid)->p_Dpb_layer[i] = (*p_Vid)->p_Dpb_layer[i-1] + 1;
      (*p_Vid)->p_Dpb_layer[i]->layer_id = i;
    }
  }
  if ((((*p_Vid)->p_Quant)  = (QuantParameters *) calloc(1, sizeof(QuantParameters)))==NULL)
    no_mem_exit("alloc_video_params: p_Quant");
  if ((((*p_Vid)->p_QScale)  = (ScaleParameters *) calloc(1, sizeof(ScaleParameters)))==NULL)
    no_mem_exit("alloc_video_params: p_QScale");
  if ((((*p_Vid)->p_SEI)  = (SEIParameters *) calloc(1, sizeof(SEIParameters)))==NULL)
    no_mem_exit("alloc_video_params: p_SEI");


  (*p_Vid)->p_dec = -1;
#if (MVC_EXTENSION_ENABLE)
  (*p_Vid)->p_dec2 = -1;
#endif
  (*p_Vid)->p_log = NULL;
  (*p_Vid)->f_annexb = NULL;
  // Init rtp related info
  (*p_Vid)->f_rtp = NULL;
  (*p_Vid)->CurrentRTPTimestamp = 0;
  (*p_Vid)->CurrentRTPSequenceNumber = 0;
}

/*!
 ***********************************************************************
 * \brief
 *    Allocate the Input structure
 * \par  Output:
 *    Input Parameters InputParameters *p_Vid
 ***********************************************************************
 */
static void alloc_params( InputParameters **p_Inp )
{
  if ((*p_Inp = (InputParameters *) calloc(1, sizeof(InputParameters)))==NULL)
    no_mem_exit("alloc_params: p_Inp");

  (*p_Inp)->top_left          = NULL;
  (*p_Inp)->bottom_right      = NULL;
  (*p_Inp)->slice_group_id    = NULL;
  (*p_Inp)->run_length_minus1 = NULL;
}


  /*!
 ***********************************************************************
 * \brief
 *    Allocate the Encoder Structure
 * \par  Output:
 *    Encoder Parameters
 ***********************************************************************
 */
static void alloc_encoder( EncoderParams **p_Enc)
{
  if ((*p_Enc = (EncoderParams *) calloc(1, sizeof(EncoderParams)))==NULL)
    no_mem_exit("alloc_encoder: p_Enc");

  alloc_video_params(&((*p_Enc)->p_Vid));
  alloc_params(&((*p_Enc)->p_Inp));
  (*p_Enc)->p_trace = NULL;
  (*p_Enc)->bufferSize = 0;
}

/*!
 ***********************************************************************
 * \brief
 *    Free the Encoder Structure
 ***********************************************************************
 */
static void free_encoder (EncoderParams *p_Enc)
{
  free_pointer( p_Enc );
}

#if WANT_ROPE_PMF
/*We allocate and free the working buffer on a per frame basis to save memory during run time*/
void ROPEAllocWorkingMem(VideoParameters *p_Vid)
{
   int i;

   /*allocate the consecutive memories first*/
   p_Vid->pixelDistribution1DWorking = (PixelPMF *)malloc(sizeof(PixelPMF)*(p_Vid->height+2*IMG_PAD_SIZE_Y)*(p_Vid->width+2*IMG_PAD_SIZE_X));
   if(p_Vid->pixelDistribution1DWorking == NULL)
   {
      printf("Error %s line %d allocating memory\n",__FUNCTION__,__LINE__);
      exit(0);
   }
   memset(p_Vid->pixelDistribution1DWorking, 0, sizeof(PixelPMF)*(p_Vid->height+2*IMG_PAD_SIZE_Y)*(p_Vid->width+2*IMG_PAD_SIZE_X));

   /*assign the 2d array*/
   for(i=-IMG_PAD_SIZE_Y; i<p_Vid->height+IMG_PAD_SIZE_Y; i++)
   {
      p_Vid->pixelDistributionWorking[i] = &p_Vid->pixelDistribution1DWorking[(i+IMG_PAD_SIZE_Y)*(p_Vid->width+2*IMG_PAD_SIZE_X)+IMG_PAD_SIZE_X];
      p_Vid->pixelDistributionWorking[i] = &p_Vid->pixelDistribution1DWorking[(i+IMG_PAD_SIZE_Y)*(p_Vid->width+2*IMG_PAD_SIZE_X)+IMG_PAD_SIZE_X];
   }
}
void ROPEFreeWorkingMem(VideoParameters *p_Vid)
{
   if(p_Vid->pixelDistribution1DWorking)
   {
      free(p_Vid->pixelDistribution1DWorking);
      p_Vid->pixelDistribution1DWorking = NULL;
   }
}
/*we create the shared memory buffer once*/
void ROPEAlloc(VideoParameters *p_Vid, float lossRate, TCHAR sharedMemName[256])
{
   int i;
   PixelPMF **temppp;

   printf("Alloc ROPE memory width:%d height:%d shared memory %s\n", p_Vid->width, p_Vid->height, sharedMemName);

   /*create shared memory*/
   p_Vid->hMapFile = CreateFileMapping(
                 INVALID_HANDLE_VALUE,    // use paging file
                 NULL,                    // default security
                 PAGE_READWRITE,          // read/write access
                 0,                       // maximum object size (high-order DWORD)
                 sizeof(PixelPMF)*(p_Vid->height+2*IMG_PAD_SIZE_Y)*(p_Vid->width+2*IMG_PAD_SIZE_X),                // maximum object size (low-order DWORD)
                 sharedMemName);                 // name of mapping object
   if (p_Vid->hMapFile == NULL)
   {
      printf("Error creating file mapping line %d error %d\n",__LINE__,GetLastError());
      exit(0);
   }
   p_Vid->pixelDistribution1DShared = (PixelPMF *) MapViewOfFile(p_Vid->hMapFile,   // handle to map object
                        FILE_MAP_ALL_ACCESS, // read/write permission
                        0,
                        0,
                        sizeof(PixelPMF)*(p_Vid->height+2*IMG_PAD_SIZE_Y)*(p_Vid->width+2*IMG_PAD_SIZE_X));
   if (p_Vid->pixelDistribution1DShared == NULL)
   {
      printf("Error creating file view line %d error %d\n",__LINE__,GetLastError());
      CloseHandle(p_Vid->hMapFile);
      exit(0);
   }
   memset(p_Vid->pixelDistribution1DShared, 0, sizeof(PixelPMF)*(p_Vid->height+2*IMG_PAD_SIZE_Y)*(p_Vid->width+2*IMG_PAD_SIZE_X));

   /*allocate the 2d array for working*/
   temppp = (PixelPMF **)malloc(sizeof(PixelPMF *)*(p_Vid->height+2*IMG_PAD_SIZE_Y));
   if(temppp == NULL)
   {
      printf("Error %s line %d allocating memory\n",__FUNCTION__,__LINE__);
      exit(0);
   }
   p_Vid->pixelDistributionWorking = &temppp[IMG_PAD_SIZE_Y];

   /*allocate the 2d array for sharing*/
   temppp = (PixelPMF **)malloc(sizeof(PixelPMF *)*(p_Vid->height+2*IMG_PAD_SIZE_Y));
   if(temppp == NULL)
   {
      printf("Error %s line %d allocating memory\n",__FUNCTION__,__LINE__);
      exit(0);
   }
   p_Vid->pixelDistributionShared = &temppp[IMG_PAD_SIZE_Y];

   /*assign the 2d array for the shared memory*/
   for(i=-IMG_PAD_SIZE_Y; i<p_Vid->height+IMG_PAD_SIZE_Y; i++)
   {
      p_Vid->pixelDistributionShared[i] = &p_Vid->pixelDistribution1DShared[(i+IMG_PAD_SIZE_Y)*(p_Vid->width+2*IMG_PAD_SIZE_X)+IMG_PAD_SIZE_X];
      p_Vid->pixelDistributionShared[i] = &p_Vid->pixelDistribution1DShared[(i+IMG_PAD_SIZE_Y)*(p_Vid->width+2*IMG_PAD_SIZE_X)+IMG_PAD_SIZE_X];
   }

   /*set the packet loss rate*/
   p_Vid->lossRate = lossRate;

   /*allocate memory for prediction and residue signal*/
   p_Vid->frameResidue = (int **)malloc(sizeof(int *)*p_Vid->height);
   p_Vid->framePrediction = (imgpel **)malloc(sizeof(imgpel *)*p_Vid->height);
   if((p_Vid->frameResidue==NULL) || (p_Vid->framePrediction==NULL))
   {
      printf("Error %s allocation memory(residue or prediction)\n",__FUNCTION__);
   }
   for(i=0; i<p_Vid->height; i++)
   {
      p_Vid->frameResidue[i] = (int *)malloc(sizeof(int)*p_Vid->width);
      p_Vid->framePrediction[i] = (imgpel *)malloc(sizeof(imgpel)*p_Vid->width);
      if((p_Vid->frameResidue[i]==NULL) || (p_Vid->framePrediction[i]==NULL))
      {
         printf("Error %s allocation memory(residue or prediction)\n",__FUNCTION__);
      }
   }
   printf("ROPE memory allocated\n");
}
void ROPEFree(VideoParameters *p_Vid)
{
   int i;

   /*free the 2d array for working*/
   if(p_Vid->pixelDistributionWorking)
   {
      free(&p_Vid->pixelDistributionWorking[-IMG_PAD_SIZE_Y]);
      p_Vid->pixelDistributionWorking = NULL;
   }

   /*free the 2d array for sharing*/
   if(p_Vid->pixelDistributionShared)
   {
      free(&p_Vid->pixelDistributionShared[-IMG_PAD_SIZE_Y]);
      p_Vid->pixelDistributionShared = NULL;
   }

   /*unmap the shared memory*/
   UnmapViewOfFile(p_Vid->pixelDistribution1DShared);
   CloseHandle(p_Vid->hMapFile);

   /*free residue and prediction signal*/
   for(i=0; i<p_Vid->height; i++)
   {
      free(p_Vid->frameResidue[i]);
      free(p_Vid->framePrediction[i]);
   }
   free(p_Vid->frameResidue);
   free(p_Vid->framePrediction);
   printf("ROPE memory freed\n");
}
/*use this only when packet loss rate is set to 0. What we are trying to do here is to
check the first moment against the encoder reconstruction when packet loss rate is set to 0*/
void ROPEverifyFrameFirstMomentNoLoss(VideoParameters *p_Vid)//debug
{
   int i;
   int j;
   int k;
   double firstMoment;
   unsigned char firstMomentUC;
   imgpel **encRecon;

   if(p_Vid->lossRate!=0)
   {
      printf("Error %s can't proceed with checking loss rate is %f\n", __FUNCTION__, p_Vid->lossRate);
      exit(0);
   }

   encRecon = p_Vid->enc_picture->p_curr_img; /*reconstructed image y*/
   for(i=0; i<p_Vid->height; i++)
   {
      for(j=0; j<p_Vid->width; j++)
      {
         PixelPMF *pixelPMF = &p_Vid->pixelDistributionWorking[i][j];

         unsigned char temp = (unsigned char)encRecon[i][j];

         firstMoment = 0;
         for(k=0; k<256; k++)
         {
            if(pixelPMF->map[k]!=0)
            {
               firstMoment = firstMoment + (double)pixelPMF->prob[k]*(double)k;
            }
         }
         firstMomentUC = (unsigned char)firstMoment;

         if(temp!=firstMomentUC)
         {
            printf("ERROR first moment and encoder recon mismatch %d %d at %d %d frame %d\n", temp, firstMomentUC, i, j, p_Vid->frame_no);
            exit(0);
         }
      }
   }
}
void ROPEverifyFramePMF(VideoParameters *p_Vid)//debug
{
   int i;
   int j;
   int k;
   double totalProb;


   for(i=-IMG_PAD_SIZE_Y; i<p_Vid->height+IMG_PAD_SIZE_Y; i++)
   {
      for(j=-IMG_PAD_SIZE_X; j<p_Vid->width+IMG_PAD_SIZE_X; j++)
      {
         PixelPMF *pixelPMF = &p_Vid->pixelDistributionWorking[i][j];
         totalProb=0;
         for(k=0; k<256; k++)
         {
            if(pixelPMF->map[k]!=0)
            {
               totalProb = totalProb + (double)pixelPMF->prob[k];
            }
         }
         if(fabs(totalProb-(double)1)>=0.00001)
         {
            printf("Error pixel PMF does not sum to 1:%f at(%d %d) frame %d\n",totalProb, i, j, p_Vid->frame_no);
            exit(0);
         }
      }
   }
}
#endif
/*!
 ***********************************************************************
 * \brief
 *    Main function for encoder.
 * \param argc
 *    number of command line arguments
 * \param argv
 *    command line arguments
 * \return
 *    exit code
 ***********************************************************************
 */
#if SAVE_ENCODER_LUMA_RECON
FILE *encLumaReconFile;
#endif
#if SAVE_MANUAL_LUMA_RECON
FILE *manLumaReconFile;
#endif
#if WANT_ROPE_PMF
void createSharedMemoryNames(TCHAR output[STR_SIZE], char *input)
{
   int i=0;
   while(1)
   {
      if(input[i] == 0)
      {
         break;
      }
      output[i] = input[i];
      i++;
   }
   output[i] = 0;
}
#endif
#if WANT_ROPE_PMF
void charToTCHAR(TCHAR output[STR_SIZE], char input[STR_SIZE])
{
   int i=0;
   while(1)
   {
      if(input[i] == 0)
      {
         break;
      }
      output[i] = input[i];
      i++;
   }

   output[i] = 0;
}

void msgSend(HANDLE hSlot, int *msg, int msgSize)
{
   BOOL fResult;
   DWORD cbWritten;

   fResult = WriteFile(hSlot,
     msg,
     (DWORD) msgSize,
     &cbWritten,
     (LPOVERLAPPED) NULL);

   if (!fResult)
   {
      printf("WriteFile failed with %d.\n", GetLastError());
      exit(0);
   }
}

int msgRead(HANDLE hSlot)
{
   TCHAR eventNameTCHAR[STR_SIZE];
   char eventName[STR_SIZE];

    DWORD cbMessage, cMessage, cbRead;
    BOOL fResult;
    int receivedMsg;

    HANDLE hEvent;
    OVERLAPPED ov;

    cbMessage = cMessage = cbRead = 0;

   sprintf(eventName,"%d_event",GetCurrentProcessId());
   charToTCHAR(eventNameTCHAR, eventName);
    hEvent = CreateEvent(NULL, FALSE, FALSE, eventNameTCHAR);
    if( NULL == hEvent )
        return FALSE;
    ov.Offset = 0;
    ov.OffsetHigh = 0;
    ov.hEvent = hEvent;

    fResult = GetMailslotInfo(hSlot, // mailslot handle
        (LPDWORD) NULL,               // no maximum message size
        &cbMessage,                   // size of next message
        &cMessage,                    // number of messages
        (LPDWORD) NULL);              // no read time-out
    if (!fResult)
    {
        printf("GetMailslotInfo failed with %d.\n", GetLastError());
        exit(0);
    }

    if (cbMessage == MAILSLOT_NO_MESSAGE)
    {
        /*no message in the mail box*/
        return MSG_NO_MESSAGE;
    }

   /*with the way we set up the mail box, there should only be 1 message waiting*/
    if(cMessage!=1)
   {
      printf("ERROR messaging system is not synchronized\n");
      exit(0);
   }
    if(cbMessage!=4)
   {
      printf("ERROR message size is incorrect\n");
      exit(0);
   }

    /*retrieve the message*/
    fResult = ReadFile(hSlot,
        &receivedMsg,
        cbMessage,
        &cbRead,
        &ov);
    if (!fResult)
    {
        printf("ReadFile failed with %d.\n", GetLastError());
        exit(0);
    }

    CloseHandle(hEvent);

    return receivedMsg;
}
#endif
int main(int argc, char **argv)
{
#if WANT_ROPE_PMF
  TCHAR temp[STR_SIZE];
  /*argument format: shared memory names, incoming mail box, outgoing mail box*/
  TCHAR sharedMemName[STR_SIZE];
  char mailBoxNameIn[STR_SIZE];
  char mailBoxNameOut[STR_SIZE];
  double lossRate;

  lossRate = atof(argv[argc-4]);
  createSharedMemoryNames(sharedMemName, argv[argc-3]);
  sprintf(mailBoxNameIn,"%s",argv[argc-2]);
  sprintf(mailBoxNameOut,"%s",argv[argc-1]);
  argc=argc-4;
#endif
  init_time();
#if MEMORY_DEBUG
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

  alloc_encoder(&p_Enc);

  Configure (p_Enc->p_Vid, p_Enc->p_Inp, argc, argv);

  // init encoder
  init_encoder(p_Enc->p_Vid, p_Enc->p_Inp);
#if SAVE_ENCODER_LUMA_RECON
  encLumaReconFile = fopen("encLumaReconFile.yuv","wb");
#endif
#if SAVE_MANUAL_LUMA_RECON
  manLumaReconFile = fopen("manLumaReconFile.yuv","wb");
#endif
#if WANT_ROPE_PMF/*allocate memory used by ROPE*/
  ROPEAlloc(p_Enc->p_Vid, lossRate, sharedMemName);
#endif
#if WANT_ROPE_PMF
  /*create the mail boxes for communication*/
  /*outgoing mail boxes should have been created by the control process now create the outgoing mail boxes*/
  charToTCHAR(temp, mailBoxNameOut);
  //printf("PID:%d outgoing mail box name %s\n",GetCurrentProcessId(), mailBoxNameOut);//debug
  p_Enc->p_Vid->mailHandleOut = CreateFile(temp,
       GENERIC_WRITE,
       FILE_SHARE_READ,
       (LPSECURITY_ATTRIBUTES) NULL,
       OPEN_EXISTING,
       FILE_ATTRIBUTE_NORMAL,
       (HANDLE) NULL);

  if(p_Enc->p_Vid->mailHandleOut == INVALID_HANDLE_VALUE)
  {
     printf("CreateFile failed with %d.\n", GetLastError());
     exit(0);
  }

  /*create the incoming mail boxes so the control process can issue command to us*/
  charToTCHAR(temp, mailBoxNameIn);
  //printf("PID:%d incoming mail box name %s\n",GetCurrentProcessId(), mailBoxNameIn);//debug
  p_Enc->p_Vid->mailHandleIn = CreateMailslot(temp,
         0,                             // no maximum message size
         MAILSLOT_WAIT_FOREVER,         // no time-out for operations
         (LPSECURITY_ATTRIBUTES) NULL); // default security

  if(p_Enc->p_Vid->mailHandleIn == INVALID_HANDLE_VALUE)
  {
     printf("CreateMailslot failed with %d\n", GetLastError());
     exit(0);
  }
#endif
  // encode sequence
#if WANT_ROPE_PMF
  encode_sequence(p_Enc->p_Vid, p_Enc->p_Inp, mailBoxNameIn, mailBoxNameOut);
#else
  encode_sequence(p_Enc->p_Vid, p_Enc->p_Inp);
#endif
/*signal the end of the sequence*/
#if WANT_ROPE_PMF
    while(1)
    {
       int incomingMessage;
       incomingMessage = msgRead(p_Enc->p_Vid->mailHandleIn);
       if(incomingMessage==MSG_START_FRAME)
       {
          //printf("PID:%d Got message will send sequence finish message\n",GetCurrentProcessId());//debug
          break;
       }
       else if(incomingMessage==MSG_NO_MESSAGE)
       {
       }
       else
       {
          printf("PID:%d Received unexpected message %d", GetCurrentProcessId(), incomingMessage);
          exit(0);
       }
       Sleep(200);
    }
    {
       int outgoingMessage=MSG_SEQUENCE_FINISHED;
       msgSend(p_Enc->p_Vid->mailHandleOut, &outgoingMessage, sizeof(int));
       //printf("PID:%d Sequence finish message sent\n",GetCurrentProcessId());//debug
    }
#endif
#if WANT_ROPE_PMF/*free memory used by ROPE*/
  ROPEFree(p_Enc->p_Vid);
#endif
#if WANT_ROPE_PMF
  /*close the mail boxes*/
  CloseHandle(p_Enc->p_Vid->mailHandleIn);
  CloseHandle(p_Enc->p_Vid->mailHandleOut);
#endif
#if SAVE_ENCODER_LUMA_RECON
  fclose(encLumaReconFile);
#endif
#if SAVE_MANUAL_LUMA_RECON
  fclose(manLumaReconFile);
#endif
  // terminate sequence
  free_encoder_memory(p_Enc->p_Vid, p_Enc->p_Inp);

  free_params (p_Enc->p_Inp);
  free_encoder(p_Enc);

  return 0;
}


/*!
 ************************************************************************
 * \brief
 *    calculate Ceil(Log2(uiVal))
 ************************************************************************
 */
static unsigned CeilLog2( unsigned uiVal)
{
  unsigned uiTmp = uiVal-1;
  unsigned uiRet = 0;

  while( uiTmp != 0 )
  {
    uiTmp >>= 1;
    uiRet++;
  }
  return uiRet;
}

void SetDpbLayerId(VideoParameters *p_Vid, int idx)
{
  assert(idx>=0 && idx<2);
  p_Vid->dpb_layer_id = idx;
}

static void set_storage_format(VideoParameters *p_Vid, FrameFormat *p_src, FrameFormat *p_dst)
{
  InputParameters *p_Inp = p_Vid->p_Inp;
  *p_dst = *p_src;
  p_dst->width[0] = (p_Inp->output.width[0]  + p_Vid->auto_crop_right);
  p_dst->height[0] = (p_Inp->output.height[0] + p_Vid->auto_crop_bottom);
  p_dst->width[1] = p_dst->width[2] = p_dst->width[0]  * mb_width_cr [p_dst->yuv_format] / 16;
  p_dst->height[1]= p_dst->height[2] = p_dst->height[0] * mb_height_cr[p_dst->yuv_format] / 16;
  p_dst->size_cmp[0] = p_dst->width[0] * p_dst->height[0];
  p_dst->size_cmp[2] = p_dst->size_cmp[1] = p_dst->width[1] * p_dst->height[1];
  p_dst->size        = p_dst->size_cmp[0] + p_dst->size_cmp[1] + p_dst->size_cmp[2];
}

/*!
 ***********************************************************************
 * \brief
 *    Initialize encoder
 ***********************************************************************
 */
static void init_encoder(VideoParameters *p_Vid, InputParameters *p_Inp)
{
  p_Vid->p_Inp = p_Inp;
  p_Vid->giRDOpt_B8OnlyFlag = FALSE;
  p_Vid->p_log = NULL;

  //set coding layer number;
  p_Vid->num_of_layers = p_Inp->num_of_views;

  p_Vid->cabac_encoding = 0;
  p_Vid->frame_statistic_start = 1;

  if (p_Inp->Log2MaxFNumMinus4 == -1)
  {
    p_Vid->log2_max_frame_num_minus4 = iClip3(0,12, (int) (CeilLog2(p_Inp->no_frames) - 4)); // hack for now...
  }
  else
    p_Vid->log2_max_frame_num_minus4 = p_Inp->Log2MaxFNumMinus4;

  if (p_Vid->log2_max_frame_num_minus4 == 0 && p_Inp->num_ref_frames == 16)
  {
    snprintf(errortext, ET_SIZE, " NumberReferenceFrames=%d and Log2MaxFNumMinus4=%d may lead to an invalid value of frame_num.", p_Inp->num_ref_frames, p_Inp-> Log2MaxFNumMinus4);
    error (errortext, 500);
  }

  // set proper p_Vid->log2_max_pic_order_cnt_lsb_minus4.
  if (p_Inp->Log2MaxPOCLsbMinus4 == - 1)
    p_Vid->log2_max_pic_order_cnt_lsb_minus4 = iClip3(0,12, (int) (CeilLog2( imax( p_Inp->no_frames, (p_Inp->NumberBFrames + 1) << 1 ) << 1 ) - 4)); // hack for now
  else
    p_Vid->log2_max_pic_order_cnt_lsb_minus4 = p_Inp->Log2MaxPOCLsbMinus4;

  if (((1<<(p_Vid->log2_max_pic_order_cnt_lsb_minus4 + 3)) < p_Inp->jumpd * 4) && p_Inp->Log2MaxPOCLsbMinus4 != -1)
    error("log2_max_pic_order_cnt_lsb_minus4 might not be sufficient for encoding. Increase value.",400);

  if (strlen (p_Inp->ReconFile) > 0 && (p_Vid->p_dec = open(p_Inp->ReconFile, OPENFLAGS_WRITE, OPEN_PERMISSIONS))==-1)
  {
    snprintf(errortext, ET_SIZE, "Error open file %s", p_Inp->ReconFile);
    error (errortext, 500);
  }

#if (MVC_EXTENSION_ENABLE)
  if(p_Vid->num_of_layers == 2)
  {
    if (strlen (p_Inp->ReconFile2) > 0 && (p_Vid->p_dec2 = open(p_Inp->ReconFile2, OPENFLAGS_WRITE, OPEN_PERMISSIONS))==-1)
    {
      snprintf(errortext, ET_SIZE, "Error open file %s", p_Inp->ReconFile2);
      error (errortext, 500);
    }
  }
#endif
  if ((p_Inp->output.width[0] & 0x0F) != 0)
  {
    p_Vid->auto_crop_right = 16 - (p_Inp->output.width[0] & 0x0F);
  }
  else
  {
    p_Vid->auto_crop_right = 0;
  }

  if (p_Inp->PicInterlace || p_Inp->MbInterlace)
  {
    if ((p_Inp->output.height[0] & 0x01) != 0)
    {
      error ("even number of lines required for interlaced coding", 500);
    }

    if ((p_Inp->output.height[0] & 0x1F) != 0)
    {
      p_Vid->auto_crop_bottom = 32 - (p_Inp->output.height[0] & 0x1F);
    }
    else
    {
      p_Vid->auto_crop_bottom=0;
    }
  }
  else
  {
    if ((p_Inp->output.height[0] & 0x0F) != 0)
    {
      p_Vid->auto_crop_bottom = 16 - (p_Inp->output.height[0] & 0x0F);
    }
    else
    {
      p_Vid->auto_crop_bottom = 0;
    }
  }
  if (p_Vid->auto_crop_bottom || p_Vid->auto_crop_right)
  {
    fprintf (stderr, "Warning: Automatic cropping activated: Coded frame Size: %dx%d\n",
      p_Inp->output.width[0] + p_Vid->auto_crop_right, p_Inp->output.height[0] + p_Vid->auto_crop_bottom);
  }

  // read the slice group configuration file. Only for types 0, 2 or 6
  if ( 0 != p_Inp->num_slice_groups_minus1 )
  {
    read_slice_group_info(p_Vid, p_Inp);
  }
  //! the number of slice groups is forced to be 1 for slice group type 3-5
  if(p_Inp->num_slice_groups_minus1 > 0)
  {
    if( (p_Inp->slice_group_map_type >= 3) && (p_Inp->slice_group_map_type<=5) )
      p_Inp->num_slice_groups_minus1 = 1;
  }

  if(p_Inp->RCEnable)
  {
    if (p_Inp->basicunit == 0)
      p_Inp->basicunit = (p_Inp->output.height[0] + p_Vid->auto_crop_bottom)*(p_Inp->output.width[0] + p_Vid->auto_crop_right)/256;

    if ( ((p_Inp->output.height[0] + p_Vid->auto_crop_bottom)*(p_Inp->output.width[0] + p_Vid->auto_crop_right)/256) % p_Inp->basicunit != 0)
    {
      snprintf(errortext, ET_SIZE, "Frame size in macroblocks must be a multiple of BasicUnit.");
      error (errortext, 500);
    }
  }

  LevelCheck(p_Vid, p_Inp);

  // Open Files
  OpenFiles(&p_Inp->input_file1);
#if (MVC_EXTENSION_ENABLE)
  if(p_Vid->num_of_layers==2)
  {
    OpenFiles(&p_Inp->input_file2);
  }
  p_Vid->prev_view_is_anchor = 0;
  p_Vid->view_id = 0;  // initialise view_id
  SetDpbLayerId(p_Vid, p_Vid->view_id);
#endif

  //init dpb storage formage;
  p_Vid->output_format[0] = p_Inp->output;
  set_storage_format(p_Vid, &(p_Inp->source), &(p_Vid->p_Dpb_layer[0]->storage_format));
  if(p_Vid->num_of_layers == 2)
  {
    {
      p_Vid->output_format[1] = p_Inp->output;
      set_storage_format(p_Vid, &(p_Inp->source), &(p_Vid->p_Dpb_layer[1]->storage_format));
    }
  }
  //end;

  Init_QMatrix(p_Vid, p_Inp);
  Init_QOffsetMatrix(p_Vid);

  init_poc(p_Vid);
  GenerateParameterSets(p_Vid);
  generate_encode_parameters(p_Vid);
  SetLevelIndices(p_Vid);

  init_img  (p_Vid);

  if (p_Inp->rdopt == 3)
  {
    init_error_conceal(p_Vid,p_Inp->ErrorConcealment);
    //Zhifeng 090611
    init_distortion_estimation(p_Vid,p_Inp->de);
  }

#ifdef _LEAKYBUCKET_
  p_Vid->initial_Bframes = 0;
#if (MVC_EXTENSION_ENABLE)
  if(p_Vid->num_of_layers == 2)
  {
    p_Vid->Bit_Buffer = (long *)malloc(((p_Inp->no_frames << 1) + 1) * sizeof(long));
  }
  else
    p_Vid->Bit_Buffer = (long *)malloc((p_Inp->no_frames + 1) * sizeof(long));
#else
  p_Vid->Bit_Buffer = (long *)malloc((p_Inp->no_frames + 1) * sizeof(long));
#endif
  p_Vid->total_frame_buffer = 0;
#endif

  // Prepare hierarchical coding structures.
  // Code could be extended in the future to allow structure adaptation.
  if (p_Inp->NumberBFrames && p_Inp->HierarchicalCoding == 3)
  {
    init_gop_structure(p_Vid, p_Inp);
    interpret_gop_structure(p_Vid, p_Inp);
  }
  {
    int i;
    for(i=0; i<MAX_NUM_DPB_LAYERS; i++)
    {
      p_Vid->p_Dpb_layer[i]->init_done = 0;
    }
    for(i=0; i<imin(p_Vid->num_of_layers, MAX_NUM_DPB_LAYERS); i++)
    {
      init_dpb(p_Vid, p_Vid->p_Dpb_layer[i]);
    }
  }
  init_out_buffer(p_Vid);
  init_stats (p_Inp, p_Vid->p_Stats);
  init_dstats(p_Vid->p_Dist);

#if (MVC_EXTENSION_ENABLE)
  if(p_Vid->num_of_layers == 2)
  {
    p_Vid->p_Dist->frame_ctr_v[0] = 0;
    p_Vid->p_Dist->frame_ctr_v[1] = 0;
    memset(p_Vid->p_Dist->metric_v[0], 0, TOTAL_DIST_TYPES * sizeof(DistMetric));
    memset(p_Vid->p_Dist->metric_v[1], 0, TOTAL_DIST_TYPES * sizeof(DistMetric));
  }
#endif

  p_Vid->enc_picture = NULL;

  init_global_buffers(p_Vid, p_Inp);


  if ( p_Inp->WPMCPrecision )
  {
    wpxInitWPXPasses(p_Vid, p_Inp);
  }

  Init_Motion_Search_Module (p_Vid, p_Inp);
  information_init(p_Vid, p_Inp, p_Vid->p_Stats);

  if(p_Inp->DistortionYUVtoRGB)
    init_YUVtoRGB(p_Vid, p_Inp);

  //Rate control
  if (p_Inp->RCEnable)
    rc_init_sequence(p_Vid, p_Inp);

  p_Vid->last_valid_reference = 0;
  p_Vid->tot_time = 0;                 // time for total encoding session
  p_Vid->last_bit_ctr_n = 0;

  p_Vid->initial_Bframes = p_Inp->NumberBFrames;

  p_Vid->type = I_SLICE;
  // Write sequence header (with parameter sets)
  p_Vid->p_Stats->bit_ctr_filler_data = 0;
  p_Vid->p_Stats->bit_ctr_filler_data_n = 0;
  p_Vid->p_Stats->bit_ctr_parametersets = 0;

#if (MVC_EXTENSION_ENABLE)
  if ( p_Vid->num_of_layers == 2 )
  {
    p_Vid->p_Stats->bit_ctr_filler_data_v[0] = 0;
    p_Vid->p_Stats->bit_ctr_filler_data_v[1] = 0;
    p_Vid->p_Stats->bit_ctr_filler_data_n_v[0] = 0;
    p_Vid->p_Stats->bit_ctr_filler_data_n_v[1] = 0;
    p_Vid->p_Stats->bit_ctr_parametersets_v[0] = 0;
    p_Vid->p_Stats->bit_ctr_parametersets_v[1] = 0;
  }
#endif

  p_Vid->p_Stats->bit_slice = start_sequence(p_Vid, p_Inp);

  if (p_Inp->UseRDOQuant)
    precalculate_unary_exp_golomb_level(p_Vid);

  if (p_Inp->ExplicitSeqCoding)
    OpenExplicitSeqFile(p_Vid, p_Inp);

  if ( p_Inp->ChromaMCBuffer )
    p_Vid->OneComponentChromaPrediction4x4 = OneComponentChromaPrediction4x4_retrieve;
  else
    p_Vid->OneComponentChromaPrediction4x4 = OneComponentChromaPrediction4x4_regenerate;

  p_Vid->searchRange.min_x = -p_Inp->search_range[0] << 2;
  p_Vid->searchRange.max_x =  p_Inp->search_range[0] << 2;
  p_Vid->searchRange.min_y = -p_Inp->search_range[0] << 2;
  p_Vid->searchRange.max_y =  p_Inp->search_range[0] << 2;
}

void setup_coding_layer(VideoParameters *p_Vid)
{
  //setup the buffers;
  int i;
  int dpb_layer_id = p_Vid->dpb_layer_id;
  InputParameters *p_Inp = p_Vid->p_Inp;

  p_Vid->active_sps = p_Vid->sps[dpb_layer_id];
  p_Vid->p_CurrEncodePar = p_Vid->p_EncodePar[dpb_layer_id];
  //set bitdepth info;
  if(p_Vid->num_of_layers >1)
  {
//all these initializations will be removed later;
    CodingParameters *cps = p_Vid->p_CurrEncodePar;

    p_Vid->yuv_format  = cps->yuv_format;
    p_Vid->P444_joined = cps->P444_joined;
    p_Vid->bitdepth_luma            = cps->bitdepth_luma;
    p_Vid->bitdepth_scale[0]        = cps->bitdepth_scale[0];
    p_Vid->bitdepth_lambda_scale    = cps->bitdepth_lambda_scale;
    p_Vid->bitdepth_luma_qp_scale   = cps->bitdepth_luma_qp_scale;
    p_Vid->dc_pred_value_comp[0]    =  cps->dc_pred_value_comp[0];

    p_Vid->max_pel_value_comp[0] = cps->max_pel_value_comp[0];
    p_Vid->max_imgpel_value_comp_sq[0] = cps->max_imgpel_value_comp_sq[0];
    p_Vid->mb_size[0][0]            = p_Vid->mb_size[0][1] = cps->mb_size[0][0];

    if (p_Vid->yuv_format != YUV400)
    {
      p_Vid->chroma_qp_offset[0] = p_Vid->active_pps->cb_qp_index_offset;
      p_Vid->chroma_qp_offset[1] = p_Vid->active_pps->cr_qp_index_offset;
    }
    else
    {
      p_Vid->chroma_qp_offset[0] = 0;
      p_Vid->chroma_qp_offset[1] = 0;
    }
    p_Vid->bitdepth_chroma             = cps->bitdepth_chroma;
    p_Vid->bitdepth_scale[1]           = cps->bitdepth_scale[1];
    p_Vid->dc_pred_value_comp[2]       = p_Vid->dc_pred_value_comp[1]       = cps->dc_pred_value_comp[1];
    p_Vid->max_pel_value_comp[2]       = p_Vid->max_pel_value_comp[1]       = cps->max_pel_value_comp[1];
    p_Vid->max_imgpel_value_comp_sq[1] = cps->max_imgpel_value_comp_sq[1];
    p_Vid->max_imgpel_value_comp_sq[2] = cps->max_imgpel_value_comp_sq[2];
    p_Vid->num_blk8x8_uv               = cps->num_blk8x8_uv;
    p_Vid->num_cdc_coeff               = cps->num_cdc_coeff;
    p_Vid->mb_size[1][0] = p_Vid->mb_size[2][0] = p_Vid->mb_cr_size_x = cps->mb_cr_size_x;
    p_Vid->mb_size[1][1] = p_Vid->mb_size[2][1] = p_Vid->mb_cr_size_y = cps->mb_cr_size_y;
    p_Vid->bitdepth_chroma_qp_scale = cps->bitdepth_chroma_qp_scale;
    p_Vid->max_bitCount =  cps->max_bitCount;

    ////if ( p_Inp->ChromaMCBuffer )
      //chroma_mc_setup(p_Vid);
    p_Vid->pad_size_uv_x = cps->pad_size_uv_x;
    p_Vid->pad_size_uv_y = cps->pad_size_uv_y;
    p_Vid->chroma_mask_mv_y = cps->chroma_mask_mv_y;
    p_Vid->chroma_mask_mv_x = cps->chroma_mask_mv_x;
    p_Vid->chroma_shift_y = cps->chroma_shift_y;
    p_Vid->chroma_shift_x = cps->chroma_shift_x;
    p_Vid->shift_cr_y  = cps->shift_cr_y;
    p_Vid->shift_cr_x  = cps->shift_cr_x;

    p_Vid->padded_size_x       = cps->padded_size_x;
    p_Vid->padded_size_x_m8x8  = cps->padded_size_x_m8x8;
    p_Vid->padded_size_x_m4x4  = cps->padded_size_x_m4x4;
    p_Vid->cr_padded_size_x    = cps->cr_padded_size_x;
    p_Vid->cr_padded_size_x2   = cps->cr_padded_size_x2;
    p_Vid->cr_padded_size_x4   = cps->cr_padded_size_x4;
    p_Vid->cr_padded_size_x_m8 = cps->cr_padded_size_x_m8;
//end;

    p_Vid->dc_pred_value            = p_Vid->dc_pred_value_comp[0]; // set defaults
    p_Vid->max_imgpel_value         = (short) p_Vid->max_pel_value_comp[0];

    p_Vid->lambda = p_Vid->lambda_buf[dpb_layer_id];
    p_Vid->lambda_md = p_Vid->lambda_md_buf[dpb_layer_id];
    p_Vid->lambda_me = p_Vid->lambda_me_buf[dpb_layer_id];
    p_Vid->lambda_mf = p_Vid->lambda_mf_buf[dpb_layer_id];
    if ( p_Inp->UseRDOQuant )
    {
      p_Vid->lambda_rdoq = p_Vid->lambda_rdoq_buf[dpb_layer_id];
    }
    if (p_Inp->CtxAdptLagrangeMult == 1)
    {
      p_Vid->lambda_mf_factor = p_Vid->lambda_mf_factor_buf[dpb_layer_id];
    }

    setup_mbs(p_Vid->mb_data, p_Vid->FrameSizeInMbs, dpb_layer_id);
    p_Vid->nz_coeff = p_Vid->nz_coeff_buf[dpb_layer_id];
  }

  if(p_Vid->imgY_com_buf[0])
  {
    p_Vid->imgY_com = p_Vid->imgY_com_buf[dpb_layer_id];
    p_Vid->imgUV_com[0] = p_Vid->imgUV_com_buf[dpb_layer_id][0];
    p_Vid->imgUV_com[1] = p_Vid->imgUV_com_buf[dpb_layer_id][1];
  }
  if(p_Vid->imgY_tmp_buf[0])
  {
    p_Vid->imgY_tmp = p_Vid->imgY_tmp_buf[dpb_layer_id];
    p_Vid->imgUV_tmp[0] = p_Vid->imgUV_tmp_buf[dpb_layer_id][0];
    p_Vid->imgUV_tmp[1] = p_Vid->imgUV_tmp_buf[dpb_layer_id][1];
  }
  if ( p_Inp->ChromaMCBuffer )
    p_Vid->OneComponentChromaPrediction4x4 = OneComponentChromaPrediction4x4_retrieve;
  else
    p_Vid->OneComponentChromaPrediction4x4 = OneComponentChromaPrediction4x4_regenerate;

  select_distortion(p_Vid, p_Inp);
  // Setup Distortion Metrics depending on refinement level
  for (i=0; i<3; i++)
  {
    switch(p_Inp->MEErrorMetric[i])
    {
    case ERROR_SAD:
      p_Vid->computeUniPred[i] = computeSAD;
      p_Vid->computeUniPred[i + 3] = computeSADWP;
      p_Vid->computeBiPred1[i] = computeBiPredSAD1;
      p_Vid->computeBiPred2[i] = computeBiPredSAD2;
      break;
    case ERROR_SSE:
      p_Vid->computeUniPred[i] = computeSSE;
      p_Vid->computeUniPred[i + 3] = computeSSEWP;
      p_Vid->computeBiPred1[i] = computeBiPredSSE1;
      p_Vid->computeBiPred2[i] = computeBiPredSSE2;
      break;
    case ERROR_SATD :
    default:
      p_Vid->computeUniPred[i] = computeSATD;
      p_Vid->computeUniPred[i + 3] = computeSATDWP;
      p_Vid->computeBiPred1[i] = computeBiPredSATD1;
      p_Vid->computeBiPred2[i] = computeBiPredSATD2;
      break;
    }
  }

}

/*!
************************************************************************
* \brief
*    Prepare parameters for the current frame
************************************************************************
*/
static void prepare_frame_params(VideoParameters *p_Vid, InputParameters *p_Inp, int curr_frame_to_code)
{
  SeqStructure *p_seq_struct = p_Vid->p_pred;
  FrameUnitStruct *p_cur_frm = p_Vid->p_curr_frm_struct;

  setup_coding_layer(p_Vid);

  if ( p_Inp->ExplicitSeqCoding )
  {
    ExpFrameInfo *info = &p_Vid->expSeq->info[curr_frame_to_code % p_Vid->expSeq->no_frames];
    ReadExplicitSeqFile(p_Vid->expSeq, p_Vid->expSFile, curr_frame_to_code);
    // override and overwrite whatever was in p_cur_frm
    populate_frame_explicit( info, p_Inp, p_cur_frm, p_seq_struct->max_num_slices );
  }

  // populate coding parameters for the current frame (frame_no, slice_type, nal_ref_idc, poc, reset frame_num)
  p_Vid->frame_no = p_cur_frm->frame_no;
  p_Vid->frm_no_in_file = (1 + p_Inp->frame_skip) * p_Vid->frame_no;
  if ( p_cur_frm->type == SI_SLICE )
  {
    set_slice_type( p_Vid, p_Inp, SP_SLICE );
  }
  else
  {
    set_slice_type( p_Vid, p_Inp, p_cur_frm->type ); // dominant slice type; perhaps it is better to use the type of the first slice?
  }
  p_Vid->nal_reference_idc = p_cur_frm->nal_ref_idc;
  switch( p_Vid->pic_order_cnt_type )
  {
  case 0:
    get_poc_type_zero( p_Vid, p_Inp, p_cur_frm );
    break;
  case 1:
    get_poc_type_one( p_Vid, p_Inp, p_cur_frm );
    break;
  default:
  case 2:
    get_poc_type_zero( p_Vid, p_Inp, p_cur_frm );
    break;
  }
  p_Vid->frame_num = ( p_cur_frm->idr_flag == 1 ) ? 0 : p_Vid->frame_num;

  //Rate control
  if (p_Inp->RCEnable && p_Vid->type == I_SLICE)
    rc_init_gop_params(p_Vid, p_Inp);

  // which layer does the image belong to?
  p_Vid->layer = ((p_Vid->curr_frm_idx - p_Vid->last_idr_code_order) % (p_Inp->NumFramesInELSubSeq + 1)) ? 0 : 1;
}

#if WANT_ROPE_PMF
#define SAVE_MOMENTS 0
void ROPEcalculateFrameDistortion(VideoParameters *p_Vid, InputParameters *p_Inp)
{
   int i;
   int j;
   int k;
   double firstMoment;
   double secondMoment;
   double frameDistortion=0;
   double pixelDistortion=0;
#if SAVE_MOMENTS
   FILE *tempFile1;
   FILE *tempFile2;
   char tempFileName[256];
   sprintf(tempFileName,"%s.1.bin",p_Inp->ReconFile);
   tempFile1 = fopen(tempFileName,"wb");
   sprintf(tempFileName,"%s.2.bin",p_Inp->ReconFile);
   tempFile2 = fopen(tempFileName,"wb");
   if(!tempFile1 || !tempFile2)
   {
	   printf("Error opening files\n");
	   exit(0);
   }
#endif
   for(i=0; i<p_Vid->height; i++)
   {
      for(j=0; j<p_Vid->width; j++)
      {
         PixelPMF *pixelPMF = &p_Vid->pixelDistributionWorking[i][j];

         firstMoment = 0;
         secondMoment = 0;
         for(k=0; k<256; k++)
         {
            if(pixelPMF->map[k]!=0)
            {
               firstMoment = firstMoment + (double)pixelPMF->prob[k]*(double)k;
               secondMoment = secondMoment + (double)pixelPMF->prob[k]*((double)(k*k));
            }
         }
#if SAVE_MOMENTS
		 fwrite(&firstMoment, sizeof(double), 1, tempFile1);
		 fwrite(&secondMoment, sizeof(double), 1, tempFile2);
#endif
		 pixelDistortion = secondMoment + (double)(p_Vid->pImgOrg[0][i][j]*p_Vid->pImgOrg[0][i][j]) - 2*((double)p_Vid->pImgOrg[0][i][j])*firstMoment;
         frameDistortion = frameDistortion + pixelDistortion;
      }
   }
   printf("Frame %d distortion:%f ",p_Vid->frame_no, frameDistortion);
#if SAVE_MOMENTS
   fclose(tempFile1);
   fclose(tempFile2);
#endif
}

void ROPEprocessPMFPaddedRegion(VideoParameters *p_Vid)
{
   int i;
   int j;
   PixelPMF *dst;
   PixelPMF *src;

   /*Top band*/
    /*first row, left padded region*/
   for (i = -IMG_PAD_SIZE_X; i < 0; i++)
   {
      dst = &p_Vid->pixelDistributionWorking[-IMG_PAD_SIZE_Y][i];
      src = &p_Vid->pixelDistributionWorking[0][0];

      memcpy(dst->map, src->map, sizeof(unsigned char)*256);
      memcpy(dst->prob, src->prob, sizeof(float)*256);
   }
   /*first row, middle region*/
   for (i = 0; i < p_Vid->width; i++)
   {
      dst = &p_Vid->pixelDistributionWorking[-IMG_PAD_SIZE_Y][i];
      src = &p_Vid->pixelDistributionWorking[0][i];

      memcpy(dst->map, src->map, sizeof(unsigned char)*256);
      memcpy(dst->prob, src->prob, sizeof(float)*256);
   }
   /*first row, right padded region*/
   for (i = p_Vid->width; i < p_Vid->width+IMG_PAD_SIZE_X; i++)
   {
      dst = &p_Vid->pixelDistributionWorking[-IMG_PAD_SIZE_Y][i];
      src = &p_Vid->pixelDistributionWorking[0][p_Vid->width-1];

      memcpy(dst->map, src->map, sizeof(unsigned char)*256);
      memcpy(dst->prob, src->prob, sizeof(float)*256);
   }
   /*replicate the top band*/
   for(i=-IMG_PAD_SIZE_Y+1; i<0; i++)
   {
      for(j=-IMG_PAD_SIZE_X; j<p_Vid->width+IMG_PAD_SIZE_X; j++)
      {
         dst = &p_Vid->pixelDistributionWorking[i][j];
         src = &p_Vid->pixelDistributionWorking[-IMG_PAD_SIZE_Y][j];

         memcpy(dst->map, src->map, sizeof(unsigned char)*256);
         memcpy(dst->prob, src->prob, sizeof(float)*256);
      }
   }

    /*Left band*/
   for(i=0; i<p_Vid->height; i++)
   {
      for(j=-IMG_PAD_SIZE_X; j<0; j++)
      {
         dst = &p_Vid->pixelDistributionWorking[i][j];
         src = &p_Vid->pixelDistributionWorking[i][0];

         memcpy(dst->map, src->map, sizeof(unsigned char)*256);
         memcpy(dst->prob, src->prob, sizeof(float)*256);
      }
   }

    /*Right band*/
   for(i=0; i<p_Vid->height; i++)
   {
      for(j=p_Vid->width; j<p_Vid->width+IMG_PAD_SIZE_X; j++)
      {
         dst = &p_Vid->pixelDistributionWorking[i][j];
         src = &p_Vid->pixelDistributionWorking[i][p_Vid->width-1];

         memcpy(dst->map, src->map, sizeof(unsigned char)*256);
         memcpy(dst->prob, src->prob, sizeof(float)*256);
      }
   }

   /*Bottom band*/
   /*first row, left padded region*/
   for (i = -IMG_PAD_SIZE_X; i < 0; i++)
   {
      dst = &p_Vid->pixelDistributionWorking[p_Vid->height][i];
      src = &p_Vid->pixelDistributionWorking[p_Vid->height-1][0];

      memcpy(dst->map, src->map, sizeof(unsigned char)*256);
      memcpy(dst->prob, src->prob, sizeof(float)*256);
   }
   /*first row, middle region*/
   for (i = 0; i < p_Vid->width; i++)
   {
      dst = &p_Vid->pixelDistributionWorking[p_Vid->height][i];
      src = &p_Vid->pixelDistributionWorking[p_Vid->height-1][i];

      memcpy(dst->map, src->map, sizeof(unsigned char)*256);
      memcpy(dst->prob, src->prob, sizeof(float)*256);
   }
   /*first row, right padded region*/
   for (i = p_Vid->width; i < p_Vid->width+IMG_PAD_SIZE_X; i++)
   {
      dst = &p_Vid->pixelDistributionWorking[p_Vid->height][i];
      src = &p_Vid->pixelDistributionWorking[p_Vid->height-1][p_Vid->width-1];

      memcpy(dst->map, src->map, sizeof(unsigned char)*256);
      memcpy(dst->prob, src->prob, sizeof(float)*256);
   }
   /*replicate the bottom band*/
   for(i=p_Vid->height+1; i<p_Vid->height+IMG_PAD_SIZE_Y; i++)
   {
      for(j=-IMG_PAD_SIZE_X; j<p_Vid->width+IMG_PAD_SIZE_X; j++)
      {
         dst = &p_Vid->pixelDistributionWorking[i][j];
         src = &p_Vid->pixelDistributionWorking[p_Vid->height][j];

         memcpy(dst->map, src->map, sizeof(unsigned char)*256);
         memcpy(dst->prob, src->prob, sizeof(float)*256);
      }
   }
}
#endif
/*!
 ***********************************************************************
 * \brief
 *    Encode a sequence
 ***********************************************************************
 */
#if SAVE_ENCODER_LUMA_RECON
void saveEncReconDataToFile(VideoParameters *p_Vid, boolean flag)//debug
{
    int i;
    int j;

   imgpel **img_enc;

   if(flag)
   {
      img_enc = p_Vid->enc_picture->p_curr_img; /*reconstructed image y*/
      for(i=0; i<p_Vid->height; i++)
      {
         for(j=0; j<p_Vid->width; j++)
         {
            unsigned char temp = (unsigned char)img_enc[i][j];
            fwrite(&temp, 1, 1, encLumaReconFile);
         }
      }
#if 0
      img_enc = p_Vid->enc_picture->imgUV[0]; /*reconstructed image u*/
      for(i=0; i<p_Vid->height>>1; i++)
      {
         for(j=0; j<p_Vid->width>>1; j++)
         {
            unsigned char temp = (unsigned char)img_enc[i][j];
            fwrite(&temp, 1, 1, encLumaReconFile);
         }
      }

      img_enc = p_Vid->enc_picture->imgUV[1]; /*reconstructed image v*/
      for(i=0; i<p_Vid->height>>1; i++)
      {
         for(j=0; j<p_Vid->width>>1; j++)
         {
            unsigned char temp = (unsigned char)img_enc[i][j];
            fwrite(&temp, 1, 1, encLumaReconFile);
         }
      }
#endif
   }
}
#endif
#if SAVE_MANUAL_LUMA_RECON
unsigned char manReconData[1088][1920]; //make it big enough for 1080p
void saveManReconDataToFile(VideoParameters *p_Vid, boolean flag)//debug
{
    int i;
    int j;
    if(flag)
    {
      for(i=0; i<p_Vid->height; i++)
      {
         for(j=0; j<p_Vid->width; j++)
         {
           fwrite(&manReconData[i][j], 1, 1, manLumaReconFile);
         }
      }
    }
}//debug
#endif
#if WANT_ROPE_PMF
static void encode_sequence(VideoParameters *p_Vid, InputParameters *p_Inp, char mailBoxNameIn[STR_SIZE], char mailBoxNameOut[STR_SIZE])
#else
static void encode_sequence(VideoParameters *p_Vid, InputParameters *p_Inp)
#endif
{
  int curr_frame_to_code;
  int frames_to_code;
  int frame_num_bak = 0, frame_coded;
  int frm_struct_buffer;
  SeqStructure *p_seq_struct = p_Vid->p_pred;
  FrameUnitStruct *p_frm;

#if (MVC_EXTENSION_ENABLE)
  int tmp_rate_control_enable = p_Inp->RCEnable;

  if ( p_Inp->num_of_views == 2 )
  {
    frames_to_code = p_Inp->no_frames << 1;
    p_frm = p_seq_struct->p_frm_mvc;
    frm_struct_buffer = p_seq_struct->num_frames_mvc;
  }
  else
#endif
  {
    frames_to_code = p_Inp->no_frames;
    p_frm = p_seq_struct->p_frm;
    frm_struct_buffer = p_Vid->frm_struct_buffer;
  }

  for (curr_frame_to_code = 0; curr_frame_to_code < frames_to_code; curr_frame_to_code++)
  {
#if (MVC_EXTENSION_ENABLE)
    if ( p_Inp->num_of_views == 2 )
    {
      if ( (curr_frame_to_code & 1) == 0 ) // call only for view_id 0
      {
        // determine whether to populate additional frames in the prediction structure
        if ( (curr_frame_to_code >> 1) >= p_Vid->p_pred->pop_start_frame )
        {
          int start = p_seq_struct->pop_start_frame, end;

          populate_frm_struct( p_Vid, p_Inp, p_seq_struct, p_Inp->FrmStructBufferLength, frames_to_code >> 1 );
          end = p_seq_struct->pop_start_frame;
          populate_frm_struct_mvc( p_Vid, p_Inp, p_seq_struct, start, end );
        }
      }
    }
    else
#endif
    {
      // determine whether to populate additional frames in the prediction structure
      if ( curr_frame_to_code >= p_Vid->p_pred->pop_start_frame )
      {
        populate_frm_struct( p_Vid, p_Inp, p_seq_struct, p_Inp->FrmStructBufferLength, frames_to_code );
      }
    }

    p_Vid->curr_frm_idx = curr_frame_to_code;
    p_Vid->p_curr_frm_struct = p_frm + ( p_Vid->curr_frm_idx % frm_struct_buffer ); // pointer to current frame structure
    p_Vid->number = curr_frame_to_code;

#if (MVC_EXTENSION_ENABLE)
    if(p_Inp->num_of_views==2)
    {
      p_Vid->view_id = p_Vid->p_curr_frm_struct->view_id;
      SetDpbLayerId(p_Vid, p_Vid->view_id);
      if ( p_Vid->view_id == 1 )
      {
        p_Vid->curr_frm_idx = p_Vid->number = (curr_frame_to_code - 1) >> 1;
        p_Vid->p_curr_frm_struct->qp = p_Vid->qp = iClip3( -p_Vid->bitdepth_luma_qp_scale, MAX_QP, p_Vid->AverageFrameQP + p_Inp->View1QPOffset );
      }
      else
      {
        p_Vid->curr_frm_idx = p_Vid->number = curr_frame_to_code >> 1;
      }
      if ( p_Vid->view_id == 1 && tmp_rate_control_enable )
      {
        p_Inp->RCEnable = 0;
      }
      else
      {
        p_Inp->RCEnable = tmp_rate_control_enable;
      }
    }
#endif

    if ( p_Vid->p_curr_frm_struct->frame_no >= p_Inp->no_frames )
    {
      continue;
    }

    // Update frame_num counter
    frame_num_bak = p_Vid->frame_num;
    if (p_Vid->last_ref_idc == 1)
    {
      p_Vid->frame_num++;

#if (MVC_EXTENSION_ENABLE)
      if ( p_Vid->num_of_layers == 2 )
      {
        p_Vid->frame_num %= (p_Vid->max_frame_num << 1);
      }
      else
#endif
      p_Vid->frame_num %= p_Vid->max_frame_num;
    }

    prepare_frame_params(p_Vid, p_Inp, curr_frame_to_code);

    // redundant frame initialization and allocation
    if (p_Inp->redundant_pic_flag)
    {
      init_redundant_frame(p_Vid, p_Inp);
      set_redundant_frame(p_Vid, p_Inp);
    }
#if SAVE_MANUAL_LUMA_RECON
    memset(manReconData, 0, 1088*1920);
#endif
#if WANT_ROPE_PMF
    /*wait for the parent process to signal us*/
    while(1)
    {
       int incomingMessage;
       incomingMessage = msgRead(p_Vid->mailHandleIn);
       if(incomingMessage==MSG_START_FRAME)
       {
          //printf("PID:%d Got message will encode frame\n",GetCurrentProcessId());//debug
          break;
       }
       else if(incomingMessage==MSG_NO_MESSAGE)
       {
       }
       else
       {
          printf("PID:%d Received unexpected message %d\n", GetCurrentProcessId(), incomingMessage);
          exit(0);
       }
       Sleep(200);
    }
	ROPEAllocWorkingMem(p_Vid);
#endif
    frame_coded = encode_one_frame(p_Vid, p_Inp); // encode one frame;
#if WANT_ROPE_PMF
    /*save the frame data to a file*/
    {
       int i;
       int j;
       imgpel **img_enc;
       FILE *tempFile;
       char tempFileName[256];
       sprintf(tempFileName,"%s.sin",p_Inp->ReconFile);
       tempFile = fopen(tempFileName,"wb");
       if(tempFile==NULL)
       {
          printf("Error opening file for writing\n");
          exit(0);
       }
       img_enc = p_Vid->enc_picture->p_curr_img; /*reconstructed image y*/
       for(i=0; i<p_Vid->height; i++)
       {
          for(j=0; j<p_Vid->width; j++)
          {
             unsigned char temp = (unsigned char)img_enc[i][j];
             fwrite(&temp, 1, 1, tempFile);
          }
       }
       img_enc = p_Vid->enc_picture->imgUV[0]; /*reconstructed image u*/
       for(i=0; i<p_Vid->height>>1; i++)
       {
          for(j=0; j<p_Vid->width>>1; j++)
          {
             unsigned char temp = (unsigned char)img_enc[i][j];
             fwrite(&temp, 1, 1, tempFile);
          }
       }
       img_enc = p_Vid->enc_picture->imgUV[1]; /*reconstructed image v*/
       for(i=0; i<p_Vid->height>>1; i++)
       {
          for(j=0; j<p_Vid->width>>1; j++)
          {
             unsigned char temp = (unsigned char)img_enc[i][j];
             fwrite(&temp, 1, 1, tempFile);
          }
       }
       fclose(tempFile);
    }
    ROPEprocessPMFPaddedRegion(p_Vid);
    ROPEverifyFramePMF(p_Vid);//debug check
    //ROPEverifyFrameFirstMomentNoLoss(p_Vid);//debug check
    ROPEcalculateFrameDistortion(p_Vid, p_Inp);
    /*transfer the distortion data to the shared memory*/
    memcpy(p_Vid->pixelDistribution1DShared, p_Vid->pixelDistribution1DWorking, sizeof(PixelPMF)*(p_Vid->height+2*IMG_PAD_SIZE_Y)*(p_Vid->width+2*IMG_PAD_SIZE_X));
    ROPEFreeWorkingMem(p_Vid);
    /*signal that we finished with a frame*/
    {
       int outgoingMessage=MSG_FRAME_FINISHED;
       msgSend(p_Vid->mailHandleOut, &outgoingMessage, sizeof(int));
       //printf("PID:%d Finished with frame message sent\n",GetCurrentProcessId());//debug
    }
#endif
#if SAVE_ENCODER_LUMA_RECON
    saveEncReconDataToFile(p_Vid, TRUE);
#endif
#if SAVE_MANUAL_LUMA_RECON
    saveManReconDataToFile(p_Vid, TRUE);
#endif
    if ( !frame_coded )
    {
      p_Vid->frame_num = frame_num_bak;
      continue;
    }

    p_Vid->last_ref_idc = p_Vid->nal_reference_idc ? 1 : 0;

    // if key frame is encoded, encode one redundant frame
    if (p_Inp->redundant_pic_flag && p_Vid->key_frame)
    {
      encode_one_redundant_frame(p_Vid, p_Inp);
    }

    if (p_Vid->type == I_SLICE && p_Inp->EnableOpenGOP && p_Vid->p_curr_frm_struct->random_access)
    {
      p_Vid->last_valid_reference = p_Vid->ThisPOC;
    }

    if (p_Inp->ReportFrameStats)
    {
      report_frame_statistic(p_Vid, p_Inp);
    }

  }

#if EOS_OUTPUT
  end_of_stream(p_Vid);
#endif

#if (MVC_EXTENSION_ENABLE)
  if(p_Inp->num_of_views == 2) //? it should use num_of_layers;
  {
    p_Inp->RCEnable = tmp_rate_control_enable;
  }
#endif
}


/*!
 ***********************************************************************
 * \brief
 *    Free memory allocated for the encoder
 ***********************************************************************
 */

void free_encoder_memory(VideoParameters *p_Vid, InputParameters *p_Inp)
{
  terminate_sequence(p_Vid, p_Inp);
  flush_dpb(p_Vid->p_Dpb_layer[0], &p_Inp->output);
  flush_dpb(p_Vid->p_Dpb_layer[1], &p_Inp->output);
  CloseFiles(&p_Inp->input_file1);

  if (-1 != p_Vid->p_dec)
    close(p_Vid->p_dec);

#if (MVC_EXTENSION_ENABLE)
  if(p_Vid->num_of_layers==2)
    CloseFiles(&p_Inp->input_file2);
  if (-1 != p_Vid->p_dec2)
    close(p_Vid->p_dec2);
#endif

  if (p_Enc->p_trace)
    fclose(p_Enc->p_trace);

  Clear_Motion_Search_Module (p_Vid, p_Inp);

  RandomIntraUninit(p_Vid);
  FmoUninit(p_Vid);

  if (p_Inp->NumberBFrames && p_Inp->HierarchicalCoding == 3)
  {
    clear_gop_structure (p_Vid);
  }

#ifdef _LEAKYBUCKET_
  calc_buffer(p_Vid, p_Inp);
#endif

  // report everything
  report(p_Vid, p_Inp, p_Vid->p_Stats);

#ifdef _LEAKYBUCKET_
  free_pointer(p_Vid->Bit_Buffer);
#endif
  free_dpb(p_Vid->p_Dpb_layer[0]);
  free_dpb(p_Vid->p_Dpb_layer[1]);
  uninit_out_buffer(p_Vid);

  free_global_buffers(p_Vid, p_Inp);

  FreeParameterSets(p_Vid);

  if (p_Inp->ExplicitSeqCoding)
    CloseExplicitSeqFile(p_Vid);

  // free image mem
  free_img (p_Vid, p_Inp);
}

/*!
 ***********************************************************************
 * \brief
 *    Initializes the Image structure with appropriate parameters.
 * \par Input:
 *    Input Parameters InputParameters *inp
 * \par  Output:
 *    Image Parameters VideoParameters *p_Vid
 ***********************************************************************
 */
static void init_img( VideoParameters *p_Vid)
{
  InputParameters *p_Inp = p_Vid->p_Inp;
  int i, j;

  p_Vid->number         = -1;

  p_Vid->last_idr_code_order = 0;
  p_Vid->last_idr_disp_order = 0;
  p_Vid->last_mmco_5_code_order = -1;
  p_Vid->last_mmco_5_disp_order = -1;
  // Color format
  p_Vid->yuv_format  = p_Inp->output.yuv_format;
  p_Vid->P444_joined = (p_Vid->yuv_format == YUV444 && (p_Inp->separate_colour_plane_flag == 0));

  //pel bitdepth init
  p_Vid->bitdepth_luma            = (short) p_Inp->output.bit_depth[0];
  p_Vid->bitdepth_scale[0]        = 1 << (p_Vid->bitdepth_luma - 8);
  p_Vid->bitdepth_lambda_scale    = 2 * (p_Vid->bitdepth_luma - 8);
  p_Vid->bitdepth_luma_qp_scale   = 3 *  p_Vid->bitdepth_lambda_scale;
  p_Vid->dc_pred_value_comp[0]    =  (imgpel) (1<<(p_Vid->bitdepth_luma - 1));
  p_Vid->max_pel_value_comp[0] = (1<<p_Vid->bitdepth_luma) - 1;
  p_Vid->max_imgpel_value_comp_sq[0] = p_Vid->max_pel_value_comp[0] * p_Vid->max_pel_value_comp[0];

  p_Vid->dc_pred_value            = p_Vid->dc_pred_value_comp[0]; // set defaults
  p_Vid->max_imgpel_value         = (short) p_Vid->max_pel_value_comp[0];
  p_Vid->mb_size[0][0]            = p_Vid->mb_size[0][1] = MB_BLOCK_SIZE;

  // Initialization for RC QP parameters (could be placed in ratectl.c)
  p_Vid->RCMinQP                = p_Inp->RCMinQP[P_SLICE];
  p_Vid->RCMaxQP                = p_Inp->RCMaxQP[P_SLICE];

  p_Vid->WalkAround = 0;
  p_Vid->NumberOfMBs = 0;

  // Set current residue & prediction array pointers

  if (p_Vid->active_sps->profile_idc == BASELINE || p_Vid->active_sps->profile_idc == MAIN || p_Vid->active_sps->profile_idc == EXTENDED)
    p_Vid->min_IPCM_value = 1;  // See Annex A for restriction in pcm sample values for pre FRExt profiles
  else
    p_Vid->min_IPCM_value = 0;

  if (p_Vid->yuv_format != YUV400)
  {
    p_Vid->bitdepth_chroma             = (short) p_Inp->output.bit_depth[1];
    p_Vid->bitdepth_scale[1]           = 1 << (p_Vid->bitdepth_chroma - 8);
    p_Vid->dc_pred_value_comp[1]       = (imgpel) (1<<(p_Vid->bitdepth_chroma - 1));
    p_Vid->dc_pred_value_comp[2]       = p_Vid->dc_pred_value_comp[1];
    p_Vid->max_pel_value_comp[1]       = (1<<p_Vid->bitdepth_chroma) - 1;
    p_Vid->max_pel_value_comp[2]       = p_Vid->max_pel_value_comp[1];
    p_Vid->max_imgpel_value_comp_sq[1] = p_Vid->max_pel_value_comp[1] * p_Vid->max_pel_value_comp[1];
    p_Vid->max_imgpel_value_comp_sq[2] = p_Vid->max_pel_value_comp[2] * p_Vid->max_pel_value_comp[2];
    p_Vid->num_blk8x8_uv               = (1<<p_Vid->yuv_format)&(~(0x1));
    p_Vid->num_cdc_coeff               = p_Vid->num_blk8x8_uv << 1;

    p_Vid->mb_size[1][0] = p_Vid->mb_size[2][0] = p_Vid->mb_cr_size_x = (p_Vid->yuv_format == YUV420 || p_Vid->yuv_format == YUV422) ? 8 : 16;
    p_Vid->mb_size[1][1] = p_Vid->mb_size[2][1] = p_Vid->mb_cr_size_y = (p_Vid->yuv_format == YUV444 || p_Vid->yuv_format == YUV422) ? 16 : 8;

    p_Vid->bitdepth_chroma_qp_scale = 6*(p_Vid->bitdepth_chroma - 8);

    p_Vid->chroma_qp_offset[0] = p_Vid->active_pps->cb_qp_index_offset;
    p_Vid->chroma_qp_offset[1] = p_Vid->active_pps->cr_qp_index_offset;
  }
  else
  {
    p_Vid->bitdepth_chroma     = 0;
    p_Vid->bitdepth_scale[1]   = 0;
    p_Vid->max_pel_value_comp[1] = 0;
    p_Vid->max_pel_value_comp[2] = p_Vid->max_pel_value_comp[1];
    p_Vid->max_imgpel_value_comp_sq[1] = p_Vid->max_pel_value_comp[1] * p_Vid->max_pel_value_comp[1];
    p_Vid->max_imgpel_value_comp_sq[2] = p_Vid->max_pel_value_comp[2] * p_Vid->max_pel_value_comp[2];
    p_Vid->num_blk8x8_uv       = 0;
    p_Vid->num_cdc_coeff       = 0;
    p_Vid->mb_size[1][0] = p_Vid->mb_size[2][0] = p_Vid->mb_cr_size_x = 0;
    p_Vid->mb_size[1][1] = p_Vid->mb_size[2][1] = p_Vid->mb_cr_size_y = 0;

    p_Vid->bitdepth_chroma_qp_scale = 0;

    p_Vid->chroma_qp_offset[0] = 0;
    p_Vid->chroma_qp_offset[1] = 0;
  }

  p_Vid->max_bitCount =  128 + 256 * p_Vid->bitdepth_luma + 2 * p_Vid->mb_cr_size_y * p_Vid->mb_cr_size_x * p_Vid->bitdepth_chroma;
  //p_Vid->max_bitCount =  (128 + 256 * p_Vid->bitdepth_luma + 2 *p_Vid->mb_cr_size_y * p_Vid->mb_cr_size_x * p_Vid->bitdepth_chroma)*2;

  p_Vid->max_qp_delta = (25 + (p_Vid->bitdepth_luma_qp_scale>>1));
  p_Vid->min_qp_delta = p_Vid->max_qp_delta + 1;

  p_Vid->num_ref_frames = p_Vid->active_sps->num_ref_frames;
  p_Vid->max_num_references = p_Vid->active_sps->frame_mbs_only_flag ? p_Vid->active_sps->num_ref_frames : 2 * p_Vid->active_sps->num_ref_frames;

#if (MVC_EXTENSION_ENABLE)
  if(p_Vid->num_of_layers == 2) //if(p_Inp->num_of_views==2)//? Yuwen: not sure it should use num_of_layers or not;
  {
    //p_Vid->num_ref_frames <<= 1;
    p_Vid->max_num_references <<= 1;
  }
  p_Vid->sec_view_force_fld = 0;
#endif

  p_Vid->base_dist = p_Inp->jumpd + 1;

  // Intra/IDR related parameters
  p_Vid->lastIntraNumber = 0;
  p_Vid->lastINTRA       = 0;
  p_Vid->last_ref_idc    = 0;
  p_Vid->idr_refresh     = 0;

  p_Vid->framerate       = (float) p_Inp->output.frame_rate;   // The basic frame rate (of the original sequence)

  if (p_Inp->AdaptiveRounding)
  {
    if (p_Vid->yuv_format != 0)
    {
      get_mem4Dint(&(p_Vid->ARCofAdj4x4), 3, MAXMODE, MB_BLOCK_SIZE, MB_BLOCK_SIZE); //all modes
      get_mem4Dint(&(p_Vid->ARCofAdj8x8), p_Vid->P444_joined ? 3 : 1, MAXMODE, MB_BLOCK_SIZE, MB_BLOCK_SIZE); //modes 0, 1, 2, 3, P8x8
    }
    else
    {
      get_mem4Dint(&(p_Vid->ARCofAdj4x4), 1, MAXMODE, MB_BLOCK_SIZE, MB_BLOCK_SIZE); //all modes
      get_mem4Dint(&(p_Vid->ARCofAdj8x8), 1, MAXMODE, MB_BLOCK_SIZE, MB_BLOCK_SIZE); //modes 0, 1, 2, 3, P8x8
    }
  }

  p_Vid->width         = (p_Inp->output.width[0]  + p_Vid->auto_crop_right);
  p_Vid->height        = (p_Inp->output.height[0] + p_Vid->auto_crop_bottom);
  p_Vid->width_blk     = p_Vid->width  / BLOCK_SIZE;
  p_Vid->height_blk    = p_Vid->height / BLOCK_SIZE;
  p_Vid->width_padded  = p_Vid->width  + 2 * IMG_PAD_SIZE_X;
  p_Vid->height_padded = p_Vid->height + 2 * IMG_PAD_SIZE_Y;

  if (p_Vid->yuv_format != YUV400)
  {
    p_Vid->width_cr = p_Vid->width  * mb_width_cr [p_Vid->yuv_format] / 16;
    p_Vid->height_cr= p_Vid->height * mb_height_cr[p_Vid->yuv_format] / 16;
  }
  else
  {
    p_Vid->width_cr = 0;
    p_Vid->height_cr= 0;
  }

  p_Vid->height_cr_frame = p_Vid->height_cr;

  p_Vid->size = p_Vid->width * p_Vid->height;
  p_Vid->size_cr = p_Vid->width_cr * p_Vid->height_cr;

  p_Vid->PicWidthInMbs    = p_Vid->width  / MB_BLOCK_SIZE;
  p_Vid->FrameHeightInMbs = p_Vid->height / MB_BLOCK_SIZE;
  p_Vid->FrameSizeInMbs   = p_Vid->PicWidthInMbs * p_Vid->FrameHeightInMbs;

  p_Vid->PicHeightInMapUnits = ( p_Vid->active_sps->frame_mbs_only_flag ? p_Vid->FrameHeightInMbs : p_Vid->FrameHeightInMbs >> 1 );

  if ((p_Vid->b8x8info = (Block8x8Info *) calloc(1, sizeof(Block8x8Info))) == NULL)
     no_mem_exit("init_img: p_Vid->block8x8info");

  if( (p_Inp->separate_colour_plane_flag != 0) )
  {
    for( i = 0; i < MAX_PLANE; i++ ){
      //if ((p_Vid->mb_data_JV[i] = (Macroblock *) calloc(p_Vid->FrameSizeInMbs,sizeof(Macroblock))) == NULL)
      if((p_Vid->mb_data_JV[i] = alloc_mbs(p_Vid, p_Vid->FrameSizeInMbs, p_Vid->num_of_layers)) == NULL)
        no_mem_exit("init_img: p_Vid->mb_data_JV");
    }
    p_Vid->mb_data = NULL;
  }
  else
  {
    //if ((p_Vid->mb_data = (Macroblock *) calloc(p_Vid->FrameSizeInMbs, sizeof(Macroblock))) == NULL)
    if ((p_Vid->mb_data = alloc_mbs(p_Vid, p_Vid->FrameSizeInMbs, p_Vid->num_of_layers)) == NULL)
      no_mem_exit("init_img: p_Vid->mb_data");
  }

  if (p_Inp->UseConstrainedIntraPred)
  {
    if ((p_Vid->intra_block = (short*) calloc(p_Vid->FrameSizeInMbs, sizeof(short))) == NULL)
      no_mem_exit("init_img: p_Vid->intra_block");
  }

  if (p_Inp->CtxAdptLagrangeMult == 1)
  {
    if ((p_Vid->mb16x16_cost_frame = (double*)calloc(p_Vid->FrameSizeInMbs, sizeof(double))) == NULL)
    {
      no_mem_exit("init p_Vid->mb16x16_cost_frame");
    }
  }
  get_mem2D((byte***)&(p_Vid->ipredmode), p_Vid->height_blk, p_Vid->width_blk);        //need two extra rows at right and bottom
  get_mem2D((byte***)&(p_Vid->ipredmode8x8), p_Vid->height_blk, p_Vid->width_blk);     // help storage for ipredmode 8x8, inserted by YV
  memset(&(p_Vid->ipredmode[0][0])   , -1, p_Vid->height_blk * p_Vid->width_blk *sizeof(char));
  memset(&(p_Vid->ipredmode8x8[0][0]), -1, p_Vid->height_blk * p_Vid->width_blk *sizeof(char));


  // CAVLC mem
  if(p_Vid->num_of_layers>1)
  {
   get_mem3Dint(&(p_Vid->nz_coeff_buf[0]), p_Vid->FrameSizeInMbs, 4, 4+p_Vid->num_blk8x8_uv);
   if(p_Vid->p_Dpb_layer[1]->storage_format.yuv_format != YUV400 && p_Vid->p_Dpb_layer[1]->storage_format.yuv_format != p_Vid->p_Dpb_layer[0]->storage_format.yuv_format)
   {
     int num_blk8x8_uv = (1<<p_Vid->p_Dpb_layer[1]->storage_format.yuv_format)&(~(0x1));
     get_mem3Dint(&(p_Vid->nz_coeff_buf[1]), p_Vid->FrameSizeInMbs, 4, 4+num_blk8x8_uv);
   }
   else
     p_Vid->nz_coeff_buf[1] = p_Vid->nz_coeff_buf[0];
   p_Vid->nz_coeff = p_Vid->nz_coeff_buf[0];
  }
  else
  {
   get_mem3Dint(&(p_Vid->nz_coeff_buf[0]), p_Vid->FrameSizeInMbs, 4, 4+p_Vid->num_blk8x8_uv);
   p_Vid->nz_coeff = p_Vid->nz_coeff_buf[0];
   p_Vid->nz_coeff_buf[1] = NULL;
  }

  get_mem2Dolm     (&(p_Vid->lambda_buf[0])   , 10, 52 + p_Vid->bitdepth_luma_qp_scale, p_Vid->bitdepth_luma_qp_scale);
  p_Vid->lambda = p_Vid->lambda_buf[0];
  get_mem2Dodouble (&(p_Vid->lambda_md_buf[0]), 10, 52 + p_Vid->bitdepth_luma_qp_scale, p_Vid->bitdepth_luma_qp_scale);
  p_Vid->lambda_md = p_Vid->lambda_md_buf[0];
  get_mem3Dodouble (&(p_Vid->lambda_me_buf[0]), 10, 52 + p_Vid->bitdepth_luma_qp_scale, 3, p_Vid->bitdepth_luma_qp_scale);
  p_Vid->lambda_me = p_Vid->lambda_me_buf[0];
  get_mem3Doint    (&(p_Vid->lambda_mf_buf[0]), 10, 52 + p_Vid->bitdepth_luma_qp_scale, 3, p_Vid->bitdepth_luma_qp_scale);
  p_Vid->lambda_mf = p_Vid->lambda_mf_buf[0];
  if ( p_Inp->UseRDOQuant )
  {
    get_mem2Dodouble (&(p_Vid->lambda_rdoq_buf[0]), 10, 52 + p_Vid->bitdepth_luma_qp_scale, p_Vid->bitdepth_luma_qp_scale);
    p_Vid->lambda_rdoq = p_Vid->lambda_rdoq_buf[0];
  }
  if (p_Inp->CtxAdptLagrangeMult == 1)
  {
    get_mem2Dodouble(&(p_Vid->lambda_mf_factor_buf[0]), 10, 52 + p_Vid->bitdepth_luma_qp_scale, p_Vid->bitdepth_luma_qp_scale);
    p_Vid->lambda_mf_factor = p_Vid->lambda_mf_factor_buf[0];
  }
  if(p_Vid->num_of_layers>1)
  {
      p_Vid->lambda_buf[1] = p_Vid->lambda_buf[0];
      p_Vid->lambda_md_buf[1] = p_Vid->lambda_md_buf[0];
      p_Vid->lambda_me_buf[1] = p_Vid->lambda_me_buf[0];
      p_Vid->lambda_mf_buf[1] = p_Vid->lambda_mf_buf[0];
      p_Vid->lambda_rdoq_buf[1] = p_Vid->lambda_rdoq_buf[0];
      p_Vid->lambda_mf_factor_buf[1] = p_Vid->lambda_mf_factor_buf[0];
  }

  p_Vid->mb_y_upd  = 0;

  if(((p_Inp->RDPictureDecision) && p_Inp->GenerateMultiplePPS) || (p_Inp->WeightedPrediction || p_Inp->WeightedBiprediction))
  {
    int num_slices;

    if(p_Inp->slice_mode == 1)
    {
      num_slices = p_Vid->FrameSizeInMbs/p_Inp->slice_argument;
      if((unsigned int)(num_slices * p_Inp->slice_argument) < p_Vid->FrameSizeInMbs)
        num_slices++;
    }
    else
      num_slices = 1;

    // All weight structures assigned using "MAX_REFERENCE_PICTURES". This plays it safe since one may
    // consider reordering or use this for MVC/FCFR coding where references are expanded by 1.
    p_Vid->num_slices_wp = num_slices;
    get_mem4Dshort(&(p_Vid->wp_weights), 3, 2, MAX_REFERENCE_PICTURES, num_slices);
    get_mem4Dshort(&(p_Vid->wp_offsets), 3, 2, MAX_REFERENCE_PICTURES, num_slices);
    get_mem5Dshort(&(p_Vid->wbp_weight), 3, 2, MAX_REFERENCE_PICTURES, MAX_REFERENCE_PICTURES, num_slices);
  }
  else
  {
    p_Vid->num_slices_wp = 0;
    p_Vid->wp_weights = NULL;
    p_Vid->wp_offsets = NULL;
    p_Vid->wbp_weight = NULL;
  }

  RandomIntraInit (p_Vid, p_Vid->PicWidthInMbs, p_Vid->FrameHeightInMbs, p_Inp->RandomIntraMBRefresh);

  InitSEIMessages(p_Vid, p_Inp);

  initInput(p_Vid, &p_Inp->source, &p_Inp->output);

  // Allocate I/O Frame memory
  AllocateFrameMemory(p_Vid, p_Inp, &p_Inp->source);

  // Initialize filtering parameters. If sending parameters, the offsets are
  // multiplied by 2 since inputs are taken in "div 2" format.
  // If not sending parameters, all fields are cleared
  if (p_Inp->DFSendParameters)
  {
    for (j = 0; j < 2; j++)
    {
      for (i = 0; i < NUM_SLICE_TYPES; i++)
      {
        p_Inp->DFAlpha[j][i] <<= 1;
        p_Inp->DFBeta [j][i] <<= 1;
#if (MVC_EXTENSION_ENABLE)
        if (p_Vid->num_of_layers == 2)
        {
          p_Inp->EnhLayerDFAlpha[j][i] <<= 1;
          p_Inp->EnhLayerDFBeta [j][i] <<= 1;
        }
#endif
      }
    }
  }
  else
  {
    for (j = 0; j < 2; j++)
    {
      for (i = 0; i < NUM_SLICE_TYPES; i++)
      {
        p_Inp->DFDisableIdc[j][i] = 0;
        p_Inp->DFAlpha     [j][i] = 0;
        p_Inp->DFBeta      [j][i] = 0;
#if (MVC_EXTENSION_ENABLE)
        if (p_Vid->num_of_layers == 2)
        {
          p_Inp->EnhLayerDFDisableIdc[j][i] = 0;
          p_Inp->EnhLayerDFAlpha[j][i] = 0;
          p_Inp->EnhLayerDFBeta [j][i] = 0;
        }
#endif
      }
    }
  }

  p_Vid->ChromaArrayType = p_Inp->separate_colour_plane_flag ? 0 : p_Inp->output.yuv_format;
  p_Vid->colour_plane_id = 0;

  if (p_Inp->RDPictureDecision)
    p_Vid->frm_iter = 3;
  else
    p_Vid->frm_iter = 1;

  p_Vid->max_frame_num = 1 << (p_Vid->log2_max_frame_num_minus4 + 4);
  p_Vid->max_pic_order_cnt_lsb = 1 << (p_Vid->log2_max_pic_order_cnt_lsb_minus4 + 4);

  p_Vid->prev_frame_no = 0; // POC200301
  p_Vid->consecutive_non_reference_pictures = 0; // POC200301

  p_Vid->fld_type = 0;

  p_Vid->p_Inp = p_Inp;

  create_context_memory (p_Vid, p_Inp);

}


/*!
 ***********************************************************************
 * \brief
 *    Free the Image structures
 * \par Input:
 *    Image Parameters VideoParameters *p_Vid
 ***********************************************************************
 */
static void free_img (VideoParameters *p_Vid, InputParameters *p_Inp)
{
  // Delete Frame memory
  DeleteFrameMemory(p_Vid);

  CloseSEIMessages(p_Vid, p_Inp);

  free_context_memory (p_Vid);

  if (p_Inp->AdaptiveRounding)
  {
    free_mem4Dint(p_Vid->ARCofAdj4x4);
    free_mem4Dint(p_Vid->ARCofAdj8x8);
  }

  if (p_Vid->wp_weights)
    free_mem4Dshort(p_Vid->wp_weights);
  if (p_Vid->wp_offsets)
    free_mem4Dshort(p_Vid->wp_offsets);
  if (p_Vid->wbp_weight)
    free_mem5Dshort(p_Vid->wbp_weight);


  free_pointer (p_Vid->p_SEI);
  free_pointer (p_Vid->p_QScale);
  free_pointer (p_Vid->p_Quant);
  free_pointer(p_Vid->p_Dpb_layer[0]);
  p_Vid->p_Dpb_layer[0] = p_Vid->p_Dpb_layer[1] = NULL;
  free_pointer (p_Vid->p_Stats);
  free_pointer (p_Vid->p_Dist);
  //
  free_encode_parameters(p_Vid);
  free_pointer (p_Vid);
}


/*!
 ***********************************************************************
 * \brief
 *    Free the Input structures
 * \par Input:
 *    Input Parameters InputParameters *p_Inp
 ***********************************************************************
 */
static void free_params (InputParameters *p_Inp)
{
  if ( p_Inp != NULL )
  {
    free_pointer( p_Inp->top_left );
    free_pointer( p_Inp->bottom_right );
    free_pointer( p_Inp->slice_group_id );
    free_pointer( p_Inp->run_length_minus1 );
    free_pointer( p_Inp );
  }
}


/*!
 ************************************************************************
 * \brief
 *    Allocates the picture structure along with its dependent
 *    data structures
 * \return
 *    Pointer to a Picture
 ************************************************************************
 */
Picture *malloc_picture()
{
  Picture *pic;
  if ((pic = calloc (1, sizeof (Picture))) == NULL) no_mem_exit ("malloc_picture: Picture structure");
  //! Note: slice structures are allocated as needed in code_a_picture
  return pic;
}

/*!
 ************************************************************************
 * \brief
 *    Frees a picture
 * \param
 *    pic: POinter to a Picture to be freed
 ************************************************************************
 */
void free_picture(Picture *pic)
{
  if (pic != NULL)
  {
    free_slice_list(pic);
    free_pointer (pic);
  }
}


/*!
 ************************************************************************
 * \brief
 *    memory allocation for original picture buffers
 ************************************************************************
 */
int init_orig_buffers(VideoParameters *p_Vid, ImageData *imgData)
{
  InputParameters *p_Inp = p_Vid->p_Inp;
  int memory_size = 0;
  int nplane;

  // allocate memory for reference frame buffers: imgData->frm_data
  imgData->format           = p_Inp->output;
  imgData->format.width[0]  = p_Vid->width;
  imgData->format.width[1]  = p_Vid->width_cr;
  imgData->format.width[2]  = p_Vid->width_cr;
  imgData->format.height[0] = p_Vid->height;
  imgData->format.height[1] = p_Vid->height_cr;
  imgData->format.height[2] = p_Vid->height_cr;
  imgData->format.yuv_format = p_Vid->yuv_format;
  imgData->format.auto_crop_bottom = p_Vid->auto_crop_bottom;
  imgData->format.auto_crop_right  = p_Vid->auto_crop_right;
  imgData->format.auto_crop_bottom_cr = (p_Vid->auto_crop_bottom * mb_height_cr [p_Vid->yuv_format]) / MB_BLOCK_SIZE;
  imgData->format.auto_crop_right_cr  = (p_Vid->auto_crop_right * mb_width_cr [p_Vid->yuv_format]) / MB_BLOCK_SIZE;
  imgData->frm_stride[0]    = p_Vid->width;
  imgData->frm_stride[1] = imgData->frm_stride[2] = p_Vid->width_cr;
  imgData->top_stride[0] = imgData->bot_stride[0] = imgData->frm_stride[0] << 1;
  imgData->top_stride[1] = imgData->top_stride[2] = imgData->bot_stride[1] = imgData->bot_stride[2] = imgData->frm_stride[1] << 1;

  imgData->frm_data_buf[0][0] = imgData->frm_data_buf[0][1] = imgData->frm_data_buf[0][2] = NULL;
  imgData->frm_data_buf[1][0] = imgData->frm_data_buf[1][1] = imgData->frm_data_buf[1][2] = NULL;
  if( (p_Inp->separate_colour_plane_flag != 0) )
  {
    for( nplane=0; nplane<MAX_PLANE; nplane++ )
    {
      memory_size += get_mem2Dpel(&(imgData->frm_data[nplane]), p_Vid->height, p_Vid->width);
    }
  }
  else
  {
    //imgData->format = p_Inp->input_file1.format;
    memory_size += get_mem2Dpel(&(imgData->frm_data[0]), p_Vid->height, p_Vid->width);

    if (p_Vid->yuv_format != YUV400)
    {
      int i, j, k;
      memory_size += get_mem2Dpel(&(imgData->frm_data[1]), p_Vid->height_cr, p_Vid->width_cr);
      memory_size += get_mem2Dpel(&(imgData->frm_data[2]), p_Vid->height_cr, p_Vid->width_cr);

      if (sizeof(imgpel) == sizeof(unsigned char))
      {
        for (k = 1; k < 3; k++)
          memset(&(imgData->frm_data[k][0][0]), 128, p_Vid->height_cr * p_Vid->width_cr * sizeof(imgpel));
      }
      else
      {
        for (k = 1; k < 3; k++)
          for (j = 0; j < p_Vid->height_cr; j++)
            for (i = 0; i < p_Vid->width_cr; i++)
              imgData->frm_data[k][j][i] = 128;
      }
    }
  }

  if (!p_Vid->active_sps->frame_mbs_only_flag)
  {
    // allocate memory for field reference frame buffers
    memory_size += init_top_bot_planes(imgData->frm_data[0], p_Vid->height, &(imgData->top_data[0]), &(imgData->bot_data[0]));

    if (p_Vid->yuv_format != YUV400)
    {
      memory_size += 4*(sizeof(imgpel**));

      memory_size += init_top_bot_planes(imgData->frm_data[1], p_Vid->height_cr, &(imgData->top_data[1]), &(imgData->bot_data[1]));
      memory_size += init_top_bot_planes(imgData->frm_data[2], p_Vid->height_cr, &(imgData->top_data[2]), &(imgData->bot_data[2]));
    }
  }
  return memory_size;
}

/*!
 ************************************************************************
 * \brief
 *    Dynamic memory allocation of frame size related global buffers
 *    buffers are defined in global.h, allocated memory must be freed in
 *    void free_global_buffers()
 * \par Input:
 *    Input Parameters InputParameters *inp,                            \n
 *    Image Parameters VideoParameters *p_Vid
 * \return Number of allocated bytes
 ************************************************************************
 */
static int init_global_buffers(VideoParameters *p_Vid, InputParameters *p_Inp)
{
  int j, memory_size=0;

  if ((p_Vid->enc_frame_picture = (StorablePicture**)malloc(6 * sizeof(StorablePicture*))) == NULL)
    no_mem_exit("init_global_buffers: *p_Vid->enc_frame_picture");

  for (j = 0; j < 6; j++)
    p_Vid->enc_frame_picture[j] = NULL;

  if ((p_Vid->enc_field_picture = (StorablePicture**)malloc(2 * sizeof(StorablePicture*))) == NULL)
    no_mem_exit("init_global_buffers: *p_Vid->enc_field_picture");

  for (j = 0; j < 2; j++)
    p_Vid->enc_field_picture[j] = NULL;

  if ((p_Vid->frame_pic = (Picture**)malloc(p_Vid->frm_iter * sizeof(Picture*))) == NULL)
    no_mem_exit("init_global_buffers: *p_Vid->frame_pic");

  for (j = 0; j < p_Vid->frm_iter; j++)
    p_Vid->frame_pic[j] = malloc_picture();

  if (p_Inp->si_frame_indicator || p_Inp->sp_periodicity)
  {
    p_Vid->number_sp2_frames=0;
    p_Vid->frame_pic_si = malloc_picture();//picture buffer for the encoded SI picture
    //allocation of lrec and p_Vid->lrec_uv for SI picture
    get_mem2Dint (&p_Vid->lrec, p_Vid->height, p_Vid->width);
    get_mem3Dint (&p_Vid->lrec_uv, 2, p_Vid->height, p_Vid->width);
  }

#if (MVC_EXTENSION_ENABLE)
  p_Vid->field_pic_ptr = NULL;
  p_Vid->field_pic1    = NULL;
  p_Vid->field_pic2    = NULL;

  // Allocate memory for field picture coding
  if (p_Inp->PicInterlace != FRAME_CODING)
  {
    if ((p_Vid->field_pic1 = (Picture**)malloc(2 * sizeof(Picture*))) == NULL)
      no_mem_exit("init_global_buffers: *p_Vid->field_pic1");

    for (j = 0; j < 2; j++)
      p_Vid->field_pic1[j] = malloc_picture();

    if(p_Vid->num_of_layers==2)
    {
      if ((p_Vid->field_pic2 = (Picture**)malloc(2 * sizeof(Picture*))) == NULL)
        no_mem_exit("init_global_buffers: *p_Vid->field_pic2");

      for (j = 0; j < 2; j++)
        p_Vid->field_pic2[j] = malloc_picture();
    }
  }
#else
  // Allocate memory for field picture coding
  if (p_Inp->PicInterlace != FRAME_CODING)
  {
    if ((p_Vid->field_pic = (Picture**)malloc(2 * sizeof(Picture*))) == NULL)
      no_mem_exit("init_global_buffers: *p_Vid->field_pic");

    for (j = 0; j < 2; j++)
      p_Vid->field_pic[j] = malloc_picture();
  }
#endif

  // Init memory data for input & encoded images
  memory_size += init_orig_buffers(p_Vid, &p_Vid->imgData);

  if ( p_Inp->MDReference[0] || p_Inp->MDReference[1] )
  {
    memory_size += init_orig_buffers(p_Vid, &p_Vid->imgRefData);
  }

  memory_size += init_orig_buffers(p_Vid, &p_Vid->imgData0);

  if (p_Inp->enable_32_pulldown)
  {
    memory_size += init_orig_buffers(p_Vid, &p_Vid->imgData32);
    memory_size += init_orig_buffers(p_Vid, &p_Vid->imgData4);
    memory_size += init_orig_buffers(p_Vid, &p_Vid->imgData5);
    memory_size += init_orig_buffers(p_Vid, &p_Vid->imgData6);
  }


  //memory_size += get_mem2Dshort(&PicPos, p_Vid->FrameSizeInMbs + 1, 2);
  p_Vid->PicPos = calloc(p_Vid->FrameSizeInMbs + 1, sizeof(BlockPos));

  for (j = 0; j < (int) p_Vid->FrameSizeInMbs + 1; j++)
  {
    p_Vid->PicPos[j].x = (short) (j % p_Vid->PicWidthInMbs);
    p_Vid->PicPos[j].y = (short) (j / p_Vid->PicWidthInMbs);
  }


  if (p_Inp->rdopt == 3)
  {
    memory_size += allocate_errdo_mem(p_Vid, p_Inp);
  }

  if (p_Inp->RestrictRef)
  {
    memory_size += get_mem2D(&p_Vid->pixel_map,   p_Vid->height,   p_Vid->width);
    memory_size += get_mem2D(&p_Vid->refresh_map, p_Vid->height >> 3, p_Vid->width >> 3);
  }

  if (!p_Vid->active_sps->frame_mbs_only_flag)
  {
    for(j=0; j<p_Vid->num_of_layers; j++)
    {
      CodingParameters *cps = p_Vid->p_EncodePar[j];
      memory_size += get_mem2Dpel(&(p_Vid->imgY_com_buf[j]), cps->height, cps->width);
      if (cps->yuv_format != YUV400)
      {
        memory_size += get_mem2Dpel(&(p_Vid->imgUV_com_buf[j][0]), cps->height_cr, cps->width_cr);
        memory_size += get_mem2Dpel(&(p_Vid->imgUV_com_buf[j][1]), cps->height_cr, cps->width_cr);
      }
    }
    //set;
    p_Vid->imgY_com = p_Vid->imgY_com_buf[0];
    p_Vid->imgUV_com[0] = p_Vid->imgUV_com_buf[0][0];
    p_Vid->imgUV_com[1] = p_Vid->imgUV_com_buf[0][1];
  }

  // allocate and set memory relating to motion estimation
  if (!p_Inp->IntraProfile)
  {
    if (p_Inp->SearchMode[0] == UM_HEX || p_Inp->SearchMode[1] == UM_HEX)
    {
      if ((p_Vid->p_UMHex = (UMHexStruct*)calloc(1, sizeof(UMHexStruct))) == NULL)
        no_mem_exit("init_mv_block: p_Vid->p_UMHex");
      memory_size += UMHEX_get_mem(p_Vid, p_Inp);
    }
    if (p_Inp->SearchMode[0] == UM_HEX_SIMPLE || p_Inp->SearchMode[1] == UM_HEX_SIMPLE)
    {
      if ((p_Vid->p_UMHexSMP = (UMHexSMPStruct*)calloc(1, sizeof(UMHexSMPStruct))) == NULL)
        no_mem_exit("init_mv_block: p_Vid->p_UMHexSMP");

      smpUMHEX_init(p_Vid);
      memory_size += smpUMHEX_get_mem(p_Vid);
    }
    if (p_Inp->SearchMode[0] == EPZS || p_Inp->SearchMode[1] == EPZS)
    {
      memory_size += EPZSInit(p_Vid);
    }
  }

  if (p_Inp->RCEnable)
    rc_allocate_memory(p_Vid, p_Inp);

  if (p_Inp->redundant_pic_flag)
  {
    for(j=0; j<p_Vid->num_of_layers; j++)
    {
      CodingParameters *cps = p_Vid->p_EncodePar[j];
      memory_size += get_mem2Dpel(&p_Vid->imgY_tmp_buf[j], cps->height, cps->width);
      memory_size += get_mem2Dpel(&p_Vid->imgUV_tmp_buf[j][0], cps->height_cr, cps->width_cr);
      memory_size += get_mem2Dpel(&p_Vid->imgUV_tmp_buf[j][1], cps->height_cr, cps->width_cr);
    }
    p_Vid->imgY_tmp = p_Vid->imgY_tmp_buf[0];
    p_Vid->imgUV_tmp[0] = p_Vid->imgUV_tmp_buf[0][0];
    p_Vid->imgUV_tmp[1] = p_Vid->imgUV_tmp_buf[0][1];
  }

  memory_size += get_mem2Dint_pad (&p_Vid->imgY_sub_tmp, p_Vid->height, p_Vid->width, IMG_PAD_SIZE_Y, IMG_PAD_SIZE_X);

  //if ( p_Inp->ChromaMCBuffer )
    chroma_mc_setup(p_Vid);

  p_Vid->padded_size_x       = (p_Vid->width + 2 * IMG_PAD_SIZE_X);
  p_Vid->padded_size_x_m8x8  = (p_Vid->padded_size_x - BLOCK_SIZE_8x8);
  p_Vid->padded_size_x_m4x4  = (p_Vid->padded_size_x - BLOCK_SIZE);
  p_Vid->cr_padded_size_x    = (p_Vid->width_cr + 2 * p_Vid->pad_size_uv_x);
  p_Vid->cr_padded_size_x2   = (p_Vid->cr_padded_size_x << 1);
  p_Vid->cr_padded_size_x4   = (p_Vid->cr_padded_size_x << 2);
  p_Vid->cr_padded_size_x_m8 = (p_Vid->cr_padded_size_x - 8);

  // RGB images for distortion calculation
  // Recommended to do this allocation (and de-allocation) in
  // the appropriate file instead of here.
  if(p_Inp->DistortionYUVtoRGB)
  {
    memory_size += create_RGB_memory(p_Vid);
  }

  p_Vid->pWPX = NULL;
  if ( p_Inp->WPMCPrecision )
  {
    wpxInitWPXObject(p_Vid);
  }

  memory_size += init_process_image( p_Vid, p_Inp );

  p_Vid->p_pred = init_seq_structure( p_Vid, p_Inp, &memory_size );

  return memory_size;
}

/*!
 ************************************************************************
 * \brief
 *    Free allocated memory of original picture buffers
 ************************************************************************
 */
void free_orig_planes(VideoParameters *p_Vid, ImageData *imgData)
{
  if( (p_Vid->p_Inp->separate_colour_plane_flag != 0) )
  {
    int nplane;
    for( nplane=0; nplane<MAX_PLANE; nplane++ )
    {
      free_mem2Dpel(imgData->frm_data[nplane]);
      imgData->frm_data[nplane] = NULL;
    }
  }
  else
  {
      free_mem2Dpel(imgData->frm_data[0]);      // free ref frame buffers
      imgData->frm_data[0] = NULL;
      if (imgData->format.yuv_format != YUV400)
      {
        free_mem2Dpel(imgData->frm_data[1]);
        imgData->frm_data[1] = NULL;
        free_mem2Dpel(imgData->frm_data[2]);
        imgData->frm_data[2] = NULL;
      }
  }

  if (!p_Vid->active_sps->frame_mbs_only_flag)
  {
    free_top_bot_planes(imgData->top_data[0], imgData->bot_data[0]);
    if (imgData->format.yuv_format != YUV400)
    {
      free_top_bot_planes(imgData->top_data[1], imgData->bot_data[1]);
      free_top_bot_planes(imgData->top_data[2], imgData->bot_data[2]);
    }
    imgData->top_data[0] = imgData->bot_data[0] = NULL;
    imgData->top_data[1] = imgData->bot_data[1] = NULL;
    imgData->top_data[2] = imgData->bot_data[2] = NULL;
  }
}


/*!
 ************************************************************************
 * \brief
 *    Free allocated memory of frame size related global buffers
 *    buffers are defined in global.h, allocated memory is allocated in
 *    int get_mem4global_buffers()
 * \par Input:
 *    Input Parameters InputParameters *inp,                             \n
 *    Image Parameters VideoParameters *p_Vid
 * \par Output:
 *    none
 ************************************************************************
 */
static void free_global_buffers(VideoParameters *p_Vid, InputParameters *p_Inp)
{
  int  i,j;

  free_pointer (p_Vid->enc_frame_picture);

  if (p_Vid->frame_pic)
  {
    for (j = 0; j < p_Vid->frm_iter; j++)
    {
      if (p_Vid->frame_pic[j])
        free_picture (p_Vid->frame_pic[j]);
    }
    free_pointer (p_Vid->frame_pic);
  }

  free_pointer (p_Vid->enc_field_picture);

#if (MVC_EXTENSION_ENABLE)
  if (p_Vid->field_pic1)
  {
    for (j = 0; j < 2; j++)
    {
      if (p_Vid->field_pic1[j])
        free_picture (p_Vid->field_pic1[j]);
    }
    free_pointer (p_Vid->field_pic1);

    if(p_Vid->num_of_layers==2)
    {
      for (j = 0; j < 2; j++)
      {
        if (p_Vid->field_pic2[j])
          free_picture (p_Vid->field_pic2[j]);
      }
      free_pointer (p_Vid->field_pic2);
    }
  }
#else
  if (p_Vid->field_pic)
  {
    for (j = 0; j < 2; j++)
    {
      if (p_Vid->field_pic[j])
        free_picture (p_Vid->field_pic[j]);
    }
    free_pointer (p_Vid->field_pic);
  }
#endif

  // Deallocation of SI picture related memory
  if (p_Inp->si_frame_indicator || p_Inp->sp_periodicity)
  {
    free_picture (p_Vid->frame_pic_si);
    //deallocation of lrec and p_Vid->lrec_uv for SI frames
    free_mem2Dint (p_Vid->lrec);
    free_mem3Dint (p_Vid->lrec_uv);
  }

  free_orig_planes(p_Vid, &p_Vid->imgData);
  if ( p_Vid->p_Inp->MDReference[0] || p_Vid->p_Inp->MDReference[1] )
  {
    free_orig_planes(p_Vid, &p_Vid->imgRefData);
  }

  free_orig_planes(p_Vid, &p_Vid->imgData0);

  if (p_Inp->enable_32_pulldown)
  {
    free_orig_planes(p_Vid, &p_Vid->imgData32);
    free_orig_planes(p_Vid, &p_Vid->imgData4);
    free_orig_planes(p_Vid, &p_Vid->imgData5);
    free_orig_planes(p_Vid, &p_Vid->imgData6);
  }

  // free lookup memory which helps avoid divides with PicWidthInMbs
  //free_mem2Dshort(PicPos);
  free_pointer(p_Vid->PicPos);
  // Free Qmatrices and offsets
  free_QMatrix(p_Vid->p_Quant);
  free_QOffsets(p_Vid->p_Quant, p_Inp);


  if ( p_Inp->WPMCPrecision )
  {
    wpxFreeWPXObject(p_Vid);
  }

  if (p_Vid->imgY_sub_tmp) // free temp quarter pel frame buffers
  {
    free_mem2Dint_pad (p_Vid->imgY_sub_tmp, IMG_PAD_SIZE_Y, IMG_PAD_SIZE_X);
    p_Vid->imgY_sub_tmp = NULL;
  }

  // free mem, allocated in init_img()
  // free intra pred mode buffer for blocks
  free_mem2D((byte**)p_Vid->ipredmode);
  free_mem2D((byte**)p_Vid->ipredmode8x8);

  if(p_Vid->ipredmode4x4_line)
  {
    free_mem2D((byte**)p_Vid->ipredmode4x4_line);
    p_Vid->ipredmode4x4_line=NULL;
  }
  if(p_Vid->ipredmode8x8_line)
  {
    free_mem2D((byte **)p_Vid->ipredmode8x8_line);
    p_Vid->ipredmode8x8_line = NULL;
  }

  free_pointer( p_Vid->b8x8info );

  if( (p_Inp->separate_colour_plane_flag != 0) )
  {
    for( i=0; i<MAX_PLANE; i++ )
    {
      free_mbs(p_Vid->mb_data_JV[i], p_Vid->FrameSizeInMbs);
    }
  }
  else
  {
    //free_pointer(p_Vid->mb_data);
    free_mbs(p_Vid->mb_data, p_Vid->FrameSizeInMbs);
  }

  if(p_Inp->UseConstrainedIntraPred)
  {
    free_pointer (p_Vid->intra_block);
  }

  if (p_Inp->CtxAdptLagrangeMult == 1)
  {
    free_pointer(p_Vid->mb16x16_cost_frame);
  }

  if (p_Inp->rdopt == 3)
  {
    free_errdo_mem(p_Vid);
  }

  if (p_Inp->RestrictRef)
  {
    free_pointer(p_Vid->pixel_map[0]);
    free_pointer(p_Vid->pixel_map);
    free_pointer(p_Vid->refresh_map[0]);
    free_pointer(p_Vid->refresh_map);
  }

  if (!p_Vid->active_sps->frame_mbs_only_flag)
  {
    for(i=0; i<p_Vid->num_of_layers; i++)
    {
      free_mem2Dpel(p_Vid->imgY_com_buf[i]);
      free_mem2Dpel(p_Vid->imgUV_com_buf[i][0]);
      free_mem2Dpel(p_Vid->imgUV_com_buf[i][1]);
    }
    p_Vid->imgY_com = NULL;
    p_Vid->imgUV_com[0] = p_Vid->imgUV_com[1] = NULL;
  }

  if(p_Vid->num_of_layers>1)
  {
    int bEqual = p_Vid->nz_coeff_buf[0] == p_Vid->nz_coeff_buf[1];
    free_mem3Dint(p_Vid->nz_coeff_buf[0]);
    p_Vid->nz_coeff_buf[0] = NULL;
    if(!bEqual && p_Vid->nz_coeff_buf[1])
      free_mem3Dint(p_Vid->nz_coeff_buf[1]);
    p_Vid->nz_coeff_buf[1] = NULL;
  }
  else
    free_mem3Dint(p_Vid->nz_coeff_buf[0]);
  p_Vid->nz_coeff = NULL;

  free_mem2Dolm     (p_Vid->lambda_buf[0], p_Vid->bitdepth_luma_qp_scale);
  p_Vid->lambda = p_Vid->lambda_buf[0] = p_Vid->lambda_buf[1] = NULL;
  free_mem2Dodouble (p_Vid->lambda_md_buf[0], p_Vid->bitdepth_luma_qp_scale);
  p_Vid->lambda_md = p_Vid->lambda_md_buf[0] = p_Vid->lambda_md_buf[1] = NULL;
  free_mem3Dodouble (p_Vid->lambda_me_buf[0], 10, 52 + p_Vid->bitdepth_luma_qp_scale, p_Vid->bitdepth_luma_qp_scale);
  p_Vid->lambda_me = p_Vid->lambda_me_buf[0] = p_Vid->lambda_me_buf[1] = NULL;
  free_mem3Doint    (p_Vid->lambda_mf_buf[0], 10, 52 + p_Vid->bitdepth_luma_qp_scale, p_Vid->bitdepth_luma_qp_scale);
  p_Vid->lambda_mf = p_Vid->lambda_mf_buf[0] = p_Vid->lambda_mf_buf[1] = NULL;
  if ( p_Inp->UseRDOQuant )
  {
    free_mem2Dodouble (p_Vid->lambda_rdoq_buf[0], p_Vid->bitdepth_luma_qp_scale);
    p_Vid->lambda_rdoq = p_Vid->lambda_rdoq_buf[0] = p_Vid->lambda_rdoq_buf[1] = NULL;
  }
  if (p_Inp->CtxAdptLagrangeMult == 1)
  {
    free_mem2Dodouble(p_Vid->lambda_mf_factor_buf[0], p_Vid->bitdepth_luma_qp_scale);
    p_Vid->lambda_mf_factor = p_Vid->lambda_mf_factor_buf[0] = p_Vid->lambda_mf_factor_buf[1] = NULL;
  }

  if (!p_Inp->IntraProfile)
  {
    if (p_Inp->SearchMode[0] == UM_HEX || p_Inp->SearchMode[1] == UM_HEX)
    {
      UMHEX_free_mem(p_Vid, p_Inp);
    }
    if (p_Inp->SearchMode[0] == UM_HEX_SIMPLE || p_Inp->SearchMode[1] == UM_HEX_SIMPLE)
    {
      smpUMHEX_free_mem(p_Vid);
    }
    if (p_Inp->SearchMode[0] == EPZS || p_Inp->SearchMode[1] == EPZS)
    {
      EPZSDelete(p_Vid);
    }
  }

  if (p_Inp->RCEnable)
    rc_free_memory(p_Vid, p_Inp);

  if (p_Inp->redundant_pic_flag)
  {
    for(i=0; i<p_Vid->num_of_layers; i++)
    {
     free_mem2Dpel(p_Vid->imgY_tmp_buf[i]);
     free_mem2Dpel(p_Vid->imgUV_tmp_buf[i][0]);
     free_mem2Dpel(p_Vid->imgUV_tmp_buf[i][1]);
    }
    p_Vid->imgY_tmp = NULL;
    p_Vid->imgUV_tmp[0] = NULL;
    p_Vid->imgUV_tmp[1] = NULL;
  }

  // Again process should be moved into cconv_yuv2rgb.c file for cleanliness
  // These should not be globals but instead only be visible through that code.
  if(p_Inp->DistortionYUVtoRGB)
  {
    delete_RGB_memory(p_Vid);
  }

  clear_process_image( p_Vid, p_Inp );
  free_seq_structure( p_Vid->p_pred );
}


/*!
 ************************************************************************
 * \brief
 *    Allocate memory for AC coefficients
 ************************************************************************
 */
int get_mem_ACcoeff (VideoParameters *p_Vid, int***** cofAC)
{
  int num_blk8x8 = BLOCK_SIZE + p_Vid->num_blk8x8_uv;

  get_mem4Dint(cofAC, num_blk8x8, BLOCK_SIZE, 2, 65);

  return num_blk8x8 * BLOCK_SIZE * 2 * 65 * sizeof(int);// 18->65 for ABT
}

/*!
 ************************************************************************
 * \brief
 *    Allocate memory for AC coefficients
 ************************************************************************
 */
int get_mem_ACcoeff_new (int****** cofAC, int chroma)
{
  get_mem5Dint(cofAC, BLOCK_SIZE, chroma, BLOCK_SIZE, 2, 65);
  return chroma * BLOCK_PIXELS * 2 * 65 * sizeof(int);// 18->65 for ABT
}

/*!
 ************************************************************************
 * \brief
 *    Allocate memory for DC coefficients
 ************************************************************************
 */
int get_mem_DCcoeff (int**** cofDC)
{
  get_mem3Dint(cofDC, 3, 2, 18);
  return 3 * 2 * 18 * sizeof(int);
}


/*!
 ************************************************************************
 * \brief
 *    Free memory of AC coefficients
 ************************************************************************
 */
void free_mem_ACcoeff (int**** cofAC)
{
  free_mem4Dint(cofAC);
}

/*!
 ************************************************************************
 * \brief
 *    Free memory of AC coefficients
 ************************************************************************
 */
void free_mem_ACcoeff_new (int***** cofAC)
{
  free_mem5Dint(cofAC);
}

/*!
 ************************************************************************
 * \brief
 *    Free memory of DC coefficients
 ************************************************************************
 */
void free_mem_DCcoeff (int*** cofDC)
{
  free_mem3Dint(cofDC);
}

/*!
 ************************************************************************
 * \brief
 *    Sets indices to appropriate level constraints, depending on
 *    current level_idc
 ************************************************************************
 */
static void SetLevelIndices(VideoParameters *p_Vid)
{
  switch(p_Vid->active_sps->level_idc)
  {
  case 9:
    p_Vid->LevelIndex=1;
    break;
  case 10:
    p_Vid->LevelIndex=0;
    break;
  case 11:
    if (!is_FREXT_profile(p_Vid->active_sps->profile_idc) && (p_Vid->active_sps->constrained_set3_flag == 0))
      p_Vid->LevelIndex=2;
    else
      p_Vid->LevelIndex=1;
    break;
  case 12:
    p_Vid->LevelIndex=3;
    break;
  case 13:
    p_Vid->LevelIndex=4;
    break;
  case 20:
    p_Vid->LevelIndex=5;
    break;
  case 21:
    p_Vid->LevelIndex=6;
    break;
  case 22:
    p_Vid->LevelIndex=7;
    break;
  case 30:
    p_Vid->LevelIndex=8;
    break;
  case 31:
    p_Vid->LevelIndex=9;
    break;
  case 32:
    p_Vid->LevelIndex=10;
    break;
  case 40:
    p_Vid->LevelIndex=11;
    break;
  case 41:
    p_Vid->LevelIndex=12;
    break;
  case 42:
    if (!is_FREXT_profile(p_Vid->active_sps->profile_idc))
      p_Vid->LevelIndex=13;
    else
      p_Vid->LevelIndex=14;
    break;
  case 50:
    p_Vid->LevelIndex=15;
    break;
  case 51:
    p_Vid->LevelIndex=16;
    break;
  default:
    fprintf ( stderr, "Warning: unknown LevelIDC, using maximum level 5.1 \n" );
    p_Vid->LevelIndex=16;
    break;
  }
}

/*!
 ************************************************************************
 * \brief
 *    initialize key frames and corresponding redundant frames.
 ************************************************************************
 */
void init_redundant_frame(VideoParameters *p_Vid, InputParameters *p_Inp)
{
  if (p_Inp->redundant_pic_flag)
  {
    if (p_Inp->NumberBFrames)
    {
      error("B frame not supported when redundant picture used!", 100);
    }

    if (p_Inp->PicInterlace)
    {
      error("Interlace not supported when redundant picture used!", 100);
    }

    if (p_Inp->num_ref_frames < p_Inp->PrimaryGOPLength)
    {
      error("NumberReferenceFrames must be no less than PrimaryGOPLength", 100);
    }

    if ((1<<p_Inp->NumRedundantHierarchy) > p_Inp->PrimaryGOPLength)
    {
      error("PrimaryGOPLength must be greater than 2^NumRedundantHeirarchy", 100);
    }

    if (p_Inp->Verbose != 1)
    {
      error("Redundant slices not supported when Verbose != 1", 100);
    }
  }

  p_Vid->key_frame = 0;
  p_Vid->redundant_coding = 0;
  p_Vid->redundant_pic_cnt = 0;
  p_Vid->frameNuminGOP = p_Vid->curr_frm_idx % p_Inp->PrimaryGOPLength;
  if (p_Vid->curr_frm_idx == 0)
  {
    p_Vid->frameNuminGOP = -1;
  }
}

/*!
 ************************************************************************
 * \brief
 *    allocate redundant frames in a primary GOP.
 ************************************************************************
 */
void set_redundant_frame(VideoParameters *p_Vid, InputParameters *p_Inp)
{
  int GOPlength = p_Inp->PrimaryGOPLength;

  //start frame of GOP
  if (p_Vid->frameNuminGOP == 0)
  {
    p_Vid->redundant_coding = 0;
    p_Vid->key_frame = 1;
    p_Vid->redundant_ref_idx = GOPlength;
  }

  //1/2 position
  if (p_Inp->NumRedundantHierarchy > 0)
  {
    if (p_Vid->frameNuminGOP == GOPlength >> 1)
    {
      p_Vid->redundant_coding = 0;
      p_Vid->key_frame = 1;
      p_Vid->redundant_ref_idx = GOPlength >> 1;
    }
  }

  //1/4, 3/4 position
  if (p_Inp->NumRedundantHierarchy > 1)
  {
    if (p_Vid->frameNuminGOP == (GOPlength >> 2) || p_Vid->frameNuminGOP == ((GOPlength*3) >> 2))
    {
      p_Vid->redundant_coding = 0;
      p_Vid->key_frame = 1;
      p_Vid->redundant_ref_idx = GOPlength >> 2;
    }
  }

  //1/8, 3/8, 5/8, 7/8 position
  if (p_Inp->NumRedundantHierarchy > 2)
  {
    if (p_Vid->frameNuminGOP == GOPlength >> 3 || p_Vid->frameNuminGOP == ((GOPlength*3) >> 3)
      || p_Vid->frameNuminGOP == ((GOPlength*5) >> 3) || p_Vid->frameNuminGOP == ((GOPlength*7) & 0x03))
    {
      p_Vid->redundant_coding = 0;
      p_Vid->key_frame = 1;
      p_Vid->redundant_ref_idx = GOPlength >> 3;
    }
  }

  //1/16, 3/16, 5/16, 7/16, 9/16, 11/16, 13/16 position
  if (p_Inp->NumRedundantHierarchy > 3)
  {
    if (p_Vid->frameNuminGOP == (GOPlength >> 4) || p_Vid->frameNuminGOP == ((GOPlength*3) >> 4)
      || p_Vid->frameNuminGOP == ((GOPlength*5) >> 4) || p_Vid->frameNuminGOP == ((GOPlength*7) >> 4)
      || p_Vid->frameNuminGOP == ((GOPlength*9) >> 4) || p_Vid->frameNuminGOP == ((GOPlength*11) >> 4)
      || p_Vid->frameNuminGOP == ((GOPlength*13) >> 4))
    {
      p_Vid->redundant_coding = 0;
      p_Vid->key_frame = 1;
      p_Vid->redundant_ref_idx = GOPlength >> 4;
    }
  }
}

/*!
 ************************************************************************
 * \brief
 *    encode one redundant frame.
 ************************************************************************
 */
void encode_one_redundant_frame(VideoParameters *p_Vid, InputParameters *p_Inp)
{
  p_Vid->key_frame = 0;
  p_Vid->redundant_coding = 1;
  p_Vid->redundant_pic_cnt = 1;

  if (!p_Vid->currentPicture->idr_flag)
  {
    if (p_Vid->type == I_SLICE)
    {
      set_slice_type( p_Vid, p_Inp, P_SLICE );
    }
  }

  encode_one_frame(p_Vid, p_Inp);
}

/*!
 ************************************************************************
 * \brief
 *    Setup Chroma MC Variables
 ************************************************************************
 */
static void chroma_mc_setup(VideoParameters *p_Vid)
{
  // initialize global variables used for chroma interpolation and buffering
  if ( p_Vid->yuv_format == YUV420 )
  {
    p_Vid->pad_size_uv_x = IMG_PAD_SIZE_X >> 1;
    p_Vid->pad_size_uv_y = IMG_PAD_SIZE_Y >> 1;
    p_Vid->chroma_mask_mv_y = 7;
    p_Vid->chroma_mask_mv_x = 7;
    p_Vid->chroma_shift_x = 3;
    p_Vid->chroma_shift_y = 3;
  }
  else if ( p_Vid->yuv_format == YUV422 )
  {
    p_Vid->pad_size_uv_x = IMG_PAD_SIZE_X >> 1;
    p_Vid->pad_size_uv_y = IMG_PAD_SIZE_Y;
    p_Vid->chroma_mask_mv_y = 3;
    p_Vid->chroma_mask_mv_x = 7;
    p_Vid->chroma_shift_y = 2;
    p_Vid->chroma_shift_x = 3;
  }
  else
  { // YUV444
    p_Vid->pad_size_uv_x = IMG_PAD_SIZE_X;
    p_Vid->pad_size_uv_y = IMG_PAD_SIZE_Y;
    p_Vid->chroma_mask_mv_y = 3;
    p_Vid->chroma_mask_mv_x = 3;
    p_Vid->chroma_shift_y = 2;
    p_Vid->chroma_shift_x = 2;
  }
  p_Vid->shift_cr_y  = p_Vid->chroma_shift_y - 2;
  p_Vid->shift_cr_x  = p_Vid->chroma_shift_x - 2;
}


static void gen_enc_par(VideoParameters *p_Vid, int layer_idx)
{
  CodingParameters *cps;
  DecodedPictureBuffer *p_Dpb;
  InputParameters *p_Inp = p_Vid->p_Inp;

  cps = p_Vid->p_EncodePar[layer_idx];
  p_Dpb = p_Vid->p_Dpb_layer[layer_idx];

  cps->layer_id = layer_idx;
  cps->yuv_format = p_Dpb->storage_format.yuv_format;
  cps->P444_joined = (cps->yuv_format == YUV444 && (p_Inp->separate_colour_plane_flag == 0));
  cps->bitdepth_luma            = (short)p_Dpb->storage_format.bit_depth[0];
  cps->bitdepth_scale[0]        = 1 << (cps->bitdepth_luma - 8);
  cps->bitdepth_lambda_scale    = 2 * (cps->bitdepth_luma - 8);
  cps->bitdepth_luma_qp_scale   = 3 *  cps->bitdepth_lambda_scale;
  cps->dc_pred_value_comp[0]    =  (uint32) (1<<(cps->bitdepth_luma - 1));
  cps->max_pel_value_comp[0] = (1<<cps->bitdepth_luma) - 1;
  cps->max_imgpel_value_comp_sq[0] = cps->max_pel_value_comp[0] * cps->max_pel_value_comp[0];
  cps->mb_size[0][0]            = cps->mb_size[0][1] = MB_BLOCK_SIZE;
  if (cps->yuv_format != YUV400)
  {
    cps->bitdepth_chroma             = (short)p_Dpb->storage_format.bit_depth[1];
    cps->bitdepth_scale[1]           = 1 << (cps->bitdepth_chroma - 8);
    cps->dc_pred_value_comp[1]       = (uint32) (1<<(cps->bitdepth_chroma - 1));
    cps->dc_pred_value_comp[2]       = cps->dc_pred_value_comp[1];
    cps->max_pel_value_comp[1]       = (1<<cps->bitdepth_chroma) - 1;
    cps->max_pel_value_comp[2]       = cps->max_pel_value_comp[1];
    cps->max_imgpel_value_comp_sq[1] = cps->max_pel_value_comp[1] * cps->max_pel_value_comp[1];
    cps->max_imgpel_value_comp_sq[2] = cps->max_pel_value_comp[2] * cps->max_pel_value_comp[2];
    cps->num_blk8x8_uv               = (1<<cps->yuv_format)&(~(0x1));
    cps->num_cdc_coeff               = cps->num_blk8x8_uv << 1;
    cps->mb_size[1][0] = cps->mb_size[2][0] = cps->mb_cr_size_x = (cps->yuv_format == YUV420 || cps->yuv_format == YUV422) ? 8 : 16;
    cps->mb_size[1][1] = cps->mb_size[2][1] = cps->mb_cr_size_y = (cps->yuv_format == YUV444 || cps->yuv_format == YUV422) ? 16 : 8;
    cps->bitdepth_chroma_qp_scale = 6*(cps->bitdepth_chroma - 8);
  }
  else
  {
    cps->bitdepth_chroma     = 0;
    cps->bitdepth_scale[1]   = 0;
    cps->max_pel_value_comp[1] = 0;
    cps->max_pel_value_comp[2] = cps->max_pel_value_comp[1];
    cps->max_imgpel_value_comp_sq[1] = cps->max_pel_value_comp[1] * cps->max_pel_value_comp[1];
    cps->max_imgpel_value_comp_sq[2] = cps->max_pel_value_comp[2] * cps->max_pel_value_comp[2];
    cps->num_blk8x8_uv       = 0;
    cps->num_cdc_coeff       = 0;
    cps->mb_size[1][0] = cps->mb_size[2][0] = cps->mb_cr_size_x = 0;
    cps->mb_size[1][1] = cps->mb_size[2][1] = cps->mb_cr_size_y = 0;

    cps->bitdepth_chroma_qp_scale = 0;
  }
  cps->max_bitCount =  128 + 256 * cps->bitdepth_luma + 2 * cps->mb_cr_size_y * cps->mb_cr_size_x * cps->bitdepth_chroma;
  cps->width         = (p_Inp->output.width[0]  + p_Vid->auto_crop_right);
  cps->height        = (p_Inp->output.height[0] + p_Vid->auto_crop_bottom);
  cps->width_blk     = cps->width  / BLOCK_SIZE;
  cps->height_blk    = cps->height / BLOCK_SIZE;
  cps->width_padded  = cps->width  + 2 * IMG_PAD_SIZE_X;
  cps->height_padded = cps->height + 2 * IMG_PAD_SIZE_Y;

  if (cps->yuv_format != YUV400)
  {
    cps->width_cr = cps->width  * mb_width_cr [cps->yuv_format] / 16;
    cps->height_cr= cps->height * mb_height_cr[cps->yuv_format] / 16;
  }
  else
  {
    cps->width_cr = 0;
    cps->height_cr= 0;
  }

  cps->height_cr_frame = cps->height_cr;
  cps->size = cps->width * cps->height;
  cps->size_cr = cps->width_cr * cps->height_cr;

  cps->PicWidthInMbs    = cps->width  / MB_BLOCK_SIZE;
  cps->FrameHeightInMbs = cps->height / MB_BLOCK_SIZE;
  cps->FrameSizeInMbs   = cps->PicWidthInMbs * cps->FrameHeightInMbs;
  cps->PicHeightInMapUnits = ( p_Vid->sps[layer_idx]->frame_mbs_only_flag ? cps->FrameHeightInMbs : cps->FrameHeightInMbs >> 1 );

  if ( cps->yuv_format == YUV420 )
  {
    cps->pad_size_uv_x = IMG_PAD_SIZE_X >> 1;
    cps->pad_size_uv_y = IMG_PAD_SIZE_Y >> 1;
    cps->chroma_mask_mv_y = 7;
    cps->chroma_mask_mv_x = 7;
    cps->chroma_shift_x = 3;
    cps->chroma_shift_y = 3;
  }
  else if ( cps->yuv_format == YUV422 )
  {
    cps->pad_size_uv_x = IMG_PAD_SIZE_X >> 1;
    cps->pad_size_uv_y = IMG_PAD_SIZE_Y;
    cps->chroma_mask_mv_y = 3;
    cps->chroma_mask_mv_x = 7;
    cps->chroma_shift_y = 2;
    cps->chroma_shift_x = 3;
  }
  else
  { // YUV444
    cps->pad_size_uv_x = IMG_PAD_SIZE_X;
    cps->pad_size_uv_y = IMG_PAD_SIZE_Y;
    cps->chroma_mask_mv_y = 3;
    cps->chroma_mask_mv_x = 3;
    cps->chroma_shift_y = 2;
    cps->chroma_shift_x = 2;
  }
  cps->shift_cr_y  = cps->chroma_shift_y - 2;
  cps->shift_cr_x  = cps->chroma_shift_x - 2;

  cps->padded_size_x       = (cps->width + 2 * IMG_PAD_SIZE_X);
  cps->padded_size_x_m8x8  = (cps->padded_size_x - BLOCK_SIZE_8x8);
  cps->padded_size_x_m4x4  = (cps->padded_size_x - BLOCK_SIZE);
  cps->cr_padded_size_x    = (cps->width_cr + 2 * cps->pad_size_uv_x);
  cps->cr_padded_size_x2   = (cps->cr_padded_size_x << 1);
  cps->cr_padded_size_x4   = (cps->cr_padded_size_x << 2);
  cps->cr_padded_size_x_m8 = (cps->cr_padded_size_x - 8);

}

static void generate_encode_parameters(VideoParameters *p_Vid)
{
  int i;
  p_Vid->p_EncodePar[0] = (CodingParameters *)calloc(p_Vid->num_of_layers, sizeof(CodingParameters));
  //init;
  for(i=0; i<p_Vid->num_of_layers; i++)
  {
    p_Vid->p_EncodePar[i] = p_Vid->p_EncodePar[0]+i;
    gen_enc_par(p_Vid, i);
  }
  p_Vid->p_CurrEncodePar = p_Vid->p_EncodePar[0];
}

static void free_encode_parameters(VideoParameters *p_Vid)
{
  int i;
  if(p_Vid->p_EncodePar[0])
  {
    free_pointer(p_Vid->p_EncodePar[0]);
  }
  for(i=0; i<p_Vid->num_of_layers; i++)
  {
    p_Vid->p_EncodePar[i] = NULL;
  }
  p_Vid->p_CurrEncodePar = NULL;
}

int init_process_image( VideoParameters *p_Vid, InputParameters *p_Inp)
{
  int memory_size = 0;
  switch( p_Inp->ProcessInput )
  {
  default:
    break;
  case 0:
    if( (p_Vid->num_of_layers == 2) && is_MVC_profile(p_Inp->ProfileIDC) )
      memory_size += init_orig_buffers(p_Vid, &p_Vid->tempData3);
    break;
  }
  return memory_size;
}

void clear_process_image( VideoParameters *p_Vid, InputParameters *p_Inp)
{
  switch( p_Inp->ProcessInput )
  {
  default:
    break;
  case 0:
    if( (p_Vid->num_of_layers == 2) && is_MVC_profile(p_Inp->ProfileIDC) )
      free_orig_planes(p_Vid, &p_Vid->tempData3);
    break;
  }
}
