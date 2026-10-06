/**
 * \author Mr.Nobody
 * \file Crsf_App.cpp
 * \ingroup Crsf
 * \brief CRSF application layer core - dispatching of received frames.
 */

/* ============================= INCLUDES =================================== */
#include "Crsf_App.hpp"                     /* Self include                   */
/* ============================= TYPEDEFS =================================== */

/* ======================= FORWARD DECLARATIONS ============================= */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* ============================== MACROS ==================================== */

/* ========================= EXPORTED VARIABLES ============================= */

/* ========================== LOCAL VARIABLES =============================== */

/* ======================== EXPORTED FUNCTIONS ============================== */

/**
 * \brief Constructor of application core
 *
 * \param transportLayerObj [in]: Transport layer
 * \param frameHandlers     [in]: Array of frame handlers (can be null if count is 0)
 * \param handlersCount     [in]: Count of frame handlers
 */
Crsf::AppLayer::AppCore::AppCore( TransportLayer &transportLayerObj,
                                  FrameHandler * const frameHandlers[],
                                  uint8_t handlersCount ) :
    transportLayer( transportLayerObj ),
    handlers( frameHandlers ),
    handlersCnt( ( nullptr != frameHandlers ) ? handlersCount : 0u )
{
}


/**
 * \brief Cyclic task of application layer
 *
 * Runs transport layer task, dispatches received frames (at most
 * \ref framesPerTaskMax) and runs tasks of all handlers.
 *
 * \param taskTime [in]: Period of task calling in ms [0.001s]
 */
void Crsf::AppLayer::AppCore::Task( crsf_Time_t taskTime )
{
    crsf_Frame_t frame       = {};
    uint8_t      framesCount = 0u;

    transportLayer.Task( taskTime );

    while( ( framesPerTaskMax > framesCount                   ) &&
           ( CRSF_REQUEST_OK  == transportLayer.Get_Frame( frame ) ) )
    {
        Dispatch_Frame( frame );

        framesCount++;
    }

    for( uint8_t handlerIndex = 0u; handlersCnt > handlerIndex; handlerIndex++ )
    {
        if( nullptr != handlers[ handlerIndex ] )
        {
            handlers[ handlerIndex ]->Task( taskTime, *this );
        }
        else
        {
            /* Handler not assigned */
        }
    }
}


/** \brief Sends frame with short header (see \ref Crsf::TransportLayer::Send_Frame) */
crsf_RequestState_t Crsf::AppLayer::AppCore::Send_Frame( crsf_FrameType_t frameType,
                                                          const uint8_t * const payload, crsf_MsgLength_t payloadLen )
{
    return ( transportLayer.Send_Frame( frameType, payload, payloadLen ) );
}


/** \brief Sends frame with extended header, origin is own address (see \ref Crsf::TransportLayer::Send_ExtFrame) */
crsf_RequestState_t Crsf::AppLayer::AppCore::Send_ExtFrame( crsf_FrameType_t frameType, crsf_DeviceAddr_t destAddr,
                                                             const uint8_t * const payload, crsf_MsgLength_t payloadLen )
{
    return ( transportLayer.Send_ExtFrame( frameType, destAddr, transportLayer.Get_DeviceAddr(), payload, payloadLen ) );
}


/** \brief Returns own device address */
crsf_DeviceAddr_t Crsf::AppLayer::AppCore::Get_DeviceAddr( void ) const
{
    return ( transportLayer.Get_DeviceAddr() );
}


/**
 * \brief Sets own device address (default \ref CRSF_ADDR_FLIGHT_CONTROLLER)
 *
 * \param deviceAddr [in]: Device address
 */
void Crsf::AppLayer::AppCore::Set_DeviceAddr( crsf_DeviceAddr_t deviceAddr )
{
    transportLayer.Set_DeviceAddr( deviceAddr );
}


/**
 * \brief Sets callback called for every received valid frame
 *
 * Callback is called before the frame is dispatched to handlers, also for
 * frames addressed to other devices.
 *
 * \param callback [in]: Callback (null disables callback)
 */
void Crsf::AppLayer::AppCore::Set_FrameCallback( crsf_FrameCallback_t *callback )
{
    frameCallback = callback;
}


/**
 * \brief Sets observer called for every received valid frame
 *
 * Observer is called before the frame is dispatched to handlers, also for
 * frames addressed to other devices (eg. port observer of
 * \ref Crsf::AppLayer::Bridge).
 *
 * \param observer [in]: Observer (null disables observer)
 */
void Crsf::AppLayer::AppCore::Set_FrameObserver( FrameObserver *observer )
{
    frameObserver = observer;
}

/* ========================== LOCAL FUNCTIONS =============================== */

/**
 * \brief Passes frame to observer and to all handlers supporting its type
 *
 * \param frame [in]: Received frame
 */
void Crsf::AppLayer::AppCore::Dispatch_Frame( const crsf_Frame_t &frame )
{
    if( nullptr != frameCallback )
    {
        frameCallback( &frame );
    }
    else
    {
        /* Callback not assigned */
    }

    if( nullptr != frameObserver )
    {
        frameObserver->Observe_Frame( frame );
    }
    else
    {
        /* Observer not assigned */
    }

    const bool addressed = ( CRSF_HEADER_SHORT                == frame.HeaderType ) ||
                           ( CRSF_ADDR_BROADCAST              == frame.DestAddr   ) ||
                           ( transportLayer.Get_DeviceAddr() == frame.DestAddr   );

    if( true == addressed )
    {
        for( uint8_t handlerIndex = 0u; handlersCnt > handlerIndex; handlerIndex++ )
        {
            FrameHandler * const handler = handlers[ handlerIndex ];

            if( ( nullptr                  != handler                                    ) &&
                ( CRSF_FRAMETYPE_SUPPORTED == handler->Get_FrameTypeStatus( frame.Type ) )    )
            {
                handler->Process_Frame( frame, *this );
            }
            else
            {
                /* Handler does not process this frame type */
            }
        }
    }
    else
    {
        /* Frame for other device */
    }
}

/* =============================== TASKS ==================================== */
