/* ----------------------------------------------------------------------
** Include Files
** ------------------------------------------------------------------- */
#include "nrf52.h"
#include "HRZ_ADS1298_LP_Filter.h"
#include "HRZ_ecg_service.h"

/* ----------------------------------------------------------------------
** Macro Defines
** ------------------------------------------------------------------- */
#define TEST_LENGTH_SAMPLES   HRZ_SAMPLES_PER_PACKET
#define BLOCK_SIZE            HRZ_SAMPLES_PER_PACKET
#define NUM_TAPS              27

/* -------------------------------------------------------------------
* Declare State buffer of size (numTaps + blockSize - 1)
* ------------------------------------------------------------------- */
static 		q31_t firStateQ32[BLOCK_SIZE + NUM_TAPS - 1];
/* ----------------------------------------------------------------------
** FIR Coefficients buffer generated using fir1() MATLAB function.
** fir1(28, 6/24)
** ------------------------------------------------------------------- */
const q31_t firCoeffs32[NUM_TAPS] = {
  -11691521, -65429926, -105240240, -107318647, -41092891, 54178704, 96795372,
  31752233, -97735092, -158738078, -30437798, 278333498, 606020039, 746383407,
  606020039, 278333498, -30437798, -158738078, -97735092, 31752233, 96795372,
  54178704, -41092891, -107318647, -105240240, -65429926, -11691521
};
/* ------------------------------------------------------------------
* Global variables for FIR LPF Example
* ------------------------------------------------------------------- */
uint32_t blockSize = BLOCK_SIZE;
uint32_t numBlocks = TEST_LENGTH_SAMPLES/BLOCK_SIZE;
/* ----------------------------------------------------------------------
* FIR LPF Example
* ------------------------------------------------------------------- */
arm_status hrz_ads1298_filter_data(q31_t *inputF32, q31_t *outputF32)
{
  uint32_t i;
  arm_fir_instance_q31 S;

  /* Call FIR init function to initialize the instance structure. */
  arm_fir_init_q31(&S, NUM_TAPS, (q31_t *)&firCoeffs32[0], &firStateQ32[0], blockSize);
  /* ----------------------------------------------------------------------
  ** Call the FIR process function for every blockSize samples
  ** ------------------------------------------------------------------- */
  for(i=0; i < numBlocks; i++)
  {
    arm_fir_q31(&S, inputF32 + (i * blockSize), outputF32 + (i * blockSize), blockSize);
  }

  return ARM_MATH_SUCCESS;
}
