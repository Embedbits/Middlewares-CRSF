/**
 * \author Mr.Nobody
 * \file Test_Crsf_AppCore.cpp
 * \ingroup UnitTests
 * \brief Unit tests of CRSF application core, RC channels receiver and telemetry.
 */

/* ============================= INCLUDES =================================== */
#include "unity.h"                          /* Unity testing framework        */
#include "UtCrsf_Helpers.hpp"               /* Test stack                     */
#include "Crsf_App.hpp"                     /* Code under test                */
#include "Crsf_App_RcChannels.hpp"          /* Code under test                */
#include "Crsf_App_Telemetry.hpp"           /* Code under test                */
#include "Crsf_App_Codec.hpp"               /* Payload encoding               */
#include <string.h>                         /* memset                         */
/* ============================= TYPEDEFS =================================== */

/** Handler recording processed frames */
class RecordingHandler : public Crsf::FrameHandler
{
public:
    explicit RecordingHandler( crsf_FrameType_t type ) : frameType( type ) {}

    crsf_FrameTypeStatus_t Get_FrameTypeStatus( crsf_FrameType_t type ) const override
    {
        return ( ( frameType == type ) ? CRSF_FRAMETYPE_SUPPORTED : CRSF_FRAMETYPE_NOT_SUPPORTED );
    }

    void Process_Frame( const crsf_Frame_t &frame, Crsf::FrameSender &sender ) override
    {
        (void)sender;
        processedCnt++;
        lastPayloadLen = frame.PayloadLen;
        lastOrigAddr   = frame.OrigAddr;
    }

    void Task( crsf_Time_t taskTime, Crsf::FrameSender &sender ) override
    {
        (void)sender;
        taskTimeSum += taskTime;
        taskCnt++;
    }

    crsf_FrameType_t  frameType;
    uint32_t          processedCnt   = 0u;
    uint32_t          taskCnt        = 0u;
    crsf_Time_t       taskTimeSum    = 0u;
    crsf_MsgLength_t  lastPayloadLen = 0u;
    crsf_DeviceAddr_t lastOrigAddr   = 0u;
};

/* ========================== LOCAL VARIABLES =============================== */

static uint32_t                observedCnt       = 0u;
static uint32_t                channelsCbCnt     = 0u;
static uint32_t                linkStateCbCnt    = 0u;
static crsf_LinkState_t        lastLinkState     = CRSF_LINK_LOST;
static uint32_t                batteryCbCnt      = 0u;
static crsf_Battery_t          lastBattery       = {};
static uint32_t                linkStatsCbCnt    = 0u;
static crsf_LinkStatistics_t   lastLinkStats     = {};
static uint32_t                timingCbCnt       = 0u;
static crsf_TimingCorrection_t lastTiming        = {};
static uint32_t                flightModeCbCnt   = 0u;
static crsf_FlightMode_t       lastFlightMode    = {};

/* ========================== LOCAL FUNCTIONS =============================== */

static void Observer( const crsf_Frame_t *frame )
{
    (void)frame;
    observedCnt++;
}

static void ChannelsCallback( const crsf_RcChannels_t *channels )
{
    (void)channels;
    channelsCbCnt++;
}

static void LinkStateCallback( crsf_LinkState_t linkState )
{
    linkStateCbCnt++;
    lastLinkState = linkState;
}

static void BatteryCallback( const crsf_Battery_t *data )
{
    batteryCbCnt++;
    lastBattery = *data;
}

static void LinkStatsCallback( const crsf_LinkStatistics_t *data )
{
    linkStatsCbCnt++;
    lastLinkStats = *data;
}

static void TimingCallback( const crsf_TimingCorrection_t *data )
{
    timingCbCnt++;
    lastTiming = *data;
}

static void FlightModeCallback( const crsf_FlightMode_t *data )
{
    flightModeCbCnt++;
    lastFlightMode = *data;
}

/** Injects RC channels frame with all channels set to value */
static void InjectChannels( UtCrsf::FakePhy &phy, uint16_t value )
{
    crsf_RcChannels_t channels = {};
    uint8_t           payload[ CRSF_PAYLOAD_MAX_LEN ];

    for( uint8_t i = 0u; i < CRSF_RC_CHANNELS_CNT; i++ )
    {
        channels.Channels[ i ] = static_cast<uint16_t>( value + i );
    }

    phy.Inject_Frame( CRSF_FRAMETYPE_RC_CHANNELS_PACKED, payload, Crsf::Codec::Encode_RcChannels( channels, payload ) );
}

/* ======================== EXPORTED FUNCTIONS ============================== */

void setUp( void )
{
    observedCnt     = 0u;
    channelsCbCnt   = 0u;
    linkStateCbCnt  = 0u;
    lastLinkState   = CRSF_LINK_LOST;
    batteryCbCnt    = 0u;
    linkStatsCbCnt  = 0u;
    timingCbCnt     = 0u;
    flightModeCbCnt = 0u;
}


void tearDown( void )
{
}

/* ------------------------------- AppCore ---------------------------------- */

/** Frame is passed only to handlers supporting its type, all handlers run task */
void Ut_AppCore_DispatchByType( void )
{
    RecordingHandler            vario( CRSF_FRAMETYPE_VARIO );
    RecordingHandler            battery( CRSF_FRAMETYPE_BATTERY );
    Crsf::FrameHandler * const  handlers[] = { &vario, &battery };
    UtCrsf::Stack               stack( handlers, 2u );
    const uint8_t               data[] = { 0x00, 0x10 };

    stack.phy.Inject_Frame( CRSF_FRAMETYPE_VARIO, data, sizeof( data ) );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 1u, vario.processedCnt );
    TEST_ASSERT_EQUAL_UINT32( 0u, battery.processedCnt );
    TEST_ASSERT_EQUAL_UINT8( 2u, vario.lastPayloadLen );
    TEST_ASSERT_EQUAL_UINT32( 2u, vario.taskCnt );
    TEST_ASSERT_EQUAL_UINT32( 2u, battery.taskCnt );
}


/** Several handlers of the same type all get the frame */
void Ut_AppCore_DispatchToAllMatchingHandlers( void )
{
    RecordingHandler            first( CRSF_FRAMETYPE_VARIO );
    RecordingHandler            second( CRSF_FRAMETYPE_VARIO );
    Crsf::FrameHandler * const  handlers[] = { &first, nullptr, &second };
    UtCrsf::Stack               stack( handlers, 3u );
    const uint8_t               data[] = { 0x00, 0x10 };

    stack.phy.Inject_Frame( CRSF_FRAMETYPE_VARIO, data, sizeof( data ) );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 1u, first.processedCnt );
    TEST_ASSERT_EQUAL_UINT32( 1u, second.processedCnt );
}


/** Extended frames are dispatched only for own address and broadcast */
void Ut_AppCore_ExtendedFrameAddressFilter( void )
{
    RecordingHandler            ping( CRSF_FRAMETYPE_PARAM_PING );
    Crsf::FrameHandler * const  handlers[] = { &ping };
    UtCrsf::Stack               stack( handlers, 1u );

    stack.core.Set_FrameCallback( Observer );

    stack.phy.Inject_ExtFrame( CRSF_FRAMETYPE_PARAM_PING, UtCrsf::OwnAddr, UtCrsf::HostAddr, nullptr, 0u );
    stack.phy.Inject_ExtFrame( CRSF_FRAMETYPE_PARAM_PING, CRSF_ADDR_BROADCAST, UtCrsf::HostAddr, nullptr, 0u );
    stack.phy.Inject_ExtFrame( CRSF_FRAMETYPE_PARAM_PING, CRSF_ADDR_VTX, UtCrsf::HostAddr, nullptr, 0u );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 2u, ping.processedCnt );
    TEST_ASSERT_EQUAL_HEX8( UtCrsf::HostAddr, ping.lastOrigAddr );

    /* Observer gets all frames, also those for other devices */
    TEST_ASSERT_EQUAL_UINT32( 3u, observedCnt );
}


/** Own address is configurable */
void Ut_AppCore_DeviceAddress( void )
{
    RecordingHandler            ping( CRSF_FRAMETYPE_PARAM_PING );
    Crsf::FrameHandler * const  handlers[] = { &ping };
    UtCrsf::Stack               stack( handlers, 1u );

    stack.core.Set_DeviceAddr( CRSF_ADDR_VTX );

    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_VTX, stack.core.Get_DeviceAddr() );

    stack.phy.Inject_ExtFrame( CRSF_FRAMETYPE_PARAM_PING, CRSF_ADDR_VTX, UtCrsf::HostAddr, nullptr, 0u );
    stack.phy.Inject_ExtFrame( CRSF_FRAMETYPE_PARAM_PING, UtCrsf::OwnAddr, UtCrsf::HostAddr, nullptr, 0u );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 1u, ping.processedCnt );
}


/** At most framesPerTaskMax frames are processed in one task, the rest in the next one */
void Ut_AppCore_FramesPerTaskLimited( void )
{
    RecordingHandler            vario( CRSF_FRAMETYPE_VARIO );
    Crsf::FrameHandler * const  handlers[] = { &vario };
    UtCrsf::Stack               stack( handlers, 1u );
    const uint8_t               data[] = { 0x00, 0x10 };

    for( uint8_t i = 0u; i < 10u; i++ )
    {
        stack.phy.Inject_Frame( CRSF_FRAMETYPE_VARIO, data, sizeof( data ) );
    }

    stack.Run();
    TEST_ASSERT_EQUAL_UINT32( Crsf::AppLayer::AppCore::framesPerTaskMax, vario.processedCnt );

    stack.Run();
    TEST_ASSERT_EQUAL_UINT32( 10u, vario.processedCnt );
}


/** Extended frame is sent with own address as origin */
void Ut_AppCore_SendExtFrame_OriginIsOwnAddress( void )
{
    UtCrsf::Stack stack( nullptr, 0u );
    const uint8_t data[] = { 0x01, 0x00 };

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, stack.core.Send_ExtFrame( CRSF_FRAMETYPE_PARAM_READ, CRSF_ADDR_VTX, data, sizeof( data ) ) );

    const UtCrsf::parsedFrame_t frame = UtCrsf::Parse_Tx( stack.phy.txFrames[ 0 ], true );

    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_VTX, frame.Dest );
    TEST_ASSERT_EQUAL_HEX8( UtCrsf::OwnAddr, frame.Orig );
}

/* ----------------------------- RC channels -------------------------------- */

void Ut_RcChannels_InitialState( void )
{
    Crsf::AppLayer::RcChannels rc;

    TEST_ASSERT_EQUAL( CRSF_LINK_LOST, rc.Get_LinkState() );
    TEST_ASSERT_EQUAL_UINT16( CRSF_RC_CHANNEL_VALUE_CENTER, rc.Get_Channel( 0u ) );
    TEST_ASSERT_EQUAL_INT32( 1500, rc.Get_ChannelUs( 15u ) );
    TEST_ASSERT_EQUAL_UINT16( CRSF_RC_CHANNEL_VALUE_CENTER, rc.Get_Channel( 16u ) );
}


/** Received channels are stored, link becomes active and callbacks are called */
void Ut_RcChannels_FrameReceived( void )
{
    Crsf::AppLayer::RcChannels  rc;
    Crsf::FrameHandler * const  handlers[] = { &rc };
    UtCrsf::Stack               stack( handlers, 1u );

    rc.Set_ChannelsCallback( ChannelsCallback );
    rc.Set_LinkStateCallback( LinkStateCallback );

    InjectChannels( stack.phy, 172u );
    stack.Run();

    TEST_ASSERT_EQUAL( CRSF_LINK_ACTIVE, rc.Get_LinkState() );
    TEST_ASSERT_EQUAL_UINT16( 172u, rc.Get_Channel( 0u ) );
    TEST_ASSERT_EQUAL_UINT16( 187u, rc.Get_Channel( 15u ) );
    TEST_ASSERT_EQUAL_UINT16( 173u, rc.Get_Channels().Channels[ 1 ] );
    TEST_ASSERT_EQUAL_INT32( 988, rc.Get_ChannelUs( 0u ) );
    TEST_ASSERT_EQUAL_UINT32( 1u, rc.Get_FramesCnt() );
    TEST_ASSERT_EQUAL_UINT32( 1u, channelsCbCnt );
    TEST_ASSERT_EQUAL_UINT32( 1u, linkStateCbCnt );
    TEST_ASSERT_EQUAL( CRSF_LINK_ACTIVE, lastLinkState );

    /* Next frame does not change link state */
    InjectChannels( stack.phy, 992u );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 2u, channelsCbCnt );
    TEST_ASSERT_EQUAL_UINT32( 1u, linkStateCbCnt );
}


/** Link is lost after failsafe timeout without frames, channels keep last value */
void Ut_RcChannels_FailsafeTimeout( void )
{
    Crsf::AppLayer::RcChannels  rc( 10u );
    Crsf::FrameHandler * const  handlers[] = { &rc };
    UtCrsf::Stack               stack( handlers, 1u );

    rc.Set_LinkStateCallback( LinkStateCallback );

    /* Frame processed in the first task, its handler task counts 1 ms */
    InjectChannels( stack.phy, 500u );
    stack.Run();

    stack.Run( 8u );
    TEST_ASSERT_EQUAL( CRSF_LINK_ACTIVE, rc.Get_LinkState() );

    stack.Run();
    TEST_ASSERT_EQUAL( CRSF_LINK_LOST, rc.Get_LinkState() );
    TEST_ASSERT_EQUAL( CRSF_LINK_LOST, lastLinkState );
    TEST_ASSERT_EQUAL_UINT32( 2u, linkStateCbCnt );
    TEST_ASSERT_EQUAL_UINT16( 500u, rc.Get_Channel( 0u ) );

    /* Link recovers with next frame */
    InjectChannels( stack.phy, 600u );
    stack.Run();

    TEST_ASSERT_EQUAL( CRSF_LINK_ACTIVE, rc.Get_LinkState() );
    TEST_ASSERT_EQUAL_UINT32( 3u, linkStateCbCnt );
}


/** Frames keep the link active */
void Ut_RcChannels_PeriodicFramesKeepLink( void )
{
    Crsf::AppLayer::RcChannels  rc( 10u );
    Crsf::FrameHandler * const  handlers[] = { &rc };
    UtCrsf::Stack               stack( handlers, 1u );

    for( uint8_t i = 0u; i < 10u; i++ )
    {
        InjectChannels( stack.phy, 992u );
        stack.Run( 5u );
    }

    TEST_ASSERT_EQUAL( CRSF_LINK_ACTIVE, rc.Get_LinkState() );
    TEST_ASSERT_EQUAL_UINT32( 10u, rc.Get_FramesCnt() );
}


/** Too short RC channels payload is ignored */
void Ut_RcChannels_ShortPayloadIgnored( void )
{
    Crsf::AppLayer::RcChannels  rc;
    Crsf::FrameHandler * const  handlers[] = { &rc };
    UtCrsf::Stack               stack( handlers, 1u );
    uint8_t                     payload[ 21 ];

    memset( payload, 0, sizeof( payload ) );

    stack.phy.Inject_Frame( CRSF_FRAMETYPE_RC_CHANNELS_PACKED, payload, sizeof( payload ) );
    stack.Run();

    TEST_ASSERT_EQUAL( CRSF_LINK_LOST, rc.Get_LinkState() );
    TEST_ASSERT_EQUAL_UINT32( 0u, rc.Get_FramesCnt() );
}

/* ----------------------------- Telemetry RX ------------------------------- */

/** Only frame types with callback are supported */
void Ut_TelemetryRx_SupportedTypes( void )
{
    crsf_TelemetryCallbacks_t callbacks = {};

    callbacks.Battery = BatteryCallback;

    Crsf::AppLayer::TelemetryRx telemetry( callbacks );

    TEST_ASSERT_EQUAL( CRSF_FRAMETYPE_SUPPORTED,     telemetry.Get_FrameTypeStatus( CRSF_FRAMETYPE_BATTERY ) );
    TEST_ASSERT_EQUAL( CRSF_FRAMETYPE_NOT_SUPPORTED, telemetry.Get_FrameTypeStatus( CRSF_FRAMETYPE_GPS ) );
    TEST_ASSERT_EQUAL( CRSF_FRAMETYPE_NOT_SUPPORTED, telemetry.Get_FrameTypeStatus( CRSF_FRAMETYPE_PARAM_PING ) );
}


/** Received telemetry is decoded and passed to callbacks */
void Ut_TelemetryRx_Callbacks( void )
{
    crsf_TelemetryCallbacks_t callbacks = {};

    callbacks.Battery          = BatteryCallback;
    callbacks.LinkStatistics   = LinkStatsCallback;
    callbacks.TimingCorrection = TimingCallback;
    callbacks.FlightMode       = FlightModeCallback;

    Crsf::AppLayer::TelemetryRx telemetry( callbacks );
    Crsf::FrameHandler * const  handlers[] = { &telemetry };
    UtCrsf::Stack               stack( handlers, 1u );

    const uint8_t battery[]   = { 0x00, 0xA8, 0x00, 0x7D, 0x00, 0x03, 0xE8, 55u };
    const uint8_t linkStats[] = { 60u, 61u, 100u, 0xF6, 1u, 2u, 3u, 70u, 98u, 5u };
    const uint8_t timing[]    = { CRSF_REMOTE_SUBTYPE_TIMING_CORRECTION, 0x00, 0x00, 0x9C, 0x40, 0xFF, 0xFF, 0xFF, 0x06 };
    const uint8_t mode[]      = { 'A', 'N', 'G', 'L', 'E', 0x00 };

    stack.phy.Inject_Frame( CRSF_FRAMETYPE_BATTERY, battery, sizeof( battery ) );
    stack.phy.Inject_Frame( CRSF_FRAMETYPE_LINK_STATISTICS, linkStats, sizeof( linkStats ) );
    stack.phy.Inject_ExtFrame( CRSF_FRAMETYPE_REMOTE, UtCrsf::OwnAddr, CRSF_ADDR_RC_TRANSMITTER, timing, sizeof( timing ) );
    stack.phy.Inject_Frame( CRSF_FRAMETYPE_FLIGHT_MODE, mode, sizeof( mode ) );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 1u, batteryCbCnt );
    TEST_ASSERT_EQUAL_INT16( 168, lastBattery.Voltage );
    TEST_ASSERT_EQUAL_UINT32( 1000u, lastBattery.CapacityUsed );
    TEST_ASSERT_EQUAL_UINT8( 55u, lastBattery.Remaining );

    TEST_ASSERT_EQUAL_UINT32( 1u, linkStatsCbCnt );
    TEST_ASSERT_EQUAL_INT8( -10, lastLinkStats.UpSnr );
    TEST_ASSERT_EQUAL_UINT8( 98u, lastLinkStats.DownLinkQuality );

    TEST_ASSERT_EQUAL_UINT32( 1u, timingCbCnt );
    TEST_ASSERT_EQUAL_UINT32( 40000u, lastTiming.UpdateInterval );
    TEST_ASSERT_EQUAL_INT32( -250, lastTiming.Offset );

    TEST_ASSERT_EQUAL_UINT32( 1u, flightModeCbCnt );
    TEST_ASSERT_EQUAL_STRING( "ANGLE", lastFlightMode.Name );

    TEST_ASSERT_EQUAL_UINT32( 0u, telemetry.Get_InvalidPayloadCnt() );
}


/** Too short payload is counted and callback is not called */
void Ut_TelemetryRx_InvalidPayloadCounted( void )
{
    crsf_TelemetryCallbacks_t callbacks = {};

    callbacks.Battery = BatteryCallback;

    Crsf::AppLayer::TelemetryRx telemetry( callbacks );
    Crsf::FrameHandler * const  handlers[] = { &telemetry };
    UtCrsf::Stack               stack( handlers, 1u );

    const uint8_t battery[] = { 0x00, 0xA8, 0x00 };

    stack.phy.Inject_Frame( CRSF_FRAMETYPE_BATTERY, battery, sizeof( battery ) );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 0u, batteryCbCnt );
    TEST_ASSERT_EQUAL_UINT32( 1u, telemetry.Get_InvalidPayloadCnt() );
}


/** Remote related frame of other sub-type is ignored */
void Ut_TelemetryRx_RemoteOtherSubtypeIgnored( void )
{
    crsf_TelemetryCallbacks_t callbacks = {};

    callbacks.TimingCorrection = TimingCallback;

    Crsf::AppLayer::TelemetryRx telemetry( callbacks );
    Crsf::FrameHandler * const  handlers[] = { &telemetry };
    UtCrsf::Stack               stack( handlers, 1u );

    const uint8_t remote[] = { 0x01, 0x00, 0x00, 0x9C, 0x40, 0xFF, 0xFF, 0xFF, 0x06 };

    stack.phy.Inject_ExtFrame( CRSF_FRAMETYPE_REMOTE, UtCrsf::OwnAddr, CRSF_ADDR_RC_TRANSMITTER, remote, sizeof( remote ) );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 0u, timingCbCnt );
    TEST_ASSERT_EQUAL_UINT32( 0u, telemetry.Get_InvalidPayloadCnt() );
}

/* ----------------------------- Telemetry TX ------------------------------- */

/** Transmitted telemetry is received unchanged by telemetry receiver (loop back) */
void Ut_TelemetryTx_LoopBack( void )
{
    UtCrsf::Stack               txStack( nullptr, 0u );
    Crsf::AppLayer::TelemetryTx telemetryTx( txStack.core );

    const crsf_Battery_t battery = { 151, 230, 2200u, 40u };

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, telemetryTx.Send_Battery( battery ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, telemetryTx.Send_FlightMode( "HORIZON" ) );

    crsf_TelemetryCallbacks_t callbacks = {};

    callbacks.Battery    = BatteryCallback;
    callbacks.FlightMode = FlightModeCallback;

    Crsf::AppLayer::TelemetryRx telemetryRx( callbacks );
    Crsf::FrameHandler * const  handlers[] = { &telemetryRx };
    UtCrsf::Stack               rxStack( handlers, 1u );

    for( uint8_t i = 0u; i < txStack.phy.txFramesCnt; i++ )
    {
        rxStack.phy.Inject( txStack.phy.txFrames[ i ].Data, txStack.phy.txFrames[ i ].Length );
    }

    rxStack.Run();

    TEST_ASSERT_EQUAL_UINT32( 1u, batteryCbCnt );
    TEST_ASSERT_EQUAL_INT16( 151, lastBattery.Voltage );
    TEST_ASSERT_EQUAL_INT16( 230, lastBattery.Current );
    TEST_ASSERT_EQUAL_UINT32( 2200u, lastBattery.CapacityUsed );
    TEST_ASSERT_EQUAL_STRING( "HORIZON", lastFlightMode.Name );
}


/** Heartbeat contains own address */
void Ut_TelemetryTx_Heartbeat( void )
{
    UtCrsf::Stack               stack( nullptr, 0u );
    Crsf::AppLayer::TelemetryTx telemetryTx( stack.core );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, telemetryTx.Send_Heartbeat() );

    const UtCrsf::parsedFrame_t frame = UtCrsf::Parse_Tx( stack.phy.txFrames[ 0 ], false );

    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_HEARTBEAT, frame.Type );
    TEST_ASSERT_EQUAL_UINT8( 2u, frame.PayloadLen );
    TEST_ASSERT_EQUAL_HEX8( 0x00u, frame.Payload[ 0 ] );
    TEST_ASSERT_EQUAL_HEX8( UtCrsf::OwnAddr, frame.Payload[ 1 ] );
}


/** RC channels frame (TX module side) */
void Ut_TelemetryTx_RcChannels( void )
{
    UtCrsf::Stack               stack( nullptr, 0u );
    Crsf::AppLayer::TelemetryTx telemetryTx( stack.core );
    crsf_RcChannels_t           channels = {};

    for( uint8_t i = 0u; i < CRSF_RC_CHANNELS_CNT; i++ )
    {
        channels.Channels[ i ] = 992u;
    }

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, telemetryTx.Send_RcChannels( channels ) );

    const UtCrsf::parsedFrame_t frame = UtCrsf::Parse_Tx( stack.phy.txFrames[ 0 ], false );

    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_RC_CHANNELS_PACKED, frame.Type );
    TEST_ASSERT_EQUAL_UINT8( 22u, frame.PayloadLen );
    TEST_ASSERT_EQUAL_UINT16( 26u, stack.phy.txFrames[ 0 ].Length );
    TEST_ASSERT_EQUAL_HEX8( 0x18u, stack.phy.txFrames[ 0 ].Data[ 1 ] );
}


/** Ping is extended frame without payload */
void Ut_TelemetryTx_ParamPing( void )
{
    UtCrsf::Stack               stack( nullptr, 0u );
    Crsf::AppLayer::TelemetryTx telemetryTx( stack.core );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, telemetryTx.Send_ParamPing( CRSF_ADDR_BROADCAST ) );

    const UtCrsf::parsedFrame_t frame = UtCrsf::Parse_Tx( stack.phy.txFrames[ 0 ], true );

    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_PARAM_PING, frame.Type );
    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_BROADCAST, frame.Dest );
    TEST_ASSERT_EQUAL_HEX8( UtCrsf::OwnAddr, frame.Orig );
    TEST_ASSERT_EQUAL_UINT8( 0u, frame.PayloadLen );
}


/** Direct command contains command CRC over type, destination, origin, IDs and data */
void Ut_TelemetryTx_Command( void )
{
    UtCrsf::Stack               stack( nullptr, 0u );
    Crsf::AppLayer::TelemetryTx telemetryTx( stack.core );
    const uint8_t               power[] = { 14u };

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, telemetryTx.Send_Command( CRSF_ADDR_VTX, CRSF_CMDID_VTX, CRSF_SUBCMD_VTX_SET_POWER,
                                                                  power, sizeof( power ) ) );

    const UtCrsf::parsedFrame_t frame = UtCrsf::Parse_Tx( stack.phy.txFrames[ 0 ], true );

    const uint8_t crcData[] = { CRSF_FRAMETYPE_COMMAND, CRSF_ADDR_VTX, UtCrsf::OwnAddr,
                                CRSF_CMDID_VTX, CRSF_SUBCMD_VTX_SET_POWER, 14u };
    const uint8_t expected[] = { CRSF_CMDID_VTX, CRSF_SUBCMD_VTX_SET_POWER, 14u,
                                 UtCrsf::SpecCommandCrc8( crcData, sizeof( crcData ) ) };

    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_COMMAND, frame.Type );
    TEST_ASSERT_EQUAL_UINT8( sizeof( expected ), frame.PayloadLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, frame.Payload, sizeof( expected ) );
}


void Ut_TelemetryTx_CommandTooLongRejected( void )
{
    UtCrsf::Stack               stack( nullptr, 0u );
    Crsf::AppLayer::TelemetryTx telemetryTx( stack.core );
    uint8_t                     data[ CRSF_EXT_PAYLOAD_MAX_LEN ] = { 0u };

    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, telemetryTx.Send_Command( CRSF_ADDR_VTX, 1u, 1u, data, CRSF_EXT_PAYLOAD_MAX_LEN - 2u ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK,    telemetryTx.Send_Command( CRSF_ADDR_VTX, 1u, 1u, data, CRSF_EXT_PAYLOAD_MAX_LEN - 3u ) );
    TEST_ASSERT_EQUAL_UINT16( CRSF_FRAME_MAX_LEN, stack.phy.txFrames[ 0 ].Length );
}
