/**
 * \author Mr.Nobody
 * \file Crsf_App_Bridge.cpp
 * \ingroup Crsf
 * \brief CRSF bridge - forwarding of frames between two CRSF ports.
 */

/* ============================= INCLUDES =================================== */
#include "Crsf_App_Bridge.hpp"              /* Self include                   */
/* ============================= TYPEDEFS =================================== */

/* ======================= FORWARD DECLARATIONS ============================= */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* ============================== MACROS ==================================== */

/* ========================= EXPORTED VARIABLES ============================= */

/* ========================== LOCAL VARIABLES =============================== */

/* ======================== EXPORTED FUNCTIONS ============================== */

/**
 * \brief Constructor of bridge
 *
 * \param portA [in]: Transport layer of port A
 * \param portB [in]: Transport layer of port B
 */
Crsf::AppLayer::Bridge::Bridge( TransportLayer &portA, TransportLayer &portB ) :
    ports{ &portA, &portB },
    observers{ { *this, CRSF_BRIDGE_PORT_A }, { *this, CRSF_BRIDGE_PORT_B } }
{
}


/**
 * \brief Cyclic task of standalone bridge
 *
 * Runs tasks of both transport layers and forwards received frames (at most
 * \ref framesPerTaskMax per port). Not used when ports have application cores.
 *
 * \param taskTime [in]: Period of task calling in ms [0.001s]
 */
void Crsf::AppLayer::Bridge::Task( crsf_Time_t taskTime )
{
    for( uint8_t portIndex = 0u; CRSF_BRIDGE_PORT_CNT > portIndex; portIndex++ )
    {
        ports[ portIndex ]->Task( taskTime );
    }

    for( uint8_t portIndex = 0u; CRSF_BRIDGE_PORT_CNT > portIndex; portIndex++ )
    {
        crsf_Frame_t frame       = {};
        uint8_t      framesCount = 0u;

        while( ( framesPerTaskMax > framesCount                          ) &&
               ( CRSF_REQUEST_OK  == ports[ portIndex ]->Get_Frame( frame ) ) )
        {
            Forward_Frame( frame, static_cast<crsf_BridgePort_t>( portIndex ) );

            framesCount++;
        }
    }
}


/**
 * \brief Returns observer of port, to be attached to application core of the port
 *
 * \param port [in]: Port (invalid value returns observer of port A)
 *
 * \return Observer forwarding frames received on the port to the other port
 */
Crsf::FrameObserver &Crsf::AppLayer::Bridge::Get_PortObserver( crsf_BridgePort_t port )
{
    return ( observers[ ( CRSF_BRIDGE_PORT_B == port ) ? CRSF_BRIDGE_PORT_B : CRSF_BRIDGE_PORT_A ] );
}


/**
 * \brief Sets application filter called for every frame before forwarding
 *
 * \param filter [in]: Filter (null forwards all frames)
 */
void Crsf::AppLayer::Bridge::Set_Filter( crsf_BridgeFilter_t *filter )
{
    bridgeFilter = filter;
}


/**
 * \brief Enables skipping of extended frames addressed to own device
 *
 * Own address is the device address of transport layer of the source port.
 * Broadcast frames are always forwarded.
 *
 * \param skipOwnFrames [in]: True - frames for own device are not forwarded
 */
void Crsf::AppLayer::Bridge::Set_SkipOwnFrames( bool skipOwnFrames )
{
    skipOwn = skipOwnFrames;
}


/**
 * \brief Returns statistics of frames received on port
 *
 * \param sourcePort [in]: Port
 *
 * \return Statistics (zeros for invalid port)
 */
crsf_BridgeStats_t Crsf::AppLayer::Bridge::Get_Stats( crsf_BridgePort_t sourcePort ) const
{
    crsf_BridgeStats_t portStats = { 0u, 0u, 0u };

    if( CRSF_BRIDGE_PORT_CNT > sourcePort )
    {
        portStats = stats[ sourcePort ];
    }
    else
    {
        /* Invalid port */
    }

    return ( portStats );
}


/** \brief Resets statistics of both ports */
void Crsf::AppLayer::Bridge::Reset_Stats( void )
{
    for( uint8_t portIndex = 0u; CRSF_BRIDGE_PORT_CNT > portIndex; portIndex++ )
    {
        stats[ portIndex ] = { 0u, 0u, 0u };
    }
}

/* ========================== LOCAL FUNCTIONS =============================== */

/** \brief Constructor of port observer */
Crsf::AppLayer::Bridge::PortObserver::PortObserver( Bridge &bridgeObj, crsf_BridgePort_t sourcePort ) :
    bridge( bridgeObj ),
    port( sourcePort )
{
}


/** \brief Forwards frame received on observed port */
void Crsf::AppLayer::Bridge::PortObserver::Observe_Frame( const crsf_Frame_t &frame )
{
    bridge.Forward_Frame( frame, port );
}


/**
 * \brief Forwards frame to the other port if it passes filters
 *
 * \param frame      [in]: Received frame
 * \param sourcePort [in]: Port the frame was received on
 */
void Crsf::AppLayer::Bridge::Forward_Frame( const crsf_Frame_t &frame, crsf_BridgePort_t sourcePort )
{
    const crsf_BridgePort_t destPort = ( CRSF_BRIDGE_PORT_A == sourcePort ) ? CRSF_BRIDGE_PORT_B : CRSF_BRIDGE_PORT_A;
    crsf_BridgeStats_t     &portStats = stats[ sourcePort ];

    const bool ownFrame = ( true                                   == skipOwn          ) &&
                          ( CRSF_HEADER_EXTENDED                   == frame.HeaderType ) &&
                          ( ports[ sourcePort ]->Get_DeviceAddr() == frame.DestAddr   );

    if( ( true == ownFrame ) ||
        ( ( nullptr != bridgeFilter ) && ( CRSF_BRIDGE_DROP == bridgeFilter( &frame, sourcePort ) ) ) )
    {
        portStats.FilteredCnt++;
    }
    else if( CRSF_REQUEST_OK == ports[ destPort ]->Send_RawFrame( frame.Raw, frame.RawLen ) )
    {
        portStats.ForwardedCnt++;
    }
    else
    {
        portStats.OverflowCnt++;
    }
}

/* =============================== TASKS ==================================== */
