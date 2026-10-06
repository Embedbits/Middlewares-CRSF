/**
 * \author Mr.Nobody
 * \file Crsf_Hal.h
 * \ingroup Crsf
 * \brief Hardware interface used by example USART physical layer.
 *
 * Functions are implemented by the application (BSP), typically over USART
 * with DMA:
 * - Reception runs continuously into circular DMA buffer, received bytes are
 *   read by \ref Crsf_Hal_Read (non-blocking).
 * - Full duplex UART 416666 Bd 8N1 (flying platform side) or single wire
 *   half duplex 400000 Bd (RC - TX module; implementation switches the line
 *   direction and discards echo of own bytes).
 */

#ifndef CRSF_EXAMPLE_CRSF_HAL_H
#define CRSF_EXAMPLE_CRSF_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================= INCLUDES =================================== */
#include "Crsf_Types.h"                     /* Module types definition        */
/* ============================= TYPEDEFS =================================== */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* ========================= EXPORTED MACROS ================================ */

/* ========================= EXPORTED VARIABLES ============================= */

/* ======================== EXPORTED FUNCTIONS ============================== */

/** Initializes peripheral with requested baudrate (8N1) and starts continuous reception */
void            Crsf_Hal_Init           ( crsf_Baudrate_t baudrate );

/** Starts transmission of buffer (buffer stays valid until transmission is finished) */
void            Crsf_Hal_StartTransmit  ( const uint8_t *buffer, crsf_MsgLength_t length );

/** Returns CRSF_TX_FINISHED when the last byte (incl. stop bit) was sent */
crsf_TxStatus_t Crsf_Hal_Get_TxStatus   ( void );

/** Copies at most maxLength received bytes into buffer, returns count of copied bytes */
crsf_MsgLength_t Crsf_Hal_Read          ( uint8_t *buffer, crsf_MsgLength_t maxLength );

#ifdef __cplusplus
}
#endif

#endif /* CRSF_EXAMPLE_CRSF_HAL_H */
