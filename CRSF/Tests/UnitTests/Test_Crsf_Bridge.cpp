/**
 * \author Mr.Nobody
 * \file Test_Crsf_Bridge.cpp
 * \ingroup UnitTests
 * \brief Unit tests of CRSF bridge (forwarding of frames between two ports).
 */

/* ============================= INCLUDES =================================== */
#include "unity.h"                          /* Unity testing framework        */
#include "UtCrsf_Helpers.hpp"               /* Fake physical layer, parsing   */
#include "Crsf_App_Bridge.hpp"              /* Code under test                */
#include "Crsf_App.hpp"                     /* Application core               */
#include "Crsf_TL.hpp"                      /* Transport layer                */
/* ============================= TYPEDEFS =================================== */

/** Handler counting processed ping frames */
class PingCounter : public Crsf::FrameHandler
{
public:
    crsf_FrameTypeStatus_t Get_FrameTypeStatus( crsf_FrameType_t type ) const override
    {
        return ( ( CRSF_FRAMETYPE_PARAM_PING == type ) ? CRSF_FRAMETYPE_SUPPORTED : CRSF_FRAMETYPE_NOT_SUPPORTED );
    }

    void Process_Frame( const crsf_Frame_t &frame, Crsf::FrameSender &sender ) override
    {
        (void)frame;
        (void)sender;
        processedCnt++;
    }

    uint32_t processedCnt = 0u;
};

/* ========================== LOCAL VARIABLES =============================== */

static UtCrsf::FakePhy          *phyA   = nullptr;
static UtCrsf::FakePhy          *phyB   = nullptr;
static Crsf::TransportLayer     *tlA    = nullptr;
static Crsf::TransportLayer     *tlB    = nullptr;
static Crsf::AppLayer::Bridge   *bridge = nullptr;

static const uint8_t linkStats[] = { 60u, 61u, 100u, 0xF6, 1u, 2u, 3u, 70u, 98u, 5u };
static const uint8_t vario[]     = { 0xFF, 0x9C };

/* ========================== LOCAL FUNCTIONS =============================== */

/** Filter dropping vario frames received on port A */
static crsf_BridgeDecision_t DropVarioFromA( const crsf_Frame_t *frame, crsf_BridgePort_t sourcePort )
{
    return ( ( ( CRSF_FRAMETYPE_VARIO == frame->Type ) && ( CRSF_BRIDGE_PORT_A == sourcePort ) ) ?
             CRSF_BRIDGE_DROP : CRSF_BRIDGE_FORWARD );
}


/** Checks that frame transmitted by phy is equal to the last injected frame of other phy */
static void CheckForwarded( const UtCrsf::FakePhy &txPhy, uint8_t txIndex, const uint8_t *frame, uint16_t frameLen )
{
    TEST_ASSERT_TRUE( txIndex < txPhy.txFramesCnt );
    TEST_ASSERT_EQUAL_UINT16( frameLen, txPhy.txFrames[ txIndex ].Length );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( frame, txPhy.txFrames[ txIndex ].Data, frameLen );
}

/* ======================== EXPORTED FUNCTIONS ============================== */

void setUp( void )
{
    phyA   = new UtCrsf::FakePhy();
    phyB   = new UtCrsf::FakePhy();
    tlA    = new Crsf::TransportLayer( *phyA );
    tlB    = new Crsf::TransportLayer( *phyB );
    bridge = new Crsf::AppLayer::Bridge( *tlA, *tlB );
}


void tearDown( void )
{
    delete bridge;
    delete tlB;
    delete tlA;
    delete phyB;
    delete phyA;
}

/* --------------------------- Standalone mode ------------------------------ */

/** Task initializes both physical layers */
void Ut_Bridge_Task_InitializesPorts( void )
{
    bridge->Task( 1u );

    TEST_ASSERT_EQUAL_UINT32( 1u, phyA->initCnt );
    TEST_ASSERT_EQUAL_UINT32( 1u, phyB->initCnt );
}


/** Frames are forwarded unchanged in both directions */
void Ut_Bridge_ForwardsBothDirections( void )
{
    bridge->Task( 1u );

    phyA->Inject_Frame( CRSF_FRAMETYPE_LINK_STATISTICS, linkStats, sizeof( linkStats ) );
    phyB->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ) );

    bridge->Task( 1u );

    CheckForwarded( *phyB, 0u, phyA->rxStream, phyA->rxWritePos );
    CheckForwarded( *phyA, 0u, phyB->rxStream, phyB->rxWritePos );

    TEST_ASSERT_EQUAL_UINT32( 1u, bridge->Get_Stats( CRSF_BRIDGE_PORT_A ).ForwardedCnt );
    TEST_ASSERT_EQUAL_UINT32( 1u, bridge->Get_Stats( CRSF_BRIDGE_PORT_B ).ForwardedCnt );
}


/** Extended frame keeps its addresses and sync byte */
void Ut_Bridge_ForwardsExtendedFrameUnchanged( void )
{
    const uint8_t request[] = { 0x03, 0x00 };

    bridge->Task( 1u );

    phyA->Inject_ExtFrame( CRSF_FRAMETYPE_PARAM_READ, CRSF_ADDR_RC_TRANSMITTER, CRSF_ADDR_REMOTE_CONTROL,
                           request, sizeof( request ), CRSF_ADDR_BROADCAST );

    bridge->Task( 1u );

    CheckForwarded( *phyB, 0u, phyA->rxStream, phyA->rxWritePos );
    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_BROADCAST, phyB->txFrames[ 0 ].Data[ 0 ] );
}


/** Damaged data are not forwarded, the following valid frame is */
void Ut_Bridge_InvalidDataNotForwarded( void )
{
    const uint8_t garbage[] = { 0x11, CRSF_SYNC_BYTE, 0x04, CRSF_FRAMETYPE_VARIO, 0x00, 0x00, 0x00 };

    bridge->Task( 1u );

    phyA->Inject( garbage, sizeof( garbage ) );
    const uint16_t validStart = phyA->rxWritePos;
    phyA->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ) );

    bridge->Task( 1u );

    TEST_ASSERT_EQUAL_UINT8( 1u, phyB->txFramesCnt );
    CheckForwarded( *phyB, 0u, &phyA->rxStream[ validStart ], phyA->rxWritePos - validStart );
}


/** Frames with other sync bytes pass only in CRSF_SYNC_ANY mode */
void Ut_Bridge_SyncAnyModeIsTransparent( void )
{
    bridge->Task( 1u );

    phyA->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ), CRSF_ADDR_RC_TRANSMITTER );
    bridge->Task( 1u );

    TEST_ASSERT_EQUAL_UINT8( 0u, phyB->txFramesCnt );

    tlA->Set_SyncMode( CRSF_SYNC_ANY );

    const uint16_t frameStart = phyA->rxWritePos;
    phyA->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ), CRSF_ADDR_RC_TRANSMITTER );
    bridge->Task( 1u );

    TEST_ASSERT_EQUAL_UINT8( 1u, phyB->txFramesCnt );
    CheckForwarded( *phyB, 0u, &phyA->rxStream[ frameStart ], phyA->rxWritePos - frameStart );
}


/** At most framesPerTaskMax frames per port are forwarded in one task */
void Ut_Bridge_FramesPerTaskLimited( void )
{
    bridge->Task( 1u );

    for( uint8_t i = 0u; i < 10u; i++ )
    {
        phyA->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ) );
    }

    bridge->Task( 1u );
    TEST_ASSERT_EQUAL_UINT32( Crsf::AppLayer::Bridge::framesPerTaskMax, bridge->Get_Stats( CRSF_BRIDGE_PORT_A ).ForwardedCnt );

    bridge->Task( 1u );
    TEST_ASSERT_EQUAL_UINT32( 10u, bridge->Get_Stats( CRSF_BRIDGE_PORT_A ).ForwardedCnt );
    TEST_ASSERT_EQUAL_UINT8( 10u, phyB->txFramesCnt );
}


/** Application filter can drop frames per direction */
void Ut_Bridge_Filter( void )
{
    bridge->Set_Filter( DropVarioFromA );
    bridge->Task( 1u );

    phyA->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ) );
    phyA->Inject_Frame( CRSF_FRAMETYPE_LINK_STATISTICS, linkStats, sizeof( linkStats ) );
    phyB->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ) );

    bridge->Task( 1u );

    TEST_ASSERT_EQUAL_UINT8( 1u, phyB->txFramesCnt );
    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_LINK_STATISTICS, phyB->txFrames[ 0 ].Data[ 2 ] );
    TEST_ASSERT_EQUAL_UINT8( 1u, phyA->txFramesCnt );

    TEST_ASSERT_EQUAL_UINT32( 1u, bridge->Get_Stats( CRSF_BRIDGE_PORT_A ).FilteredCnt );
    TEST_ASSERT_EQUAL_UINT32( 1u, bridge->Get_Stats( CRSF_BRIDGE_PORT_A ).ForwardedCnt );
    TEST_ASSERT_EQUAL_UINT32( 1u, bridge->Get_Stats( CRSF_BRIDGE_PORT_B ).ForwardedCnt );
}


/** Full transmit queue of destination port is counted as overflow */
void Ut_Bridge_Overflow( void )
{
    bridge->Task( 1u );

    /* Port B transmission never finishes */
    phyB->txAutoFinish = false;

    for( uint8_t i = 0u; i < 6u; i++ )
    {
        phyA->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ) );
    }

    bridge->Task( 1u );

    const crsf_BridgeStats_t stats = bridge->Get_Stats( CRSF_BRIDGE_PORT_A );

    TEST_ASSERT_EQUAL_UINT32( Crsf::TransportLayer::txQueueLen, stats.ForwardedCnt );
    TEST_ASSERT_EQUAL_UINT32( 6u - Crsf::TransportLayer::txQueueLen, stats.OverflowCnt );

    bridge->Reset_Stats();

    TEST_ASSERT_EQUAL_UINT32( 0u, bridge->Get_Stats( CRSF_BRIDGE_PORT_A ).OverflowCnt );
    TEST_ASSERT_EQUAL_UINT32( 0u, bridge->Get_Stats( CRSF_BRIDGE_PORT_CNT ).ForwardedCnt );
}

/* ------------------------ Device with two ports --------------------------- */

/**
 * Both ports have application core, bridge is attached as observer: frames are
 * processed locally and forwarded, frames for own device are not forwarded.
 */
void Ut_Bridge_WithApplicationCores( void )
{
    PingCounter                 pingA;
    PingCounter                 pingB;
    Crsf::FrameHandler * const  handlersA[] = { &pingA };
    Crsf::FrameHandler * const  handlersB[] = { &pingB };
    Crsf::AppLayer::AppCore     coreA( *tlA, handlersA, 1u );
    Crsf::AppLayer::AppCore     coreB( *tlB, handlersB, 1u );

    coreA.Set_FrameObserver( &bridge->Get_PortObserver( CRSF_BRIDGE_PORT_A ) );
    coreB.Set_FrameObserver( &bridge->Get_PortObserver( CRSF_BRIDGE_PORT_B ) );
    bridge->Set_SkipOwnFrames( true );

    coreA.Task( 1u );
    coreB.Task( 1u );

    /* Broadcast ping: processed locally and forwarded */
    phyA->Inject_ExtFrame( CRSF_FRAMETYPE_PARAM_PING, CRSF_ADDR_BROADCAST, CRSF_ADDR_REMOTE_CONTROL, nullptr, 0u );
    /* Ping for own device: processed locally, not forwarded */
    phyA->Inject_ExtFrame( CRSF_FRAMETYPE_PARAM_PING, UtCrsf::OwnAddr, CRSF_ADDR_REMOTE_CONTROL, nullptr, 0u );
    /* Ping for device behind bridge: not processed, forwarded */
    phyA->Inject_ExtFrame( CRSF_FRAMETYPE_PARAM_PING, CRSF_ADDR_VTX, CRSF_ADDR_REMOTE_CONTROL, nullptr, 0u );
    /* Telemetry from port B is forwarded to port A */
    phyB->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ) );

    coreA.Task( 1u );
    coreB.Task( 1u );

    TEST_ASSERT_EQUAL_UINT32( 2u, pingA.processedCnt );
    TEST_ASSERT_EQUAL_UINT32( 0u, pingB.processedCnt );

    TEST_ASSERT_EQUAL_UINT8( 2u, phyB->txFramesCnt );
    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_BROADCAST, phyB->txFrames[ 0 ].Data[ 3 ] );
    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_VTX,       phyB->txFrames[ 1 ].Data[ 3 ] );

    TEST_ASSERT_EQUAL_UINT8( 1u, phyA->txFramesCnt );
    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_VARIO, phyA->txFrames[ 0 ].Data[ 2 ] );

    TEST_ASSERT_EQUAL_UINT32( 1u, bridge->Get_Stats( CRSF_BRIDGE_PORT_A ).FilteredCnt );
}


/** Frames sent by local device are sent only on its own port */
void Ut_Bridge_LocalFramesNotForwarded( void )
{
    Crsf::AppLayer::AppCore coreA( *tlA, nullptr, 0u );
    Crsf::AppLayer::AppCore coreB( *tlB, nullptr, 0u );

    coreA.Set_FrameObserver( &bridge->Get_PortObserver( CRSF_BRIDGE_PORT_A ) );
    coreB.Set_FrameObserver( &bridge->Get_PortObserver( CRSF_BRIDGE_PORT_B ) );

    coreA.Task( 1u );
    coreB.Task( 1u );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, coreA.Send_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ) ) );

    coreA.Task( 1u );
    coreB.Task( 1u );

    TEST_ASSERT_EQUAL_UINT8( 1u, phyA->txFramesCnt );
    TEST_ASSERT_EQUAL_UINT8( 0u, phyB->txFramesCnt );
}
