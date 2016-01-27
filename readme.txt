This repository contains source code used in the following paper.
D. Zhang, J. Liang, and I. Singh, "Fast Transmission Distortion Estimation and Adaptive Error Protection for H.264/AVC-based Embedded Video Conferencing Systems," Signal Processing: Image Communication, Volume 28, Issue 5, May 2013, pp. 417–429.

This readme file provides a summary on the repository directory structures and code functionalities.

I.Synthesized view distortion estimation with random depth error:
SynthDisEstError: Contains distortion estimation(SynthDisEstError\estimation) and verification(SynthDisEstError\verification) code for estimating synthesized view distortion given uniform depth errors.
I.a Estimation:
SynthDisEstError\estimation\VSRS3_5_DE_1D_uniform: contains distortion estimation code for the 1D parallel method.
SynthDisEstError\estimation\VSRS3_5_DE_GEN_uniform: contains distortion estimation code for the general method.
I.b Verification:
SynthDisEstError\verification\VSRS3_5_CORRUPT_DEPTH_child_uniform: VSRS version 3.5 with uniform random error added to the depth images. This is the child process used by “corrupt_vs”.
SynthDisEstError\verification\corrupt_vs: This is the master controller for the verification code. It uses the binary from “VSRS3_5_CORRUPT_DEPTH_child_uniform” to perform view synthesis for a specified number of iterations and then calculates the average distortion to compare with the estimation results.

II.Synthesized view distortion estimation with packet loss in depth and texture bitstreams:
SynthDisEstLoss: Contains distortion estimation(SynthDisEstLoss\estimation) and verification(SynthDisEstLoss\verification) code for estimating synthesized view distortion given random packet loss.
II.a Estimation:
SynthDisEstLoss\estimation\full_system\jm18_RODE_signaled_lowMem: Child process which uses RODE to calculate pixel level PMF.
SynthDisEstLoss\estimation\full_system\VSRS3_5_DE_1D_signaled:  Child process which receives pixel level PMF from RODE calculation and calculate the distortion in the synthesized view.
SynthDisEstLoss\estimation\full_system\oldesv: Master controller which relays estimated PMF from “jm18_RODE_signaled_lowMem” to “VSRS3_5_DE_1D_signaled” so that the synthesized virtual view distortion can be calculated.
SynthDisEstLoss\estimation\RODE_only\jm18_RODE_standalone_lowMem: This modified JM reference code contains RODE algorithm and is used to estimate single video stream packet loss distortion.
II.b Verification:
SynthDisEstLoss\verification\jm18.0_slice_loss: JM reference code which contains modifications to allow slice loss. Error concealment algorithm is the simple last frame copy algorithm.
SynthDisEstLoss\verification\RODE_only: Master controller which introduces packet loss in the encoded texture video stream and uses “jm18.0_slice_loss” to decode the lossy bitstream. This process is repeated for a specific number of iterations. Average PSNR is collected at the frame level to compare with estimation results produced by RODE.
SynthDisEstLoss\verification\full_system: Master controller which introduces packet loss in the encoded texture and depth streams. “jm18.0_slice_loss” is used to decode the corrupted bistreams and then VSRS is used for view synthesis. This process is repeated for a specified number of iterations. Average PSNR for the virtual view is collected at the frame level to compare with estimation results produced by “oldesv”.



