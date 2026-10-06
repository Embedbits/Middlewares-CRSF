/**
 * \author Mr.Nobody
 * \file Test_Crsf_Params.cpp
 * \ingroup UnitTests
 * \brief Unit tests of CRSF device parameters (ping, device info, read, write, chunks).
 */

/* ============================= INCLUDES =================================== */
#include "unity.h"                          /* Unity testing framework        */
#include "UtCrsf_Helpers.hpp"               /* Test stack                     */
#include "Crsf_App_Params.hpp"              /* Code under test                */
#include <string.h>                         /* strcpy, strlen, memcpy         */
/* ========================= SYMBOLIC CONSTANTS ============================= */

/** Parameter numbers of test configuration */
static constexpr uint8_t ParamSettings   = 1u;
static constexpr uint8_t ParamGain       = 2u;
static constexpr uint8_t ParamMode       = 3u;
static constexpr uint8_t ParamPilot      = 4u;
static constexpr uint8_t ParamVersion    = 5u;
static constexpr uint8_t ParamCalibrate  = 6u;
static constexpr uint8_t ParamHidden     = 7u;
static constexpr uint8_t ParamLong       = 8u;
static constexpr uint8_t ParamReadOnly   = 9u;
static constexpr uint8_t ParamsCount     = 9u;

/* ========================== LOCAL VARIABLES =============================== */

static int32_t          gainValue      = 150;
static uint32_t         gainSetCnt     = 0u;
static int32_t          modeValue      = 1;
static uint32_t         modeSetCnt     = 0u;
static char             pilotName[ 32 ] = "Quad";
static uint32_t         pilotSetCnt    = 0u;
static crsf_CmdStatus_t calibStatus    = CRSF_CMD_READY;
static int32_t          calibLastAction = -1;
static uint32_t         calibSetCnt    = 0u;
static int32_t          longValue      = 0;

/** Long options forcing entry into several chunks */
static const char longOptions[] =
    "Option number zero;Option number one;Option number two;Option number three;"
    "Option number four;Option number five";

/* ========================== LOCAL FUNCTIONS =============================== */

static crsf_RequestState_t Get_Gain( crsf_ParamValue_t *value )        { value->Number = gainValue; return ( CRSF_REQUEST_OK ); }
static crsf_RequestState_t Set_Gain( const crsf_ParamValue_t *value )  { gainValue = value->Number; gainSetCnt++; return ( CRSF_REQUEST_OK ); }
static crsf_RequestState_t Get_Mode( crsf_ParamValue_t *value )        { value->Number = modeValue; return ( CRSF_REQUEST_OK ); }
static crsf_RequestState_t Set_Mode( const crsf_ParamValue_t *value )  { modeValue = value->Number; modeSetCnt++; return ( CRSF_REQUEST_OK ); }
static crsf_RequestState_t Get_Pilot( crsf_ParamValue_t *value )       { value->Text = pilotName; return ( CRSF_REQUEST_OK ); }
static crsf_RequestState_t Get_Version( crsf_ParamValue_t *value )     { value->Text = "1.2.3"; return ( CRSF_REQUEST_OK ); }
static crsf_RequestState_t Get_Long( crsf_ParamValue_t *value )        { value->Number = longValue; return ( CRSF_REQUEST_OK ); }
static crsf_RequestState_t Get_Failing( crsf_ParamValue_t *value )     { (void)value; return ( CRSF_REQUEST_ERROR ); }

static crsf_RequestState_t Set_Pilot( const crsf_ParamValue_t *value )
{
    strcpy( pilotName, value->Text );
    pilotSetCnt++;
    return ( CRSF_REQUEST_OK );
}

static crsf_RequestState_t Get_Calibrate( crsf_ParamValue_t *value )
{
    value->Number = calibStatus;
    value->Text   = ( CRSF_CMD_PROGRESS == calibStatus ) ? "Calibrating" : "";
    return ( CRSF_REQUEST_OK );
}

static crsf_RequestState_t Set_Calibrate( const crsf_ParamValue_t *value )
{
    calibLastAction = value->Number;
    calibSetCnt++;

    if( CRSF_CMD_START == value->Number )
    {
        calibStatus = CRSF_CMD_PROGRESS;
    }

    return ( CRSF_REQUEST_OK );
}

static crsf_ParamVisibility_t Get_HiddenVisibility( void ) { return ( CRSF_PARAM_HIDDEN ); }

/** Device information */
static const crsf_DeviceInfo_t deviceInfo = { "EmBi FC", 0x12345678u, 0x00010002u, 0x00030004u, 7u };

/** Parameters configuration */
static crsf_ParamConfig_t paramsConfig[ ParamsCount ] =
{
    CRSF_PARAM_FOLDER_ITEM        ( 0u,            "Settings" ),
    CRSF_PARAM_FLOAT_ITEM         ( ParamSettings, "Gain",      Get_Gain, Set_Gain, 0, 500, 100, 2u, 5, "x" ),
    CRSF_PARAM_TEXT_SELECTION_ITEM( ParamSettings, "Mode",      "Off;Low;High", Get_Mode, Set_Mode, 0, "" ),
    CRSF_PARAM_STRING_ITEM        ( 0u,            "Pilot",     Get_Pilot, Set_Pilot, 8 ),
    CRSF_PARAM_INFO_ITEM          ( 0u,            "Version",   Get_Version ),
    CRSF_PARAM_COMMAND_ITEM       ( 0u,            "Calibrate", Get_Calibrate, Set_Calibrate, 50 ),
    CRSF_PARAM_FLOAT_ITEM         ( ParamSettings, "Hidden",    Get_Failing, nullptr, -10, 10, 3, 0u, 1, "" ),
    CRSF_PARAM_TEXT_SELECTION_ITEM( 0u,            "Long",      longOptions, Get_Long, nullptr, 0, "unit" ),
    CRSF_PARAM_FLOAT_ITEM         ( 0u,            "ReadOnly",  Get_Gain, nullptr, 0, 500, 100, 2u, 5, "" ),
};

static Crsf::AppLayer::Params *params = nullptr;
static UtCrsf::Stack          *stack  = nullptr;

/** Entry assembled from received chunks */
static uint8_t  entry[ 512 ];
static uint16_t entryLen = 0u;

/* ========================== HELPER FUNCTIONS ============================== */

/** Sends request and returns count of response frames */
static uint8_t Request( uint8_t type, uint8_t dest, const uint8_t *payload, uint8_t payloadLen )
{
    stack->phy.Clear_Tx();
    stack->phy.Inject_ExtFrame( type, dest, UtCrsf::HostAddr, payload, payloadLen );
    stack->Run();

    return ( stack->phy.txFramesCnt );
}


/**
 * \brief Reads parameter chunk, checks response header and appends chunk to entry
 *
 * \return Chunks remaining
 */
static uint8_t ReadChunk( uint8_t paramNumber, uint8_t chunkNumber )
{
    const uint8_t request[] = { paramNumber, chunkNumber };

    TEST_ASSERT_EQUAL_UINT8( 1u, Request( CRSF_FRAMETYPE_PARAM_READ, UtCrsf::OwnAddr, request, sizeof( request ) ) );

    const UtCrsf::parsedFrame_t frame = UtCrsf::Parse_Tx( stack->phy.txFrames[ 0 ], true );

    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_PARAM_ENTRY, frame.Type );
    TEST_ASSERT_EQUAL_HEX8( UtCrsf::HostAddr, frame.Dest );
    TEST_ASSERT_EQUAL_HEX8( UtCrsf::OwnAddr, frame.Orig );
    TEST_ASSERT_TRUE( 2u <= frame.PayloadLen );
    TEST_ASSERT_EQUAL_UINT8( paramNumber, frame.Payload[ 0 ] );

    if( 0u == chunkNumber )
    {
        entryLen = 0u;
    }

    memcpy( &entry[ entryLen ], &frame.Payload[ 2 ], frame.PayloadLen - 2u );
    entryLen = static_cast<uint16_t>( entryLen + frame.PayloadLen - 2u );

    return ( frame.Payload[ 1 ] );
}


/** Reads complete entry (all chunks) */
static void ReadEntry( uint8_t paramNumber )
{
    uint8_t chunk     = 0u;
    uint8_t remaining = ReadChunk( paramNumber, chunk );

    while( 0u < remaining )
    {
        chunk++;

        const uint8_t newRemaining = ReadChunk( paramNumber, chunk );

        TEST_ASSERT_EQUAL_UINT8( remaining - 1u, newRemaining );
        remaining = newRemaining;
    }
}


/** Writes parameter and checks that the actual entry (chunk 0) is answered */
static void Write( uint8_t paramNumber, const uint8_t *data, uint8_t dataLen )
{
    uint8_t request[ 64 ];

    request[ 0 ] = paramNumber;
    memcpy( &request[ 1 ], data, dataLen );

    TEST_ASSERT_EQUAL_UINT8( 1u, Request( CRSF_FRAMETYPE_PARAM_WRITE, UtCrsf::OwnAddr, request, static_cast<uint8_t>( dataLen + 1u ) ) );

    const UtCrsf::parsedFrame_t frame = UtCrsf::Parse_Tx( stack->phy.txFrames[ 0 ], true );

    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_PARAM_ENTRY, frame.Type );
    TEST_ASSERT_EQUAL_HEX8( UtCrsf::HostAddr, frame.Dest );
    TEST_ASSERT_EQUAL_UINT8( paramNumber, frame.Payload[ 0 ] );

    entryLen = static_cast<uint16_t>( frame.PayloadLen - 2u );
    memcpy( entry, &frame.Payload[ 2 ], entryLen );
}


/** Builds expected header of entry: parent, type, name */
static uint16_t ExpectedHeader( uint8_t *buffer, uint8_t parent, uint8_t type, const char *name )
{
    buffer[ 0 ] = parent;
    buffer[ 1 ] = type;
    strcpy( reinterpret_cast<char *>( &buffer[ 2 ] ), name );

    return ( static_cast<uint16_t>( 3u + strlen( name ) ) );
}


static uint16_t PutI32( uint8_t *buffer, uint16_t pos, int32_t value )
{
    const uint32_t raw = static_cast<uint32_t>( value );

    buffer[ pos++ ] = static_cast<uint8_t>( raw >> 24u );
    buffer[ pos++ ] = static_cast<uint8_t>( raw >> 16u );
    buffer[ pos++ ] = static_cast<uint8_t>( raw >> 8u );
    buffer[ pos++ ] = static_cast<uint8_t>( raw );

    return ( pos );
}


static uint16_t PutString( uint8_t *buffer, uint16_t pos, const char *text )
{
    strcpy( reinterpret_cast<char *>( &buffer[ pos ] ), text );

    return ( static_cast<uint16_t>( pos + strlen( text ) + 1u ) );
}

/* ======================== EXPORTED FUNCTIONS ============================== */

void setUp( void )
{
    gainValue       = 150;
    gainSetCnt      = 0u;
    modeValue       = 1;
    modeSetCnt      = 0u;
    strcpy( pilotName, "Quad" );
    pilotSetCnt     = 0u;
    calibStatus     = CRSF_CMD_READY;
    calibLastAction = -1;
    calibSetCnt     = 0u;
    longValue       = 0;

    params = new Crsf::AppLayer::Params( deviceInfo, paramsConfig, ParamsCount );

    static Crsf::FrameHandler *handlers[ 1 ];
    handlers[ 0 ] = params;

    stack = new UtCrsf::Stack( handlers, 1u );
}


void tearDown( void )
{
    delete stack;
    delete params;
}

/* -------------------------------- Ping ------------------------------------ */

/** Ping is answered by device information sent to origin */
void Ut_Params_Ping_DeviceInfo( void )
{
    TEST_ASSERT_EQUAL_UINT8( 1u, Request( CRSF_FRAMETYPE_PARAM_PING, UtCrsf::OwnAddr, nullptr, 0u ) );

    const UtCrsf::parsedFrame_t frame = UtCrsf::Parse_Tx( stack->phy.txFrames[ 0 ], true );

    const uint8_t expected[] = { 'E', 'm', 'B', 'i', ' ', 'F', 'C', 0x00,
                                 0x12, 0x34, 0x56, 0x78,
                                 0x00, 0x01, 0x00, 0x02,
                                 0x00, 0x03, 0x00, 0x04,
                                 ParamsCount, 7u };

    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_PARAM_DEVICE_INFO, frame.Type );
    TEST_ASSERT_EQUAL_HEX8( UtCrsf::HostAddr, frame.Dest );
    TEST_ASSERT_EQUAL_HEX8( UtCrsf::OwnAddr, frame.Orig );
    TEST_ASSERT_EQUAL_UINT8( sizeof( expected ), frame.PayloadLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, frame.Payload, sizeof( expected ) );
}


void Ut_Params_Ping_Broadcast( void )
{
    TEST_ASSERT_EQUAL_UINT8( 1u, Request( CRSF_FRAMETYPE_PARAM_PING, CRSF_ADDR_BROADCAST, nullptr, 0u ) );
}


void Ut_Params_Ping_OtherDeviceIgnored( void )
{
    TEST_ASSERT_EQUAL_UINT8( 0u, Request( CRSF_FRAMETYPE_PARAM_PING, CRSF_ADDR_VTX, nullptr, 0u ) );
}

/* -------------------------------- Read ------------------------------------ */

/** Parameter 0 is root folder named by device, children are parameters with parent 0 */
void Ut_Params_Read_RootFolder( void )
{
    uint8_t  expected[ 64 ];
    uint16_t len = ExpectedHeader( expected, 0u, CRSF_PARAM_FOLDER, "EmBi FC" );

    expected[ len++ ] = ParamSettings;
    expected[ len++ ] = ParamPilot;
    expected[ len++ ] = ParamVersion;
    expected[ len++ ] = ParamCalibrate;
    expected[ len++ ] = ParamLong;
    expected[ len++ ] = ParamReadOnly;
    expected[ len++ ] = 0xFFu;

    TEST_ASSERT_EQUAL_UINT8( 0u, ReadChunk( 0u, 0u ) );
    TEST_ASSERT_EQUAL_UINT16( len, entryLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, entry, len );
}


void Ut_Params_Read_Folder( void )
{
    uint8_t  expected[ 64 ];
    uint16_t len = ExpectedHeader( expected, 0u, CRSF_PARAM_FOLDER, "Settings" );

    expected[ len++ ] = ParamGain;
    expected[ len++ ] = ParamMode;
    expected[ len++ ] = ParamHidden;
    expected[ len++ ] = 0xFFu;

    ReadEntry( ParamSettings );

    TEST_ASSERT_EQUAL_UINT16( len, entryLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, entry, len );
}


void Ut_Params_Read_Float( void )
{
    uint8_t  expected[ 64 ];
    uint16_t len = ExpectedHeader( expected, ParamSettings, CRSF_PARAM_FLOAT, "Gain" );

    len = PutI32( expected, len, 150 );
    len = PutI32( expected, len, 0 );
    len = PutI32( expected, len, 500 );
    len = PutI32( expected, len, 100 );
    expected[ len++ ] = 2u;
    len = PutI32( expected, len, 5 );
    len = PutString( expected, len, "x" );

    ReadEntry( ParamGain );

    TEST_ASSERT_EQUAL_UINT16( len, entryLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, entry, len );
}


void Ut_Params_Read_TextSelection( void )
{
    uint8_t  expected[ 64 ];
    uint16_t len = ExpectedHeader( expected, ParamSettings, CRSF_PARAM_TEXT_SELECTION, "Mode" );

    len = PutString( expected, len, "Off;Low;High" );
    expected[ len++ ] = 1u;     /* value   */
    expected[ len++ ] = 0u;     /* min     */
    expected[ len++ ] = 2u;     /* max     */
    expected[ len++ ] = 0u;     /* default */
    len = PutString( expected, len, "" );

    ReadEntry( ParamMode );

    TEST_ASSERT_EQUAL_UINT16( len, entryLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, entry, len );
}


void Ut_Params_Read_String( void )
{
    uint8_t  expected[ 64 ];
    uint16_t len = ExpectedHeader( expected, 0u, CRSF_PARAM_STRING, "Pilot" );

    len = PutString( expected, len, "Quad" );
    expected[ len++ ] = 8u;

    ReadEntry( ParamPilot );

    TEST_ASSERT_EQUAL_UINT16( len, entryLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, entry, len );
}


void Ut_Params_Read_Info( void )
{
    uint8_t  expected[ 64 ];
    uint16_t len = ExpectedHeader( expected, 0u, CRSF_PARAM_INFO, "Version" );

    len = PutString( expected, len, "1.2.3" );

    ReadEntry( ParamVersion );

    TEST_ASSERT_EQUAL_UINT16( len, entryLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, entry, len );
}


void Ut_Params_Read_Command( void )
{
    uint8_t  expected[ 64 ];
    uint16_t len = ExpectedHeader( expected, 0u, CRSF_PARAM_COMMAND, "Calibrate" );

    expected[ len++ ] = CRSF_CMD_READY;
    expected[ len++ ] = 50u;
    len = PutString( expected, len, "" );

    ReadEntry( ParamCalibrate );

    TEST_ASSERT_EQUAL_UINT16( len, entryLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, entry, len );
}


/** Hidden parameter has bit 7 of data type set, failing getter reports default value */
void Ut_Params_Read_HiddenAndGetterFailure( void )
{
    paramsConfig[ ParamHidden - 1u ].VisibilityGetter = Get_HiddenVisibility;

    ReadEntry( ParamHidden );

    paramsConfig[ ParamHidden - 1u ].VisibilityGetter = nullptr;

    TEST_ASSERT_EQUAL_HEX8( CRSF_PARAM_FLOAT | 0x80u, entry[ 1 ] );

    /* Value behind "Hidden\0" is default 3 */
    const uint8_t value[] = { 0x00, 0x00, 0x00, 0x03 };

    TEST_ASSERT_EQUAL_HEX8_ARRAY( value, &entry[ 2u + 7u ], sizeof( value ) );
}


/** Not existing parameter is answered by OUT_OF_RANGE entry */
void Ut_Params_Read_OutOfRange( void )
{
    const uint8_t expected[] = { 0x00, CRSF_PARAM_OUT_OF_RANGE, 0x00 };

    TEST_ASSERT_EQUAL_UINT8( 0u, ReadChunk( ParamsCount + 1u, 0u ) );
    TEST_ASSERT_EQUAL_UINT16( sizeof( expected ), entryLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, entry, sizeof( expected ) );
}


/** Long entry is split into chunks of 56 bytes, chunks remaining counts down */
void Ut_Params_Read_Chunks( void )
{
    uint8_t  expected[ 256 ];
    uint16_t len = ExpectedHeader( expected, 0u, CRSF_PARAM_TEXT_SELECTION, "Long" );

    len = PutString( expected, len, longOptions );
    expected[ len++ ] = 0u;
    expected[ len++ ] = 0u;
    expected[ len++ ] = 5u;
    expected[ len++ ] = 0u;
    len = PutString( expected, len, "unit" );

    TEST_ASSERT_TRUE( 2u * Crsf::AppLayer::Params::chunkSizeMax < len );

    TEST_ASSERT_EQUAL_UINT8( 2u, ReadChunk( ParamLong, 0u ) );
    TEST_ASSERT_EQUAL_UINT16( Crsf::AppLayer::Params::chunkSizeMax, entryLen );
    TEST_ASSERT_EQUAL_UINT16( CRSF_FRAME_MAX_LEN, stack->phy.txFrames[ 0 ].Length );

    TEST_ASSERT_EQUAL_UINT8( 1u, ReadChunk( ParamLong, 1u ) );
    TEST_ASSERT_EQUAL_UINT8( 0u, ReadChunk( ParamLong, 2u ) );

    TEST_ASSERT_EQUAL_UINT16( len, entryLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, entry, len );
}


/** Following chunks are sent from cached entry - value change between chunks does not mix entries */
void Ut_Params_Read_ChunksConsistent( void )
{
    (void)ReadChunk( ParamLong, 0u );

    longValue = 3;

    (void)ReadChunk( ParamLong, 1u );
    (void)ReadChunk( ParamLong, 2u );

    /* Value is behind options string: header (2 + 5) + options + terminator */
    const uint16_t valuePos = static_cast<uint16_t>( 7u + sizeof( longOptions ) );

    TEST_ASSERT_EQUAL_UINT8( 0u, entry[ valuePos ] );

    /* New read starting with chunk 0 provides new value */
    ReadEntry( ParamLong );

    TEST_ASSERT_EQUAL_UINT8( 3u, entry[ valuePos ] );
}


/** Not existing chunk is not answered */
void Ut_Params_Read_ChunkOutOfRange( void )
{
    const uint8_t request[] = { ParamGain, 1u };

    TEST_ASSERT_EQUAL_UINT8( 0u, Request( CRSF_FRAMETYPE_PARAM_READ, UtCrsf::OwnAddr, request, sizeof( request ) ) );
}


/** Read request without chunk number is ignored */
void Ut_Params_Read_ShortRequestIgnored( void )
{
    const uint8_t request[] = { ParamGain };

    TEST_ASSERT_EQUAL_UINT8( 0u, Request( CRSF_FRAMETYPE_PARAM_READ, UtCrsf::OwnAddr, request, sizeof( request ) ) );
}

/* -------------------------------- Write ----------------------------------- */

/** FLOAT write in range calls setter and answers new value */
void Ut_Params_Write_Float( void )
{
    const uint8_t value[] = { 0x00, 0x00, 0x01, 0x2C };     /* 300 */

    Write( ParamGain, value, sizeof( value ) );

    TEST_ASSERT_EQUAL_UINT32( 1u, gainSetCnt );
    TEST_ASSERT_EQUAL_INT32( 300, gainValue );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( value, &entry[ 2u + 5u ], sizeof( value ) );
}


/** FLOAT write out of range or with short data is not passed to setter */
void Ut_Params_Write_FloatOutOfRange( void )
{
    const uint8_t tooHigh[] = { 0x00, 0x00, 0x01, 0xF5 };   /* 501 */
    const uint8_t negative[] = { 0xFF, 0xFF, 0xFF, 0xFF };  /* -1  */
    const uint8_t shortData[] = { 0x00, 0x01 };

    Write( ParamGain, tooHigh, sizeof( tooHigh ) );
    Write( ParamGain, negative, sizeof( negative ) );
    Write( ParamGain, shortData, sizeof( shortData ) );

    TEST_ASSERT_EQUAL_UINT32( 0u, gainSetCnt );
    TEST_ASSERT_EQUAL_INT32( 150, gainValue );

    const uint8_t actual[] = { 0x00, 0x00, 0x00, 0x96 };

    TEST_ASSERT_EQUAL_HEX8_ARRAY( actual, &entry[ 2u + 5u ], sizeof( actual ) );
}


void Ut_Params_Write_TextSelection( void )
{
    const uint8_t valid[]   = { 2u };
    const uint8_t invalid[] = { 3u };

    Write( ParamMode, valid, sizeof( valid ) );

    TEST_ASSERT_EQUAL_INT32( 2, modeValue );

    Write( ParamMode, invalid, sizeof( invalid ) );

    TEST_ASSERT_EQUAL_UINT32( 1u, modeSetCnt );
    TEST_ASSERT_EQUAL_INT32( 2, modeValue );
}


void Ut_Params_Write_String( void )
{
    const uint8_t value[] = { 'R', 'a', 'c', 'e', 'r', 0x00 };

    Write( ParamPilot, value, sizeof( value ) );

    TEST_ASSERT_EQUAL_UINT32( 1u, pilotSetCnt );
    TEST_ASSERT_EQUAL_STRING( "Racer", pilotName );
    TEST_ASSERT_EQUAL_STRING( "Racer", reinterpret_cast<const char *>( &entry[ 2u + 6u ] ) );
}


/** Too long string is truncated to maximal length of parameter */
void Ut_Params_Write_StringTruncated( void )
{
    const uint8_t value[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 0x00 };

    Write( ParamPilot, value, sizeof( value ) );

    TEST_ASSERT_EQUAL_STRING( "ABCDEFGH", pilotName );
}


/** Command START is passed to setter, the answer contains new status */
void Ut_Params_Write_CommandStart( void )
{
    const uint8_t start[] = { CRSF_CMD_START };

    Write( ParamCalibrate, start, sizeof( start ) );

    TEST_ASSERT_EQUAL_UINT32( 1u, calibSetCnt );
    TEST_ASSERT_EQUAL_INT32( CRSF_CMD_START, calibLastAction );

    /* Status behind "Calibrate\0", then timeout and info */
    TEST_ASSERT_EQUAL_UINT8( CRSF_CMD_PROGRESS, entry[ 2u + 10u ] );
    TEST_ASSERT_EQUAL_UINT8( 50u, entry[ 2u + 11u ] );
    TEST_ASSERT_EQUAL_STRING( "Calibrating", reinterpret_cast<const char *>( &entry[ 2u + 12u ] ) );
}


/** POLL only answers actual status, feedback values are not accepted */
void Ut_Params_Write_CommandPollAndInvalidAction( void )
{
    const uint8_t poll[]     = { CRSF_CMD_POLL };
    const uint8_t progress[] = { CRSF_CMD_PROGRESS };
    const uint8_t cancel[]   = { CRSF_CMD_CANCEL };

    Write( ParamCalibrate, poll, sizeof( poll ) );
    Write( ParamCalibrate, progress, sizeof( progress ) );

    TEST_ASSERT_EQUAL_UINT32( 0u, calibSetCnt );

    Write( ParamCalibrate, cancel, sizeof( cancel ) );

    TEST_ASSERT_EQUAL_UINT32( 1u, calibSetCnt );
    TEST_ASSERT_EQUAL_INT32( CRSF_CMD_CANCEL, calibLastAction );
}


/** Read only parameter, folder, info and root folder are answered without change */
void Ut_Params_Write_NotWritable( void )
{
    const uint8_t value[] = { 0x00, 0x00, 0x00, 0x01 };

    Write( ParamReadOnly, value, sizeof( value ) );
    Write( ParamSettings, value, sizeof( value ) );
    Write( ParamVersion, value, sizeof( value ) );
    Write( 0u, value, sizeof( value ) );

    TEST_ASSERT_EQUAL_INT32( 150, gainValue );
    TEST_ASSERT_EQUAL_HEX8( CRSF_PARAM_FOLDER, entry[ 1 ] );
}


/** Write of not existing parameter is answered by OUT_OF_RANGE entry */
void Ut_Params_Write_OutOfRange( void )
{
    const uint8_t value[] = { 0x01 };

    Write( ParamsCount + 1u, value, sizeof( value ) );

    TEST_ASSERT_EQUAL_HEX8( CRSF_PARAM_OUT_OF_RANGE, entry[ 1 ] );
}

/* ---------------------------- Entry update -------------------------------- */

/** Requested entry update is sent in next task */
void Ut_Params_RequestEntryUpdate( void )
{
    stack->phy.Clear_Tx();

    calibStatus = CRSF_CMD_READY;

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, params->Request_EntryUpdate( ParamCalibrate, UtCrsf::HostAddr ) );
    TEST_ASSERT_EQUAL_UINT8( 0u, stack->phy.txFramesCnt );

    stack->Run();

    TEST_ASSERT_EQUAL_UINT8( 1u, stack->phy.txFramesCnt );

    const UtCrsf::parsedFrame_t frame = UtCrsf::Parse_Tx( stack->phy.txFrames[ 0 ], true );

    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_PARAM_ENTRY, frame.Type );
    TEST_ASSERT_EQUAL_HEX8( UtCrsf::HostAddr, frame.Dest );
    TEST_ASSERT_EQUAL_UINT8( ParamCalibrate, frame.Payload[ 0 ] );

    /* Sent only once */
    stack->Run();
    TEST_ASSERT_EQUAL_UINT8( 1u, stack->phy.txFramesCnt );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, params->Request_EntryUpdate( ParamsCount + 1u, UtCrsf::HostAddr ) );
}
