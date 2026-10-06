/**
 * \author Mr.Nobody
 * \file ItCrsf_UsartPhy.hpp
 * \ingroup Crsf
 * \brief CRSF physical layer over USART MCAL used by integration tests.
 *
 * Physical layer of integration tests - CRSF stack runs over real USART
 * peripheral (USART MCAL, interrupt or DMA data handling). Reception runs
 * continuously into circular buffer, \ref Get_RxData returns bytes received
 * since the previous call (CRSF is a byte stream, frame boundaries are found
 * by the transport layer).
 *
 * The object records count of transmitted frames, received bytes and USART
 * transfer errors for the test evaluation.
 *
 * USART callbacks have no context parameter, so every object is bound to one
 * port slot (\ref itCrsf_PhyPort_t) with its own set of callbacks.
 */

#ifndef ITCRSF_USARTPHY_HPP
#define ITCRSF_USARTPHY_HPP
/* ============================= INCLUDES =================================== */
#include "Crsf_Phy.hpp"                     /* Physical layer interface       */
#include "Crsf_Types.h"                     /* Module C types definition      */
#include "Usart_Port.h"                     /* USART MCAL                     */
#include "Gpio_Port.h"                      /* RX pin pull-up                 */
/* ============================= TYPEDEFS =================================== */

/** \brief Port slot of physical layer object (selects USART callbacks) */
typedef enum
{
    IT_CRSF_PHY_PORT_HANDSET = 0u, /**< Physical layer of RC side (handset / RC receiver) */
    IT_CRSF_PHY_PORT_FC,           /**< Physical layer of flight controller             */
    IT_CRSF_PHY_PORT_CNT           /**< Count of port slots                              */
}   itCrsf_PhyPort_t;

/** \brief Hardware resources of one USART port */
typedef struct
{
    usart_PeriphId_t     Bus;            /**< USART peripheral                        */
    usart_TxPin_t        TxPin;          /**< TX pin                                  */
    usart_RxPin_t        RxPin;          /**< RX pin                                  */
    gpio_PortId_t        RxGpioPort;     /**< GPIO port of RX pin (pull-up)           */
    gpio_PinId_t         RxGpioPin;      /**< GPIO pin of RX pin (pull-up)            */
    usart_DmaChannelId_t TxDmaChannel;   /**< GPDMA1 channel of transmission (DMA)    */
    usart_DmaChannelId_t RxDmaChannel;   /**< GPDMA1 channel of reception (DMA)       */
}   itCrsf_PortConfig_t;

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* ========================= EXPORTED MACROS ================================ */

/* ========================= EXPORTED VARIABLES ============================= */

/* ======================== EXPORTED FUNCTIONS ============================== */

namespace ItCrsf
{
    class UsartPhy : public Crsf::PhysicalLayer
    {
    public:

        UsartPhy( itCrsf_PhyPort_t port, const itCrsf_PortConfig_t &portConfig );

        /* ------------------- Crsf::PhysicalLayer interface ----------------- */
        void             Set_Transmission_Start  ( const uint8_t *msgBuffer, crsf_MsgLength_t msgLen ) override;
        crsf_TxStatus_t  Get_Transmission_Status ( void )                                         override;
        crsf_MsgLength_t Get_RxData              ( uint8_t *buffer, crsf_MsgLength_t maxLen )     override;
        void             Set_Baudrate            ( crsf_Baudrate_t baudrate )                     override;
        void             PhyInit                 ( void )                                         override;

        /* ----------------------- Test configuration ------------------------ */
        void Set_XferMode( usart_XferMode_t xferMode );

        /* ------------------------- Test evaluation ------------------------- */
        bool     Get_InitOk( void ) const;
        uint32_t Get_TxFrameCnt( void ) const;
        uint32_t Get_RxByteCnt( void ) const;
        uint32_t Get_ErrorCnt( void ) const;
        uint32_t Get_TxStartErrorCnt( void ) const;
        usart_XferErrorId_t Get_LastErrorId( void ) const;
        void     Reset_Records( void );

        /* -------------------- USART events (callbacks) --------------------- */
        void On_TxComplete( void );
        void On_Error( usart_XferErrorId_t errorId );

        /** Size of circular reception buffer */
        static constexpr usart_RxDataCnt_t rxBufferSize = 256u;

    private:

        /** Port slot (USART callbacks) */
        const itCrsf_PhyPort_t phyPort;

        /** Hardware resources */
        const itCrsf_PortConfig_t &hwConfig;

        /** Data handling configuration (copied by USART module) */
        usart_DataConfig_t dataConfig = {};

        /** Data transfer mode of both directions */
        usart_XferMode_t xferMode = USART_XFER_MODE_ISR;

        /** Baud rate, applied by PhyInit */
        crsf_Baudrate_t busBaudrate = baudrateFullDuplex;

        /** Circular reception buffer (USART) */
        uint8_t rxBuffer[ rxBufferSize ] = { 0u };

        /** Read position in reception buffer */
        usart_RxDataCnt_t rxReadIdx = 0u;

        /** Transmission finished (callback context) */
        volatile bool txFinished = true;

        /** Physical layer initialized successfully */
        bool initOk = false;

        /* ------------------------------ Records ------------------------------ */
        uint32_t          txFrameCnt    = 0u;
        uint32_t          rxByteCnt     = 0u;
        volatile uint32_t errorCnt      = 0u;
        volatile usart_XferErrorId_t lastErrorId = USART_XFER_ERROR_CNT;
        uint32_t          txStartErrCnt = 0u;
    };
} /* namespace ItCrsf */

#endif /* ITCRSF_USARTPHY_HPP */
