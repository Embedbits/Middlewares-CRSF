/**
 * \author Mr.Nobody
 * \file Crsf_Phy.hpp
 * \ingroup Crsf
 * \brief CRSF Physical Layer (PL) interface.
 *
 * Physical layer is implemented by user (see Example/Crsf_Phy_Usart.cpp) and
 * it is used by transport layer. Transport layer works only by polling - the
 * physical layer does not need any callback into the module.
 *
 * Expected behavior of implementation:
 * - Reception runs continuously after \ref PhyInit (eg. USART with circular
 *   DMA buffer). CRSF is a byte stream - frames are found by the transport
 *   layer, the physical layer does not need to detect frame boundaries.
 * - \ref Get_RxData copies bytes received since the last call (not more than
 *   requested) and returns their count. It never blocks.
 * - Transmission of buffer is started by \ref Set_Transmission_Start. After the
 *   last byte is physically sent \ref Get_Transmission_Status returns
 *   \ref CRSF_TX_FINISHED.
 * - Single wire half duplex: the implementation switches the line direction
 *   and discards echo of own transmitted bytes.
 */

#ifndef CRSF_PHY_HPP
#define CRSF_PHY_HPP
/* ============================= INCLUDES =================================== */
#include "Crsf_Types.h"                     /* Module C types definition      */
/* ============================= TYPEDEFS =================================== */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* ========================= EXPORTED MACROS ================================ */

/* ========================= EXPORTED VARIABLES ============================= */

/* ======================== EXPORTED FUNCTIONS ============================== */

namespace Crsf
{
    class PhysicalLayer
    {
    public:

        virtual ~PhysicalLayer() = default;

        /**
         * \brief Starts transmission of frame.
         *
         * \param msgBuffer [in]: Frame to be transmitted (sync byte ... CRC).
         *                        Buffer stays valid until transmission is finished.
         * \param msgLen    [in]: Count of bytes in frame
         */
        virtual void            Set_Transmission_Start  ( const uint8_t *msgBuffer,
                                                          crsf_MsgLength_t msgLen )    = 0;
        /** \brief Returns status of transmission started by \ref Set_Transmission_Start */
        virtual crsf_TxStatus_t Get_Transmission_Status ( void )                       = 0;

        /**
         * \brief Reads received bytes
         *
         * \param buffer [out]: Buffer for received bytes
         * \param maxLen  [in]: Maximal count of bytes to be read
         *
         * \return Count of bytes copied to buffer (0 if nothing was received)
         */
        virtual crsf_MsgLength_t Get_RxData             ( uint8_t *buffer,
                                                          crsf_MsgLength_t maxLen )    = 0;

        /** \brief Configures baudrate. Applied by next \ref PhyInit */
        virtual void            Set_Baudrate            ( crsf_Baudrate_t baudrate )   = 0;
        /** \brief (Re)initializes peripheral with actual configuration and starts reception */
        virtual void            PhyInit                 ( void )                       = 0;

        /** Default baudrate of full duplex UART (flying platform side) */
        static constexpr crsf_Baudrate_t baudrateFullDuplex = 416666u;

        /** Default baudrate of single wire half duplex UART (RC - TX module) */
        static constexpr crsf_Baudrate_t baudrateHalfDuplex = 400000u;
    };
}

#endif /* CRSF_PHY_HPP */
