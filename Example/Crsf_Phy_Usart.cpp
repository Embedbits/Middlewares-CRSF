/**
 * \author Mr.Nobody
 * \file Crsf_Phy_Usart.cpp
 * \ingroup Crsf
 * \brief Example CRSF USART Physical Layer (PL) over Crsf_Hal.h.
 */

/* ============================= INCLUDES =================================== */
#include "Crsf_Phy_Usart.hpp"               /* Self include                   */
#include "Crsf_Hal.h"                       /* Hardware interface             */
/* ======================== EXPORTED FUNCTIONS ============================== */

void Crsf::UsartPhysicalLayer::Set_Transmission_Start( const uint8_t *msgBuffer, crsf_MsgLength_t msgLen )
{
    Crsf_Hal_StartTransmit( msgBuffer, msgLen );
}


crsf_TxStatus_t Crsf::UsartPhysicalLayer::Get_Transmission_Status( void )
{
    return ( Crsf_Hal_Get_TxStatus() );
}


crsf_MsgLength_t Crsf::UsartPhysicalLayer::Get_RxData( uint8_t *buffer, crsf_MsgLength_t maxLen )
{
    return ( Crsf_Hal_Read( buffer, maxLen ) );
}


void Crsf::UsartPhysicalLayer::Set_Baudrate( crsf_Baudrate_t baudrate )
{
    busBaudrate = baudrate;
}


void Crsf::UsartPhysicalLayer::PhyInit( void )
{
    Crsf_Hal_Init( busBaudrate );
}
