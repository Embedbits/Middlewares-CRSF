/**
 * \defgroup Crsf Crsf
 * \brief Crsf module
 */

/**
 * \author Mr.Nobody
 * \file Crsf_Example.h
 * \ingroup Crsf
 * \brief Example of CRSF integration on flight controller side (C interface).
 */

#ifndef CRSF_EXAMPLE_CRSF_EXAMPLE_H
#define CRSF_EXAMPLE_CRSF_EXAMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================= INCLUDES =================================== */
#include "Crsf_Types.h"                     /* Module types definition        */
/* ======================== EXPORTED FUNCTIONS ============================== */

/** Configures CRSF stack (call once before the first task) */
void             CrsfExample_Init           ( void );

/** Cyclic task, recommended period 1 ms */
void             CrsfExample_Task           ( crsf_Time_t taskTimeMs );

/** Returns state of RC link (CRSF_LINK_LOST = failsafe) */
crsf_LinkState_t CrsfExample_Get_LinkState  ( void );

/** Returns pulse width of channel 0 - 15 in us */
int32_t          CrsfExample_Get_ChannelUs  ( uint8_t channelIndex );

/** Updates battery telemetry (voltage and current in 0.1 V / 0.1 A, capacity in mAh) */
void             CrsfExample_Set_Battery    ( int16_t voltageDv, int16_t currentDa, uint32_t capacityMah, uint8_t remainingPct );

#ifdef __cplusplus
}
#endif

#endif /* CRSF_EXAMPLE_CRSF_EXAMPLE_H */
