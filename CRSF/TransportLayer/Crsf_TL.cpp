/**
 * \author Mr.Nobody
 * \file Crsf_TL.cpp
 * \ingroup Crsf
 * \brief CRSF Transport Layer (TL) - frame search, check and transmission.
 */

/* ============================= INCLUDES =================================== */
#include "Crsf_TL.hpp"                      /* Self include                   */
#include "Crsf_Crc.hpp"                     /* CRC calculation                */
#include "Crsf_Types.h"                     /* Module types definition        */
/* ============================= TYPEDEFS =================================== */

/* ======================= FORWARD DECLARATIONS ============================= */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* ============================== MACROS ==================================== */

/* ========================= EXPORTED VARIABLES ============================= */

/* ========================== LOCAL VARIABLES =============================== */

/* ======================== EXPORTED FUNCTIONS ============================== */

/**
 * \brief Constructor of transport layer
 *
 * \param physicalLayerObj [in]: Physical layer used by the transport layer
 */
Crsf::TransportLayer::TransportLayer( PhysicalLayer &physicalLayerObj ) :
    physicalLayer( physicalLayerObj )
{
}


/**
 * \brief Cyclic task of transport layer
 *
 * The first task initializes the physical layer. Every task updates time since
 * the last valid frame and handles transmission of queued frames. Received
 * frames are read by \ref Get_Frame.
 *
 * \param taskTime [in]: Period of task calling in ms [0.001s]
 */
void Crsf::TransportLayer::Task( crsf_Time_t taskTime )
{
    if( CRSF_TL_INIT == moduleState )
    {
        physicalLayer.PhyInit();

        moduleState = CRSF_TL_ACTIVE;
    }
    else
    {
        /* Already initialized */
    }

    if( ( timeSinceLastFrameMax - timeSinceLastFrame ) > taskTime )
    {
        timeSinceLastFrame += taskTime;
    }
    else
    {
        timeSinceLastFrame = timeSinceLastFrameMax;
    }

    if( ( 0u < rxCount ) && ( rxStallTimeout > rxStallTime ) )
    {
        rxStallTime += taskTime;
    }
    else
    {
        /* Buffer empty or stall already detected */
    }

    Process_Transmission();
}


/**
 * \brief Returns next valid received frame
 *
 * Data received by physical layer are read as long as no complete valid frame
 * is found. Invalid data are dropped.
 *
 * \param frame [out]: Description of the frame. Pointers are valid until the
 *                     next call of this function.
 *
 * \return \ref CRSF_REQUEST_OK if a frame is provided, \ref CRSF_REQUEST_ERROR
 *         if no complete frame has been received.
 */
crsf_RequestState_t Crsf::TransportLayer::Get_Frame( crsf_Frame_t &frame )
{
    crsf_RequestState_t retState  = CRSF_REQUEST_ERROR;
    bool                searching = ( CRSF_TL_ACTIVE == moduleState );

    while( true == searching )
    {
        const parseResult_t parseResult = Parse_Frame();

        if( PARSE_FRAME_FOUND == parseResult )
        {
            frameLength = rxBuffer[ lengthPos ] + CRSF_FRAME_HEADER_LEN;

            for( crsf_MsgLength_t byteIndex = 0u; frameLength > byteIndex; byteIndex++ )
            {
                frameBuffer[ byteIndex ] = rxBuffer[ byteIndex ];
            }

            Drop_RxBytes( frameLength );
            Fill_FrameDescription( frame );

            statsCounter.FramesRcvdCnt++;
            timeSinceLastFrame = 0u;

            retState  = CRSF_REQUEST_OK;
            searching = false;
        }
        else if( PARSE_NEED_MORE_DATA == parseResult )
        {
            const crsf_MsgLength_t rxCountBefore = rxCount;

            Read_PhyData();

            if( rxCountBefore != rxCount )
            {
                rxStallTime = 0u;
            }
            else if( ( 0u < rxCount ) && ( rxStallTimeout <= rxStallTime ) )
            {
                /* Incomplete frame without continuation - it is broken, search
                   for the next frame start behind its first byte */
                statsCounter.DiscardedByteCnt++;
                Drop_RxBytes( 1u );
            }
            else
            {
                /* Nothing new received - no complete frame available */
                searching = false;
            }
        }
        else
        {
            /* Invalid byte dropped - continue with next byte */
        }
    }

    return ( retState );
}


/**
 * \brief Queues frame with short header (broadcast frame) for transmission
 *
 * \param frameType  [in]: Frame type
 * \param payload    [in]: Payload (can be null if payloadLen is 0)
 * \param payloadLen [in]: Payload length [0 - CRSF_PAYLOAD_MAX_LEN]
 *
 * \return \ref CRSF_REQUEST_ERROR if parameters are invalid or the queue is full
 */
crsf_RequestState_t Crsf::TransportLayer::Send_Frame( crsf_FrameType_t frameType,
                                                       const uint8_t * const payload, crsf_MsgLength_t payloadLen )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( CRSF_PAYLOAD_MAX_LEN >= payloadLen ) &&
        ( ( nullptr != payload ) || ( 0u == payloadLen ) ) )
    {
        txFrame_t * const txFrame = Reserve_TxFrame();

        if( nullptr != txFrame )
        {
            txFrame->Data[ 0u ]        = txSyncByte;
            txFrame->Data[ lengthPos ] = payloadLen + CRSF_FRAME_LENGTH_MIN;
            txFrame->Data[ typePos ]   = frameType;

            for( crsf_MsgLength_t byteIndex = 0u; payloadLen > byteIndex; byteIndex++ )
            {
                txFrame->Data[ typePos + CRSF_FRAME_TYPE_LEN + byteIndex ] = payload[ byteIndex ];
            }

            txFrame->Length = payloadLen + CRSF_FRAME_LENGTH_MIN + CRSF_FRAME_HEADER_LEN;

            txFrame->Data[ txFrame->Length - CRSF_FRAME_CRC_LEN ] =
                Crc8::frameCrc.Get_Crc( &txFrame->Data[ typePos ], payloadLen + CRSF_FRAME_TYPE_LEN );

            Commit_TxFrame();

            retState = CRSF_REQUEST_OK;
        }
        else
        {
            /* Queue is full */
        }
    }
    else
    {
        /* Invalid parameters */
    }

    return ( retState );
}


/**
 * \brief Queues frame with extended header for transmission
 *
 * \param frameType  [in]: Frame type
 * \param destAddr   [in]: Destination address
 * \param origAddr   [in]: Origin address
 * \param payload    [in]: Payload (can be null if payloadLen is 0)
 * \param payloadLen [in]: Payload length [0 - CRSF_EXT_PAYLOAD_MAX_LEN]
 *
 * \return \ref CRSF_REQUEST_ERROR if parameters are invalid or the queue is full
 */
crsf_RequestState_t Crsf::TransportLayer::Send_ExtFrame( crsf_FrameType_t frameType,
                                                          crsf_DeviceAddr_t destAddr, crsf_DeviceAddr_t origAddr,
                                                          const uint8_t * const payload, crsf_MsgLength_t payloadLen )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( CRSF_EXT_PAYLOAD_MAX_LEN >= payloadLen ) &&
        ( ( nullptr != payload ) || ( 0u == payloadLen ) ) )
    {
        txFrame_t * const txFrame = Reserve_TxFrame();

        if( nullptr != txFrame )
        {
            const crsf_MsgLength_t bodyLen = payloadLen + CRSF_FRAME_TYPE_LEN + CRSF_FRAME_EXT_HEADER_LEN;

            txFrame->Data[ 0u ]          = txSyncByte;
            txFrame->Data[ lengthPos ]   = bodyLen + CRSF_FRAME_CRC_LEN;
            txFrame->Data[ typePos ]     = frameType;
            txFrame->Data[ destAddrPos ] = destAddr;
            txFrame->Data[ origAddrPos ] = origAddr;

            for( crsf_MsgLength_t byteIndex = 0u; payloadLen > byteIndex; byteIndex++ )
            {
                txFrame->Data[ origAddrPos + 1u + byteIndex ] = payload[ byteIndex ];
            }

            txFrame->Data[ typePos + bodyLen ] = Crc8::frameCrc.Get_Crc( &txFrame->Data[ typePos ], bodyLen );

            txFrame->Length = bodyLen + CRSF_FRAME_HEADER_LEN + CRSF_FRAME_CRC_LEN;

            Commit_TxFrame();

            retState = CRSF_REQUEST_OK;
        }
        else
        {
            /* Queue is full */
        }
    }
    else
    {
        /* Invalid parameters */
    }

    return ( retState );
}


/**
 * \brief Queues complete frame (eg. frame received on other port) for transmission
 *
 * Frame is sent unchanged (including its sync byte and CRC).
 *
 * \param frame    [in]: Complete frame (sync byte ... CRC)
 * \param frameLen [in]: Frame length, it has to correspond to frame length field
 *
 * \return \ref CRSF_REQUEST_ERROR if frame is invalid or the queue is full
 */
crsf_RequestState_t Crsf::TransportLayer::Send_RawFrame( const uint8_t * const frame, crsf_MsgLength_t frameLen )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr                                         != frame              ) &&
        ( ( CRSF_FRAME_HEADER_LEN + CRSF_FRAME_LENGTH_MIN ) <= frameLen         ) &&
        ( CRSF_FRAME_MAX_LEN                              >= frameLen           ) &&
        ( ( frameLen - CRSF_FRAME_HEADER_LEN )            == frame[ lengthPos ] )    )
    {
        txFrame_t * const txFrame = Reserve_TxFrame();

        if( nullptr != txFrame )
        {
            for( crsf_MsgLength_t byteIndex = 0u; frameLen > byteIndex; byteIndex++ )
            {
                txFrame->Data[ byteIndex ] = frame[ byteIndex ];
            }

            txFrame->Length = frameLen;

            Commit_TxFrame();

            retState = CRSF_REQUEST_OK;
        }
        else
        {
            /* Queue is full */
        }
    }
    else
    {
        /* Invalid frame */
    }

    return ( retState );
}


/**
 * \brief Sets own device address
 *
 * Own address is accepted as sync byte of received frames and it is used as
 * origin address by the application layer.
 *
 * \param deviceAddr [in]: Device address (see CRSF_ADDR_xxx)
 */
void Crsf::TransportLayer::Set_DeviceAddr( crsf_DeviceAddr_t deviceAddr )
{
    deviceAddress = deviceAddr;
}


/** \brief Returns own device address */
crsf_DeviceAddr_t Crsf::TransportLayer::Get_DeviceAddr( void ) const
{
    return ( deviceAddress );
}


/**
 * \brief Sets sync byte of transmitted frames
 *
 * Default is \ref CRSF_SYNC_BYTE. Remote control sends frames to TX module
 * with its address (\ref CRSF_ADDR_RC_TRANSMITTER) as sync byte.
 *
 * \param syncByte [in]: Sync byte
 */
void Crsf::TransportLayer::Set_TxSyncByte( crsf_DeviceAddr_t syncByte )
{
    txSyncByte = syncByte;
}


/** \brief Returns sync byte of transmitted frames */
crsf_DeviceAddr_t Crsf::TransportLayer::Get_TxSyncByte( void ) const
{
    return ( txSyncByte );
}


/**
 * \brief Sets accepted sync bytes of received frames
 *
 * \ref CRSF_SYNC_ANY is intended for transparent bridge - frames with any
 * sync byte (eg. 0xEE from remote control to TX module) are accepted, frame
 * start is found only by valid length and CRC.
 *
 * \param mode [in]: Sync mode (default \ref CRSF_SYNC_STANDARD)
 */
void Crsf::TransportLayer::Set_SyncMode( crsf_SyncMode_t mode )
{
    syncMode = mode;
}


/** \brief Returns accepted sync bytes of received frames */
crsf_SyncMode_t Crsf::TransportLayer::Get_SyncMode( void ) const
{
    return ( syncMode );
}


/** \brief Returns count of free places in transmit queue */
uint8_t Crsf::TransportLayer::Get_TxQueueFreeCnt( void ) const
{
    return ( txQueueLen - txCount );
}


/** \brief Returns time since the last valid received frame in ms (saturated) */
crsf_Time_t Crsf::TransportLayer::Get_TimeSinceLastFrame( void ) const
{
    return ( timeSinceLastFrame );
}


/** \brief Returns communication statistics counters */
crsf_StatsCounter_t Crsf::TransportLayer::Get_StatCounter( void ) const
{
    return ( statsCounter );
}


/** \brief Resets communication statistics counters */
void Crsf::TransportLayer::Reset_StatCounter( void )
{
    statsCounter = { 0u, 0u, 0u, 0u, 0u, 0u };
}


/** \brief Returns actual state of transport layer */
crsf_TransportLayerState_t Crsf::TransportLayer::Get_ModuleState( void ) const
{
    return ( moduleState );
}

/* ========================== LOCAL FUNCTIONS =============================== */

/** \brief Appends data received by physical layer to reception buffer */
void Crsf::TransportLayer::Read_PhyData( void )
{
    if( rxBufferSize > rxCount )
    {
        const crsf_MsgLength_t freeSpace = rxBufferSize - rxCount;
        crsf_MsgLength_t       readCount = physicalLayer.Get_RxData( &rxBuffer[ rxCount ], freeSpace );

        if( freeSpace < readCount )
        {
            /* Physical layer shall not return more than requested */
            readCount = freeSpace;
        }
        else
        {
            /* Count is valid */
        }

        rxCount += readCount;
    }
    else
    {
        /* Buffer is full */
    }
}


/**
 * \brief Checks if reception buffer starts with a complete valid frame
 *
 * \return Result of check. Invalid first byte is dropped.
 */
Crsf::TransportLayer::parseResult_t Crsf::TransportLayer::Parse_Frame( void )
{
    parseResult_t parseResult = PARSE_NEED_MORE_DATA;

    if( 0u == rxCount )
    {
        parseResult = PARSE_NEED_MORE_DATA;
    }
    else if( false == Is_SyncByte( rxBuffer[ 0u ] ) )
    {
        statsCounter.DiscardedByteCnt++;
        Drop_RxBytes( 1u );

        parseResult = PARSE_BYTE_DROPPED;
    }
    else if( CRSF_FRAME_HEADER_LEN > rxCount )
    {
        parseResult = PARSE_NEED_MORE_DATA;
    }
    else
    {
        const crsf_MsgLength_t lengthField = rxBuffer[ lengthPos ];

        if( ( CRSF_FRAME_LENGTH_MIN > lengthField ) ||
            ( CRSF_FRAME_LENGTH_MAX < lengthField )    )
        {
            statsCounter.LengthErrorCnt++;
            Drop_RxBytes( 1u );

            parseResult = PARSE_BYTE_DROPPED;
        }
        else if( ( lengthField + CRSF_FRAME_HEADER_LEN ) > rxCount )
        {
            parseResult = PARSE_NEED_MORE_DATA;
        }
        else if( Crc8::frameCrc.Get_Crc( &rxBuffer[ typePos ], lengthField - CRSF_FRAME_CRC_LEN ) !=
                 rxBuffer[ lengthField + CRSF_FRAME_HEADER_LEN - CRSF_FRAME_CRC_LEN ] )
        {
            statsCounter.CrcErrorCnt++;
            Drop_RxBytes( 1u );

            parseResult = PARSE_BYTE_DROPPED;
        }
        else if( ( CRSF_HEADER_EXTENDED == Crsf_Get_HeaderType( rxBuffer[ typePos ] ) ) &&
                 ( ( CRSF_FRAME_LENGTH_MIN + CRSF_FRAME_EXT_HEADER_LEN ) > lengthField )  )
        {
            /* Extended frame too short for its header */
            statsCounter.LengthErrorCnt++;
            Drop_RxBytes( 1u );

            parseResult = PARSE_BYTE_DROPPED;
        }
        else
        {
            parseResult = PARSE_FRAME_FOUND;
        }
    }

    return ( parseResult );
}


/**
 * \brief Removes bytes from the beginning of reception buffer
 *
 * \param count [in]: Count of bytes to be removed
 */
void Crsf::TransportLayer::Drop_RxBytes( crsf_MsgLength_t count )
{
    if( count < rxCount )
    {
        const crsf_MsgLength_t remaining = rxCount - count;

        for( crsf_MsgLength_t byteIndex = 0u; remaining > byteIndex; byteIndex++ )
        {
            rxBuffer[ byteIndex ] = rxBuffer[ byteIndex + count ];
        }

        rxCount = remaining;
    }
    else
    {
        rxCount = 0u;
    }
}


/**
 * \brief Checks if byte can start a frame
 *
 * \param byte [in]: Checked byte
 *
 * \return True for serial sync byte, broadcast address and own address,
 *         always true in \ref CRSF_SYNC_ANY mode
 */
bool Crsf::TransportLayer::Is_SyncByte( uint8_t byte ) const
{
    return ( ( CRSF_SYNC_ANY       == syncMode ) ||
             ( CRSF_SYNC_BYTE      == byte ) ||
             ( CRSF_ADDR_BROADCAST == byte ) ||
             ( deviceAddress       == byte )    );
}


/**
 * \brief Fills frame description of frame stored in frame buffer
 *
 * \param frame [out]: Frame description
 */
void Crsf::TransportLayer::Fill_FrameDescription( crsf_Frame_t &frame ) const
{
    const crsf_MsgLength_t bodyLen = frameBuffer[ lengthPos ] - CRSF_FRAME_LENGTH_MIN;

    frame.SyncByte   = frameBuffer[ 0u ];
    frame.Type       = frameBuffer[ typePos ];
    frame.HeaderType = Crsf_Get_HeaderType( frame.Type );
    frame.Raw        = frameBuffer;
    frame.RawLen     = frameLength;

    if( CRSF_HEADER_EXTENDED == frame.HeaderType )
    {
        frame.DestAddr   = frameBuffer[ destAddrPos ];
        frame.OrigAddr   = frameBuffer[ origAddrPos ];
        frame.Payload    = &frameBuffer[ origAddrPos + 1u ];
        frame.PayloadLen = bodyLen - CRSF_FRAME_EXT_HEADER_LEN;
    }
    else
    {
        frame.DestAddr   = CRSF_ADDR_BROADCAST;
        frame.OrigAddr   = CRSF_ADDR_BROADCAST;
        frame.Payload    = &frameBuffer[ typePos + CRSF_FRAME_TYPE_LEN ];
        frame.PayloadLen = bodyLen;
    }
}


/**
 * \brief Returns free place at the end of transmit queue
 *
 * \return Pointer to free frame or null pointer if the queue is full
 */
Crsf::TransportLayer::txFrame_t *Crsf::TransportLayer::Reserve_TxFrame( void )
{
    txFrame_t *txFrame = nullptr;

    if( txQueueLen > txCount )
    {
        txFrame = &txQueue[ ( txHead + txCount ) % txQueueLen ];
    }
    else
    {
        statsCounter.TxOverflowCnt++;
    }

    return ( txFrame );
}


/** \brief Adds reserved frame to transmit queue and starts transmission if possible */
void Crsf::TransportLayer::Commit_TxFrame( void )
{
    txCount++;

    Process_Transmission();
}


/** \brief Finishes transmitted frame and starts transmission of the next one */
void Crsf::TransportLayer::Process_Transmission( void )
{
    if( ( true             == txActive                                 ) &&
        ( CRSF_TX_FINISHED == physicalLayer.Get_Transmission_Status() )    )
    {
        txActive = false;
        txHead   = ( txHead + 1u ) % txQueueLen;
        txCount--;

        statsCounter.FramesSentCnt++;
    }
    else
    {
        /* Transmission is still active or not started */
    }

    if( ( false          == txActive    ) &&
        ( 0u             <  txCount     ) &&
        ( CRSF_TL_ACTIVE == moduleState )    )
    {
        txActive = true;

        physicalLayer.Set_Transmission_Start( txQueue[ txHead ].Data, txQueue[ txHead ].Length );
    }
    else
    {
        /* Nothing to transmit or transmission in progress */
    }
}

/* =============================== TASKS ==================================== */
