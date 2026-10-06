/**
 * \author Mr.Nobody
 * \file Crsf_Phy_Usart.hpp
 * \ingroup Crsf
 * \brief Example CRSF USART Physical Layer (PL) header file.
 */

#ifndef CRSF_USART_PL_HPP
#define CRSF_USART_PL_HPP
/* ============================= INCLUDES =================================== */
#include "Crsf_Phy.hpp"                     /* Physical layer interface       */
#include "Crsf_Types.h"                     /* Module C types definition      */
/* ============================= TYPEDEFS =================================== */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* ========================= EXPORTED MACROS ================================ */

/* ========================= EXPORTED VARIABLES ============================= */

/* ======================== EXPORTED FUNCTIONS ============================== */

namespace Crsf
{
    class UsartPhysicalLayer : public PhysicalLayer
    {
    public:
        UsartPhysicalLayer( void ) = default;

        void             Set_Transmission_Start  ( const uint8_t *msgBuffer, crsf_MsgLength_t msgLen ) override;
        crsf_TxStatus_t  Get_Transmission_Status ( void )                                           override;
        crsf_MsgLength_t Get_RxData              ( uint8_t *buffer, crsf_MsgLength_t maxLen )       override;
        void             Set_Baudrate            ( crsf_Baudrate_t baudrate )                       override;
        void             PhyInit                 ( void )                                           override;

    private:

        /** Baudrate applied by PhyInit */
        crsf_Baudrate_t busBaudrate = baudrateFullDuplex;
    };
} /* namespace Crsf */

#endif /* CRSF_USART_PL_HPP */
