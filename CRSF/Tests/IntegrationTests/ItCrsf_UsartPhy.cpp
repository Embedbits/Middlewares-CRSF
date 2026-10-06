/**
 * \author Mr.Nobody
 * \file ItCrsf_UsartPhy.cpp
 * \ingroup Crsf
 * \brief CRSF physical layer over USART MCAL used by integration tests.
 *
 * Frame format 8N1, full duplex (TX and RX pin). Reception runs continuously
 * in circular buffer mode without end of message detection, the write
 * position of the buffer is read by Usart_Get_RxCount().
 */

/* ============================= INCLUDES =================================== */
#include "ItCrsf_UsartPhy.hpp"              /* Self include                   */
/* ============================= TYPEDEFS =================================== */

/** \brief USART callbacks of one port slot */
typedef struct
{
    usart_XferCallback_t    *TxComplete; /**< Transmission complete */
    usart_XferErrCallback_t *Error;      /**< Transfer error        */
}   itCrsf_PhyCallbacks_t;

/* ======================= FORWARD DECLARATIONS ============================= */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/** USART interrupt priority */
static constexpr usart_IrqPrio_t itCrsf_PhyIrqPriority = 5u;

/* ============================== MACROS ==================================== */

/* ========================== LOCAL VARIABLES =============================== */

/** Physical layer object of every port slot (USART callbacks have no context) */
static ItCrsf::UsartPhy *itCrsf_PhyInstance[ IT_CRSF_PHY_PORT_CNT ] = { nullptr, nullptr };

/* ========================== LOCAL FUNCTIONS =============================== */

/** \brief Transmission complete callback of port slot */
template< itCrsf_PhyPort_t PORT >
static void ItCrsf_TxCompleteCallback( void )
{
    itCrsf_PhyInstance[ PORT ]->On_TxComplete();
}


/** \brief Transfer error callback of port slot */
template< itCrsf_PhyPort_t PORT >
static void ItCrsf_ErrorCallback( usart_XferErrorId_t errorId )
{
    itCrsf_PhyInstance[ PORT ]->On_Error( errorId );
}


/** USART callbacks of port slots */
static const itCrsf_PhyCallbacks_t itCrsf_PhyCallbacks[ IT_CRSF_PHY_PORT_CNT ] =
{
    { ItCrsf_TxCompleteCallback< IT_CRSF_PHY_PORT_HANDSET >, ItCrsf_ErrorCallback< IT_CRSF_PHY_PORT_HANDSET > },
    { ItCrsf_TxCompleteCallback< IT_CRSF_PHY_PORT_FC >,      ItCrsf_ErrorCallback< IT_CRSF_PHY_PORT_FC >      }
};

/* ======================== EXPORTED FUNCTIONS ============================== */

/**
 * \brief Creates physical layer bound to port slot
 *
 * \param port       [in]: Port slot (USART callbacks)
 * \param portConfig [in]: Hardware resources, must stay valid
 */
ItCrsf::UsartPhy::UsartPhy( itCrsf_PhyPort_t port, const itCrsf_PortConfig_t &portConfig )
    : phyPort( port ), hwConfig( portConfig )
{
    itCrsf_PhyInstance[ port ] = this;
}


void ItCrsf::UsartPhy::Set_Transmission_Start( const uint8_t *msgBuffer, crsf_MsgLength_t msgLen )
{
    txFrameCnt++;
    txFinished = false;

    if( USART_REQUEST_OK != Usart_Set_TxStart( hwConfig.Bus, msgBuffer, msgLen ) )
    {
        /* Transport layer waits for the end of transmission - reported as finished */
        txStartErrCnt++;
        txFinished = true;
    }
    else
    {
        /* Transmission is running */
    }
}


crsf_TxStatus_t ItCrsf::UsartPhy::Get_Transmission_Status( void )
{
    return ( ( true == txFinished ) ? CRSF_TX_FINISHED : CRSF_TX_ACTIVE );
}


/**
 * \brief Copies bytes received since the previous call
 *
 * \param buffer [out]: Destination
 * \param maxLen [in]: Maximal count of bytes
 *
 * \return Count of copied bytes
 */
crsf_MsgLength_t ItCrsf::UsartPhy::Get_RxData( uint8_t *buffer, crsf_MsgLength_t maxLen )
{
    usart_RxDataCnt_t writeIdx = 0u;
    crsf_MsgLength_t  readCnt  = 0u;

    if( ( true == initOk ) && ( USART_REQUEST_OK == Usart_Get_RxCount( hwConfig.Bus, &writeIdx ) ) )
    {
        writeIdx %= rxBufferSize;

        while( ( writeIdx != rxReadIdx ) && ( maxLen > readCnt ) )
        {
            buffer[ readCnt ] = rxBuffer[ rxReadIdx ];
            readCnt++;
            rxReadIdx = (usart_RxDataCnt_t)( ( rxReadIdx + 1u ) % rxBufferSize );
        }
    }
    else
    {
        /* Reception is not running */
    }

    rxByteCnt += readCnt;

    return ( readCnt );
}


void ItCrsf::UsartPhy::Set_Baudrate( crsf_Baudrate_t baudrate )
{
    busBaudrate = baudrate;
}


/**
 * \brief Initializes USART (8N1, full duplex) and starts continuous reception
 *
 * RX pin gets pull-up - the line is defined while the TX pin of the other side
 * is not initialized yet.
 */
void ItCrsf::UsartPhy::PhyInit( void )
{
    usart_BusConfig_t     busConfig;
    usart_RequestState_t  initState = USART_REQUEST_ERROR;

    (void)Usart_Set_TxStop( hwConfig.Bus );
    (void)Usart_Set_RxStop( hwConfig.Bus );

    dataConfig.TxMode             = xferMode;
    dataConfig.RxMode             = xferMode;
    dataConfig.RxBuffer           = rxBuffer;
    dataConfig.RxBufferSize       = rxBufferSize;
    dataConfig.RxBufferMode       = USART_BUFFER_MODE_CIRCULAR;
    dataConfig.RxEndMode          = USART_RX_END_NONE;
    dataConfig.TxDmaPeriphId      = USART_DMA_PERIPH_1;
    dataConfig.TxDmaChannelId     = hwConfig.TxDmaChannel;
    dataConfig.TxDmaPriority      = USART_DMA_PRIORITY_LOW;
    dataConfig.RxDmaPeriphId      = USART_DMA_PERIPH_1;
    dataConfig.RxDmaChannelId     = hwConfig.RxDmaChannel;
    dataConfig.RxDmaPriority      = USART_DMA_PRIORITY_HIGH;
    dataConfig.IrqPriority        = itCrsf_PhyIrqPriority;
    dataConfig.TxCompleteCallback = itCrsf_PhyCallbacks[ phyPort ].TxComplete;
    dataConfig.RxHalfCallback     = nullptr;
    dataConfig.RxCompleteCallback = nullptr;
    dataConfig.RxEndCallback      = nullptr;
    dataConfig.ErrorCallback      = itCrsf_PhyCallbacks[ phyPort ].Error;

    if( USART_REQUEST_OK == Usart_Get_DefaultConfig( &busConfig ) )
    {
        busConfig.PeriphId     = hwConfig.Bus;
        busConfig.BaudRate     = busBaudrate;
        busConfig.DataWidth    = USART_DATA_WIDTH_8;
        busConfig.StopBits     = USART_STOP_BITS_1;
        busConfig.Parity       = USART_PARITY_NONE;
        busConfig.TransferMode = USART_TRANSFER_MODE_TX_RX;
        busConfig.HalfDuplex   = USART_HALF_DUPLEX_INACTIVE;
        busConfig.BusTxPin     = hwConfig.TxPin;
        busConfig.BusRxPin     = hwConfig.RxPin;
        busConfig.DataConfig   = &dataConfig;

        initState = Usart_Init( &busConfig );
    }
    else
    {
        /* Default configuration not available */
    }

    if( USART_REQUEST_OK == initState )
    {
        initState = ( GPIO_REQUEST_OK == Gpio_Set_PinPull( hwConfig.RxGpioPort, hwConfig.RxGpioPin, GPIO_PIN_PULL_UP ) ) ?
                    USART_REQUEST_OK : USART_REQUEST_ERROR;
    }
    else
    {
        /* Initialization failed */
    }

    rxReadIdx = 0u;

    if( USART_REQUEST_OK == initState )
    {
        initState = Usart_Set_RxStart( hwConfig.Bus );
    }
    else
    {
        /* Initialization failed */
    }

    initOk     = ( USART_REQUEST_OK == initState );
    txFinished = true;
}


/**
 * \brief Selects data transfer mode of both directions, applied by next PhyInit
 *
 * \param mode [in]: USART_XFER_MODE_ISR / USART_XFER_MODE_DMA
 */
void ItCrsf::UsartPhy::Set_XferMode( usart_XferMode_t mode )
{
    xferMode = mode;
}


/** \brief Returns true if the last PhyInit succeeded (USART initialized, reception running) */
bool ItCrsf::UsartPhy::Get_InitOk( void ) const
{
    return ( initOk );
}


/** \brief Returns count of started transmissions */
uint32_t ItCrsf::UsartPhy::Get_TxFrameCnt( void ) const
{
    return ( txFrameCnt );
}


/** \brief Returns count of bytes passed to transport layer */
uint32_t ItCrsf::UsartPhy::Get_RxByteCnt( void ) const
{
    return ( rxByteCnt );
}


/** \brief Returns count of USART transfer errors */
uint32_t ItCrsf::UsartPhy::Get_ErrorCnt( void ) const
{
    return ( errorCnt );
}


/** \brief Returns count of rejected transmission starts */
uint32_t ItCrsf::UsartPhy::Get_TxStartErrorCnt( void ) const
{
    return ( txStartErrCnt );
}


/** \brief Returns identification of the last USART transfer error (USART_XFER_ERROR_CNT - none) */
usart_XferErrorId_t ItCrsf::UsartPhy::Get_LastErrorId( void ) const
{
    return ( lastErrorId );
}


/** \brief Clears records of communication */
void ItCrsf::UsartPhy::Reset_Records( void )
{
    txFrameCnt    = 0u;
    rxByteCnt     = 0u;
    errorCnt      = 0u;
    txStartErrCnt = 0u;
}


/** \brief Transmission complete event (interrupt context) */
void ItCrsf::UsartPhy::On_TxComplete( void )
{
    txFinished = true;
}


/**
 * \brief Transfer error event (interrupt context)
 *
 * \param errorId [in]: Error identification
 */
void ItCrsf::UsartPhy::On_Error( usart_XferErrorId_t errorId )
{
    lastErrorId = errorId;

    errorCnt++;
}
