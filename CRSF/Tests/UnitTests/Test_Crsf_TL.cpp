/**
 * \author Mr.Nobody
 * \file Test_Crsf_TL.cpp
 * \ingroup UnitTests
 * \brief Unit tests of CRSF transport layer (frame search, check and transmission).
 */

/* ============================= INCLUDES =================================== */
#include "unity.h"                          /* Unity testing framework        */
#include "Crsf_TL.hpp"                      /* Code under test                */
#include "UtCrsf_FakePhy.hpp"               /* Fake physical layer            */
#include "UtCrsf_Helpers.hpp"               /* Frame parsing                  */
/* ========================== LOCAL VARIABLES =============================== */

static UtCrsf::FakePhy      *phy = nullptr;
static Crsf::TransportLayer *tl  = nullptr;

/** Payload of link statistics used as test frame */
static const uint8_t linkStatsPayload[] = { 0x20, 0x21, 0x64, 0x0A, 0x01, 0x02, 0x03, 0x30, 0x63, 0xF6 };

/* ======================== EXPORTED FUNCTIONS ============================== */

void setUp( void )
{
    phy = new UtCrsf::FakePhy();
    tl  = new Crsf::TransportLayer( *phy );

    tl->Task( 1u );
}


void tearDown( void )
{
    delete tl;
    delete phy;
}


/** First task initializes physical layer, next tasks do not */
void Ut_TL_Task_InitializesPhyOnce( void )
{
    TEST_ASSERT_EQUAL_UINT32( 1u, phy->initCnt );
    TEST_ASSERT_EQUAL( CRSF_TL_ACTIVE, tl->Get_ModuleState() );

    tl->Task( 1u );

    TEST_ASSERT_EQUAL_UINT32( 1u, phy->initCnt );
}


/** Frames are not read before initialization */
void Ut_TL_GetFrame_NotInitialized( void )
{
    UtCrsf::FakePhy      localPhy;
    Crsf::TransportLayer localTl( localPhy );
    crsf_Frame_t         frame = {};

    localPhy.Inject_Frame( CRSF_FRAMETYPE_LINK_STATISTICS, linkStatsPayload, sizeof( linkStatsPayload ) );

    TEST_ASSERT_EQUAL( CRSF_TL_INIT, localTl.Get_ModuleState() );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, localTl.Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_UINT32( 0u, localPhy.rxReadCnt );
}


void Ut_TL_GetFrame_NothingReceived( void )
{
    crsf_Frame_t frame = {};

    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Get_Frame( frame ) );
}


/** Short header frame is described correctly */
void Ut_TL_GetFrame_ShortFrame( void )
{
    crsf_Frame_t frame = {};

    phy->Inject_Frame( CRSF_FRAMETYPE_LINK_STATISTICS, linkStatsPayload, sizeof( linkStatsPayload ) );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_HEX8( CRSF_SYNC_BYTE, frame.SyncByte );
    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_LINK_STATISTICS, frame.Type );
    TEST_ASSERT_EQUAL( CRSF_HEADER_SHORT, frame.HeaderType );
    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_BROADCAST, frame.DestAddr );
    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_BROADCAST, frame.OrigAddr );
    TEST_ASSERT_EQUAL_UINT8( sizeof( linkStatsPayload ), frame.PayloadLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( linkStatsPayload, frame.Payload, sizeof( linkStatsPayload ) );
    TEST_ASSERT_EQUAL_UINT8( sizeof( linkStatsPayload ) + 4u, frame.RawLen );
    TEST_ASSERT_EQUAL_HEX8( CRSF_SYNC_BYTE, frame.Raw[ 0 ] );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Get_Frame( frame ) );

    const crsf_StatsCounter_t stats = tl->Get_StatCounter();

    TEST_ASSERT_EQUAL_UINT32( 1u, stats.FramesRcvdCnt );
    TEST_ASSERT_EQUAL_UINT32( 0u, stats.CrcErrorCnt );
    TEST_ASSERT_EQUAL_UINT32( 0u, stats.DiscardedByteCnt );
}


/** Extended header frame provides destination, origin and payload behind them */
void Ut_TL_GetFrame_ExtendedFrame( void )
{
    const uint8_t payload[] = { 0x05, 0x00 };
    crsf_Frame_t  frame     = {};

    phy->Inject_ExtFrame( CRSF_FRAMETYPE_PARAM_READ, CRSF_ADDR_FLIGHT_CONTROLLER, CRSF_ADDR_REMOTE_CONTROL,
                          payload, sizeof( payload ) );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL( CRSF_HEADER_EXTENDED, frame.HeaderType );
    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_FLIGHT_CONTROLLER, frame.DestAddr );
    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_REMOTE_CONTROL, frame.OrigAddr );
    TEST_ASSERT_EQUAL_UINT8( 2u, frame.PayloadLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( payload, frame.Payload, sizeof( payload ) );
}


/** Extended frame with empty payload (ping) */
void Ut_TL_GetFrame_ExtendedFrameEmptyPayload( void )
{
    crsf_Frame_t frame = {};

    phy->Inject_ExtFrame( CRSF_FRAMETYPE_PARAM_PING, CRSF_ADDR_BROADCAST, CRSF_ADDR_REMOTE_CONTROL, nullptr, 0u );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_UINT8( 0u, frame.PayloadLen );
    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_REMOTE_CONTROL, frame.OrigAddr );
}


/** Frame delivered byte by byte over several calls is assembled */
void Ut_TL_GetFrame_SplitAcrossReads( void )
{
    crsf_Frame_t frame = {};

    phy->Inject_Frame( CRSF_FRAMETYPE_LINK_STATISTICS, linkStatsPayload, sizeof( linkStatsPayload ) );

    /* Remove the frame from stream and deliver it in two parts */
    uint8_t stream[ 64 ];
    const uint16_t streamLen = phy->rxWritePos;

    for( uint16_t i = 0u; i < streamLen; i++ )
    {
        stream[ i ] = phy->rxStream[ i ];
    }

    phy->rxWritePos = 0u;
    phy->rxReadPos  = 0u;
    phy->Inject( stream, 5u );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Get_Frame( frame ) );

    phy->Inject( &stream[ 5 ], streamLen - 5u );
    phy->rxChunkMax = 1u;

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( linkStatsPayload, frame.Payload, sizeof( linkStatsPayload ) );
}


/** Several frames received at once are provided one by one */
void Ut_TL_GetFrame_MultipleFramesInOneRead( void )
{
    crsf_Frame_t  frame      = {};
    const uint8_t vario[]    = { 0x01, 0x02 };
    const uint8_t battery[]  = { 0x00, 0xA8, 0x00, 0x10, 0x00, 0x01, 0x00, 0x55 };

    phy->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ) );
    phy->Inject_Frame( CRSF_FRAMETYPE_BATTERY, battery, sizeof( battery ) );
    phy->Inject_Frame( CRSF_FRAMETYPE_LINK_STATISTICS, linkStatsPayload, sizeof( linkStatsPayload ) );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_VARIO, frame.Type );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_BATTERY, frame.Type );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( battery, frame.Payload, sizeof( battery ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_LINK_STATISTICS, frame.Type );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Get_Frame( frame ) );

    TEST_ASSERT_EQUAL_UINT32( 3u, tl->Get_StatCounter().FramesRcvdCnt );
}


/** Bytes in front of sync byte are discarded */
void Ut_TL_GetFrame_GarbageBeforeSyncDiscarded( void )
{
    const uint8_t garbage[] = { 0x11, 0x22, 0x33 };
    crsf_Frame_t  frame     = {};

    phy->Inject( garbage, sizeof( garbage ) );
    phy->Inject_Frame( CRSF_FRAMETYPE_LINK_STATISTICS, linkStatsPayload, sizeof( linkStatsPayload ) );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_LINK_STATISTICS, frame.Type );
    TEST_ASSERT_EQUAL_UINT32( 3u, tl->Get_StatCounter().DiscardedByteCnt );
}


/** Broadcast address and own address are accepted as sync byte, other addresses are not */
void Ut_TL_GetFrame_AcceptedSyncBytes( void )
{
    const uint8_t vario[] = { 0x01, 0x02 };
    crsf_Frame_t  frame   = {};

    tl->Set_DeviceAddr( CRSF_ADDR_RC_TRANSMITTER );

    phy->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ), CRSF_ADDR_BROADCAST );
    phy->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ), CRSF_ADDR_RC_TRANSMITTER );
    phy->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ), CRSF_ADDR_RC_RECEIVER );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_BROADCAST, frame.SyncByte );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_RC_TRANSMITTER, frame.SyncByte );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Get_Frame( frame ) );
}


/** Frame length field out of range 2 - 62 is rejected */
void Ut_TL_GetFrame_InvalidLengthRejected( void )
{
    const uint8_t tooShort[] = { CRSF_SYNC_BYTE, 0x01, 0x07 };
    const uint8_t tooLong[]  = { CRSF_SYNC_BYTE, 63u, 0x07 };
    crsf_Frame_t  frame      = {};

    phy->Inject( tooShort, sizeof( tooShort ) );
    phy->Inject( tooLong, sizeof( tooLong ) );
    phy->Inject_Frame( CRSF_FRAMETYPE_LINK_STATISTICS, linkStatsPayload, sizeof( linkStatsPayload ) );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_LINK_STATISTICS, frame.Type );
    TEST_ASSERT_EQUAL_UINT32( 2u, tl->Get_StatCounter().LengthErrorCnt );
}


/** Maximal frame (64 bytes) is accepted */
void Ut_TL_GetFrame_MaximalFrameAccepted( void )
{
    uint8_t      payload[ CRSF_PAYLOAD_MAX_LEN ];
    crsf_Frame_t frame = {};

    for( uint8_t i = 0u; i < CRSF_PAYLOAD_MAX_LEN; i++ )
    {
        payload[ i ] = i;
    }

    phy->Inject_Frame( CRSF_FRAMETYPE_FLIGHT_MODE, payload, CRSF_PAYLOAD_MAX_LEN );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_UINT8( CRSF_FRAME_MAX_LEN, frame.RawLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( payload, frame.Payload, CRSF_PAYLOAD_MAX_LEN );
}


/** Frame with invalid CRC is dropped and the following valid frame is found */
void Ut_TL_GetFrame_CrcErrorResynchronizes( void )
{
    const uint8_t vario[] = { 0x01, 0x02 };
    crsf_Frame_t  frame   = {};

    phy->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ) );
    phy->rxStream[ phy->rxWritePos - 1u ] ^= 0xFFu;
    phy->Inject_Frame( CRSF_FRAMETYPE_LINK_STATISTICS, linkStatsPayload, sizeof( linkStatsPayload ) );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_LINK_STATISTICS, frame.Type );
    TEST_ASSERT_EQUAL_UINT32( 1u, tl->Get_StatCounter().CrcErrorCnt );
    TEST_ASSERT_EQUAL_UINT32( 1u, tl->Get_StatCounter().FramesRcvdCnt );
}


/** Truncated frame followed by valid frame - the valid frame is not lost */
void Ut_TL_GetFrame_TruncatedFrameResynchronizes( void )
{
    const uint8_t truncated[] = { CRSF_SYNC_BYTE, 12u, CRSF_FRAMETYPE_LINK_STATISTICS, 0x20, 0x21 };
    crsf_Frame_t  frame       = {};

    phy->Inject( truncated, sizeof( truncated ) );
    phy->Inject_Frame( CRSF_FRAMETYPE_LINK_STATISTICS, linkStatsPayload, sizeof( linkStatsPayload ) );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( linkStatsPayload, frame.Payload, sizeof( linkStatsPayload ) );
}


/** Extended frame type with length too short for extended header is rejected */
void Ut_TL_GetFrame_ExtendedFrameTooShortRejected( void )
{
    const uint8_t payload[] = { 0x11 };
    crsf_Frame_t  frame     = {};

    /* Type 0x2C with only one byte behind type */
    phy->Inject_Frame( CRSF_FRAMETYPE_PARAM_READ, payload, sizeof( payload ) );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_UINT32( 1u, tl->Get_StatCounter().LengthErrorCnt );
}


/** Time since last frame counts task time and is reset by received frame */
void Ut_TL_TimeSinceLastFrame( void )
{
    crsf_Frame_t frame = {};

    phy->Inject_Frame( CRSF_FRAMETYPE_LINK_STATISTICS, linkStatsPayload, sizeof( linkStatsPayload ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_UINT32( 0u, tl->Get_TimeSinceLastFrame() );

    tl->Task( 5u );
    tl->Task( 5u );

    TEST_ASSERT_EQUAL_UINT32( 10u, tl->Get_TimeSinceLastFrame() );
}


/** Time since last frame saturates */
void Ut_TL_TimeSinceLastFrame_Saturates( void )
{
    tl->Task( 0xFFFFFFF0u );
    tl->Task( 0x100u );

    TEST_ASSERT_EQUAL_HEX32( 0xFFFFFFFFu, tl->Get_TimeSinceLastFrame() );
}


/** Short frame is built as sync, length, type, payload, CRC */
void Ut_TL_SendFrame_Layout( void )
{
    const uint8_t vario[] = { 0xFF, 0x9C };

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Send_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ) ) );
    TEST_ASSERT_EQUAL_UINT8( 1u, phy->txFramesCnt );

    const uint8_t body[]     = { CRSF_FRAMETYPE_VARIO, 0xFF, 0x9C };
    const uint8_t expected[] = { CRSF_SYNC_BYTE, 0x04, CRSF_FRAMETYPE_VARIO, 0xFF, 0x9C,
                                 UtCrsf::SpecCrc8( body, sizeof( body ) ) };

    TEST_ASSERT_EQUAL_UINT16( sizeof( expected ), phy->txFrames[ 0 ].Length );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, phy->txFrames[ 0 ].Data, sizeof( expected ) );
}


/** Extended frame is built as sync, length, type, destination, origin, payload, CRC */
void Ut_TL_SendExtFrame_Layout( void )
{
    const uint8_t payload[] = { 0x01, 0x00 };

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Send_ExtFrame( CRSF_FRAMETYPE_PARAM_READ, CRSF_ADDR_RC_TRANSMITTER,
                                                           CRSF_ADDR_REMOTE_CONTROL, payload, sizeof( payload ) ) );

    const uint8_t body[]     = { CRSF_FRAMETYPE_PARAM_READ, CRSF_ADDR_RC_TRANSMITTER, CRSF_ADDR_REMOTE_CONTROL, 0x01, 0x00 };
    const uint8_t expected[] = { CRSF_SYNC_BYTE, 0x06, CRSF_FRAMETYPE_PARAM_READ, CRSF_ADDR_RC_TRANSMITTER,
                                 CRSF_ADDR_REMOTE_CONTROL, 0x01, 0x00, UtCrsf::SpecCrc8( body, sizeof( body ) ) };

    TEST_ASSERT_EQUAL_UINT16( sizeof( expected ), phy->txFrames[ 0 ].Length );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, phy->txFrames[ 0 ].Data, sizeof( expected ) );
}


/** Configured sync byte is used for transmitted frames */
void Ut_TL_SendFrame_ConfiguredSyncByte( void )
{
    tl->Set_TxSyncByte( CRSF_ADDR_RC_TRANSMITTER );

    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_RC_TRANSMITTER, tl->Get_TxSyncByte() );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Send_Frame( CRSF_FRAMETYPE_HEARTBEAT, linkStatsPayload, 2u ) );
    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_RC_TRANSMITTER, phy->txFrames[ 0 ].Data[ 0 ] );
}


/** Payload longer than maximum or null payload is rejected */
void Ut_TL_SendFrame_InvalidParametersRejected( void )
{
    uint8_t payload[ CRSF_PAYLOAD_MAX_LEN + 1u ] = { 0u };

    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Send_Frame( CRSF_FRAMETYPE_FLIGHT_MODE, payload, CRSF_PAYLOAD_MAX_LEN + 1u ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Send_Frame( CRSF_FRAMETYPE_FLIGHT_MODE, nullptr, 1u ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Send_ExtFrame( CRSF_FRAMETYPE_PARAM_ENTRY, 0u, 0u, payload, CRSF_EXT_PAYLOAD_MAX_LEN + 1u ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK,    tl->Send_Frame( CRSF_FRAMETYPE_FLIGHT_MODE, payload, CRSF_PAYLOAD_MAX_LEN ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK,    tl->Send_ExtFrame( CRSF_FRAMETYPE_PARAM_ENTRY, 0u, 0u, payload, CRSF_EXT_PAYLOAD_MAX_LEN ) );

    TEST_ASSERT_EQUAL_UINT16( CRSF_FRAME_MAX_LEN, phy->txFrames[ 0 ].Length );
    TEST_ASSERT_EQUAL_UINT16( CRSF_FRAME_MAX_LEN, phy->txFrames[ 1 ].Length );
}


/** Frames are queued while transmission is active and sent in order */
void Ut_TL_TxQueue_SendsInOrder( void )
{
    const uint8_t data[] = { 0x01, 0x02 };

    phy->txAutoFinish = false;

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Send_Frame( CRSF_FRAMETYPE_VARIO, data, sizeof( data ) ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Send_Frame( CRSF_FRAMETYPE_HEARTBEAT, data, sizeof( data ) ) );

    TEST_ASSERT_EQUAL_UINT32( 1u, phy->txStartCnt );

    /* Transmission still active */
    tl->Task( 1u );
    TEST_ASSERT_EQUAL_UINT32( 1u, phy->txStartCnt );

    phy->txStatus = CRSF_TX_FINISHED;
    tl->Task( 1u );

    TEST_ASSERT_EQUAL_UINT32( 2u, phy->txStartCnt );
    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_VARIO, phy->txFrames[ 0 ].Data[ 2 ] );
    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_HEARTBEAT, phy->txFrames[ 1 ].Data[ 2 ] );

    phy->txStatus = CRSF_TX_FINISHED;
    tl->Task( 1u );

    TEST_ASSERT_EQUAL_UINT32( 2u, tl->Get_StatCounter().FramesSentCnt );
    TEST_ASSERT_EQUAL_UINT8( Crsf::TransportLayer::txQueueLen, tl->Get_TxQueueFreeCnt() );
}


/** Full queue rejects frame and counts overflow */
void Ut_TL_TxQueue_Overflow( void )
{
    const uint8_t data[] = { 0x01, 0x02 };

    phy->txAutoFinish = false;

    for( uint8_t i = 0u; i < Crsf::TransportLayer::txQueueLen; i++ )
    {
        TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Send_Frame( CRSF_FRAMETYPE_VARIO, data, sizeof( data ) ) );
    }

    TEST_ASSERT_EQUAL_UINT8( 0u, tl->Get_TxQueueFreeCnt() );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Send_Frame( CRSF_FRAMETYPE_VARIO, data, sizeof( data ) ) );
    TEST_ASSERT_EQUAL_UINT32( 1u, tl->Get_StatCounter().TxOverflowCnt );
}


/** Frames queued before initialization are sent after it */
void Ut_TL_TxQueue_StartsAfterInit( void )
{
    UtCrsf::FakePhy      localPhy;
    Crsf::TransportLayer localTl( localPhy );
    const uint8_t        data[] = { 0x01, 0x02 };

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, localTl.Send_Frame( CRSF_FRAMETYPE_VARIO, data, sizeof( data ) ) );
    TEST_ASSERT_EQUAL_UINT32( 0u, localPhy.txStartCnt );

    localTl.Task( 1u );

    TEST_ASSERT_EQUAL_UINT32( 1u, localPhy.txStartCnt );
}


/** Raw frame is sent unchanged, inconsistent frame is rejected */
void Ut_TL_SendRawFrame( void )
{
    crsf_Frame_t frame = {};

    phy->Inject_Frame( CRSF_FRAMETYPE_LINK_STATISTICS, linkStatsPayload, sizeof( linkStatsPayload ), CRSF_ADDR_BROADCAST );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Send_RawFrame( frame.Raw, frame.RawLen ) );
    TEST_ASSERT_EQUAL_UINT16( frame.RawLen, phy->txFrames[ 0 ].Length );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( frame.Raw, phy->txFrames[ 0 ].Data, frame.RawLen );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Send_RawFrame( frame.Raw, frame.RawLen - 1u ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Send_RawFrame( nullptr, frame.RawLen ) );
}


void Ut_TL_ResetStatCounter( void )
{
    const uint8_t garbage[] = { 0x11 };
    crsf_Frame_t  frame     = {};

    phy->Inject( garbage, sizeof( garbage ) );
    (void)tl->Get_Frame( frame );

    tl->Reset_StatCounter();

    TEST_ASSERT_EQUAL_UINT32( 0u, tl->Get_StatCounter().DiscardedByteCnt );
}


/** CRSF_SYNC_ANY accepts any sync byte, frame is still checked by length and CRC */
void Ut_TL_GetFrame_SyncAnyMode( void )
{
    const uint8_t vario[]   = { 0x01, 0x02 };
    const uint8_t garbage[] = { 0x11, 0x22, 0x33 };
    crsf_Frame_t  frame     = {};

    TEST_ASSERT_EQUAL( CRSF_SYNC_STANDARD, tl->Get_SyncMode() );

    tl->Set_SyncMode( CRSF_SYNC_ANY );

    TEST_ASSERT_EQUAL( CRSF_SYNC_ANY, tl->Get_SyncMode() );

    phy->Inject( garbage, sizeof( garbage ) );
    phy->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ), CRSF_ADDR_RC_RECEIVER );

    /* 0x11 looks like frame start with length 0x22 - waits for more data */
    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Get_Frame( frame ) );

    /* No more data within stall timeout - search continues behind 0x11 */
    tl->Task( 5u );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_HEX8( CRSF_ADDR_RC_RECEIVER, frame.SyncByte );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( vario, frame.Payload, sizeof( vario ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Get_Frame( frame ) );
}


/** Incomplete frame followed by silence is resynchronized after stall timeout */
void Ut_TL_GetFrame_StalledFrameResynchronized( void )
{
    const uint8_t truncated[] = { CRSF_SYNC_BYTE, 60u, CRSF_FRAMETYPE_FLIGHT_MODE, 'A', 'B' };
    const uint8_t vario[]     = { 0x01, 0x02 };
    crsf_Frame_t  frame       = {};

    phy->Inject( truncated, sizeof( truncated ) );
    phy->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ) );

    /* Waiting for the rest of 62 bytes long frame */
    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Get_Frame( frame ) );

    tl->Task( 4u );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Get_Frame( frame ) );

    tl->Task( 1u );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_VARIO, frame.Type );
    TEST_ASSERT_EQUAL_UINT32( sizeof( truncated ), tl->Get_StatCounter().DiscardedByteCnt );
}


/** Slowly arriving frame is not dropped - every new byte restarts stall timeout */
void Ut_TL_GetFrame_SlowFrameNotDropped( void )
{
    const uint8_t vario[] = { 0x01, 0x02 };
    crsf_Frame_t  frame   = {};
    uint8_t       stream[ 8 ];

    /* Build frame, then deliver it byte by byte with 4 ms gaps */
    phy->Inject_Frame( CRSF_FRAMETYPE_VARIO, vario, sizeof( vario ) );

    const uint16_t frameLen = phy->rxWritePos;

    for( uint16_t i = 0u; i < frameLen; i++ )
    {
        stream[ i ] = phy->rxStream[ i ];
    }

    phy->rxWritePos = 0u;

    for( uint16_t i = 0u; i < ( frameLen - 1u ); i++ )
    {
        phy->Inject( &stream[ i ], 1u );
        TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, tl->Get_Frame( frame ) );
        tl->Task( 4u );
    }

    phy->Inject( &stream[ frameLen - 1u ], 1u );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, tl->Get_Frame( frame ) );
    TEST_ASSERT_EQUAL_UINT32( 0u, tl->Get_StatCounter().DiscardedByteCnt );
}
