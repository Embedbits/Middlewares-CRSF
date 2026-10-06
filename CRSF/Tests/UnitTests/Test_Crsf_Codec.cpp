/**
 * \author Mr.Nobody
 * \file Test_Crsf_Codec.cpp
 * \ingroup UnitTests
 * \brief Unit tests of CRSF payload encoding and decoding.
 */

/* ============================= INCLUDES =================================== */
#include "unity.h"                          /* Unity testing framework        */
#include "Crsf_App_Codec.hpp"               /* Code under test                */
#include <string.h>                         /* strcmp, memset                 */
/* ========================== LOCAL VARIABLES =============================== */

static uint8_t payload[ CRSF_PAYLOAD_MAX_LEN + 8u ];

/* ======================== EXPORTED FUNCTIONS ============================== */

void setUp( void )
{
    memset( payload, 0xEE, sizeof( payload ) );
}


void tearDown( void )
{
}

/* ----------------------------- Header type -------------------------------- */

void Ut_Codec_HeaderType( void )
{
    TEST_ASSERT_EQUAL( CRSF_HEADER_SHORT,    Crsf_Get_HeaderType( CRSF_FRAMETYPE_GPS ) );
    TEST_ASSERT_EQUAL( CRSF_HEADER_SHORT,    Crsf_Get_HeaderType( CRSF_FRAMETYPE_RC_CHANNELS_PACKED ) );
    TEST_ASSERT_EQUAL( CRSF_HEADER_SHORT,    Crsf_Get_HeaderType( 0x27u ) );
    TEST_ASSERT_EQUAL( CRSF_HEADER_EXTENDED, Crsf_Get_HeaderType( CRSF_FRAMETYPE_PARAM_PING ) );
    TEST_ASSERT_EQUAL( CRSF_HEADER_EXTENDED, Crsf_Get_HeaderType( CRSF_FRAMETYPE_COMMAND ) );
    TEST_ASSERT_EQUAL( CRSF_HEADER_EXTENDED, Crsf_Get_HeaderType( CRSF_FRAMETYPE_REMOTE ) );
    TEST_ASSERT_EQUAL( CRSF_HEADER_EXTENDED, Crsf_Get_HeaderType( CRSF_FRAMETYPE_MSP_REQUEST ) );
    TEST_ASSERT_EQUAL( CRSF_HEADER_SHORT,    Crsf_Get_HeaderType( CRSF_FRAMETYPE_LOGGING ) );
    TEST_ASSERT_EQUAL( CRSF_HEADER_SHORT,    Crsf_Get_HeaderType( CRSF_FRAMETYPE_MAVLINK_ENVELOPE ) );
    TEST_ASSERT_EQUAL( CRSF_HEADER_SHORT,    Crsf_Get_HeaderType( CRSF_FRAMETYPE_MAVLINK_SYS_STATUS ) );
}

/* ------------------------- Writer and reader ------------------------------ */

void Ut_Codec_Writer_BigEndian( void )
{
    Crsf::PayloadWriter writer( payload, 16u );

    writer.Put_U8 ( 0x11u );
    writer.Put_U16( 0x2233u );
    writer.Put_U24( 0x00445566u );
    writer.Put_U32( 0x778899AAu );
    writer.Put_String( "AB" );

    const uint8_t expected[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 'A', 'B', 0x00 };

    TEST_ASSERT_EQUAL_UINT8( sizeof( expected ), writer.Get_Length() );
    TEST_ASSERT_FALSE( writer.Is_Overflow() );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, payload, sizeof( expected ) );
}


void Ut_Codec_Writer_OverflowIgnored( void )
{
    Crsf::PayloadWriter writer( payload, 3u );

    writer.Put_U16( 0x1122u );
    writer.Put_U16( 0x3344u );

    TEST_ASSERT_EQUAL_UINT8( 3u, writer.Get_Length() );
    TEST_ASSERT_TRUE( writer.Is_Overflow() );
    TEST_ASSERT_EQUAL_HEX8( 0xEEu, payload[ 3 ] );
}


void Ut_Codec_Reader_BigEndianAndUnderflow( void )
{
    const uint8_t data[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 'X', 'Y', 0x00, 0x77 };
    char          text[ 8 ];

    Crsf::PayloadReader reader( data, sizeof( data ) );

    TEST_ASSERT_EQUAL_HEX16( 0x1122u,     reader.Get_U16() );
    TEST_ASSERT_EQUAL_HEX32( 0x33445566u, reader.Get_U32() );
    reader.Get_String( text, sizeof( text ) );
    TEST_ASSERT_EQUAL_STRING( "XY", text );
    TEST_ASSERT_EQUAL_UINT8( 1u, reader.Get_Remaining() );
    TEST_ASSERT_FALSE( reader.Is_Underflow() );

    TEST_ASSERT_EQUAL_HEX16( 0x7700u, reader.Get_U16() );
    TEST_ASSERT_TRUE( reader.Is_Underflow() );
}


/** String without terminator ends with payload, too long string is truncated */
void Ut_Codec_Reader_StringTruncation( void )
{
    const uint8_t data[] = { 'A', 'B', 'C', 'D', 'E' };
    char          text[ 4 ];

    Crsf::PayloadReader reader( data, sizeof( data ) );

    reader.Get_String( text, sizeof( text ) );

    TEST_ASSERT_EQUAL_STRING( "ABC", text );
    TEST_ASSERT_EQUAL_UINT8( 0u, reader.Get_Remaining() );
    TEST_ASSERT_FALSE( reader.Is_Underflow() );
}

/* ------------------------------ RC channels ------------------------------- */

/** All channels at center 992 give well known pattern of 11 bytes repeated twice */
void Ut_Codec_RcChannels_CenterPattern( void )
{
    crsf_RcChannels_t channels = {};

    for( uint8_t i = 0u; i < CRSF_RC_CHANNELS_CNT; i++ )
    {
        channels.Channels[ i ] = 992u;
    }

    const uint8_t pattern[] = { 0xE0, 0x03, 0x1F, 0xF8, 0xC0, 0x07, 0x3E, 0xF0, 0x81, 0x0F, 0x7C };

    TEST_ASSERT_EQUAL_UINT8( 22u, Crsf::Codec::Encode_RcChannels( channels, payload ) );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( pattern, &payload[ 0 ],  sizeof( pattern ) );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( pattern, &payload[ 11 ], sizeof( pattern ) );
    TEST_ASSERT_EQUAL_HEX8( 0xEEu, payload[ 22 ] );
}


/** Channel 1 is in LSB of first byte, channel 16 in MSB of last byte */
void Ut_Codec_RcChannels_BitOrder( void )
{
    crsf_RcChannels_t channels = {};

    channels.Channels[ 0 ]  = 0x7FFu;
    channels.Channels[ 15 ] = 0x400u;

    (void)Crsf::Codec::Encode_RcChannels( channels, payload );

    TEST_ASSERT_EQUAL_HEX8( 0xFFu, payload[ 0 ] );
    TEST_ASSERT_EQUAL_HEX8( 0x07u, payload[ 1 ] );
    TEST_ASSERT_EQUAL_HEX8( 0x80u, payload[ 21 ] );
}


void Ut_Codec_RcChannels_RoundTrip( void )
{
    crsf_RcChannels_t channels = {};
    crsf_RcChannels_t decoded  = {};

    for( uint8_t i = 0u; i < CRSF_RC_CHANNELS_CNT; i++ )
    {
        channels.Channels[ i ] = static_cast<uint16_t>( 172u + ( i * 109u ) );
    }

    const crsf_MsgLength_t len = Crsf::Codec::Encode_RcChannels( channels, payload );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_RcChannels( payload, len, decoded ) );
    TEST_ASSERT_EQUAL_UINT16_ARRAY( channels.Channels, decoded.Channels, CRSF_RC_CHANNELS_CNT );
}


/** Values above 11 bits are limited */
void Ut_Codec_RcChannels_ValueLimited( void )
{
    crsf_RcChannels_t channels = {};
    crsf_RcChannels_t decoded  = {};

    channels.Channels[ 1 ] = 0xFFFFu;

    (void)Crsf::Codec::Encode_RcChannels( channels, payload );
    (void)Crsf::Codec::Decode_RcChannels( payload, 22u, decoded );

    TEST_ASSERT_EQUAL_UINT16( 0x7FFu, decoded.Channels[ 1 ] );
    TEST_ASSERT_EQUAL_UINT16( 0u,     decoded.Channels[ 0 ] );
    TEST_ASSERT_EQUAL_UINT16( 0u,     decoded.Channels[ 2 ] );
}


void Ut_Codec_RcChannels_ShortPayloadRejected( void )
{
    crsf_RcChannels_t decoded = {};

    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, Crsf::Codec::Decode_RcChannels( payload, 21u, decoded ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, Crsf::Codec::Decode_RcChannels( nullptr, 22u, decoded ) );
}


void Ut_Codec_ChannelConversion( void )
{
    TEST_ASSERT_EQUAL_INT32( 1500, Crsf::Codec::Get_ChannelUs( 992u ) );
    TEST_ASSERT_EQUAL_INT32( 988,  Crsf::Codec::Get_ChannelUs( 172u ) );
    TEST_ASSERT_EQUAL_INT32( 2011, Crsf::Codec::Get_ChannelUs( 1811u ) );

    TEST_ASSERT_EQUAL_UINT16( 992u,  Crsf::Codec::Get_ChannelValue( 1500 ) );
    TEST_ASSERT_EQUAL_UINT16( 1792u, Crsf::Codec::Get_ChannelValue( 2000 ) );
    TEST_ASSERT_EQUAL_UINT16( 192u,  Crsf::Codec::Get_ChannelValue( 1000 ) );
    TEST_ASSERT_EQUAL_UINT16( 0u,    Crsf::Codec::Get_ChannelValue( 0 ) );
    TEST_ASSERT_EQUAL_UINT16( 2047u, Crsf::Codec::Get_ChannelValue( 3000 ) );
}

/* ------------------------------ Telemetry --------------------------------- */

void Ut_Codec_Gps_LayoutAndRoundTrip( void )
{
    const crsf_Gps_t gps     = { 0x12345678, -1, 0x0102u, 0x0304u, 0x0506u, 12u };
    crsf_Gps_t       decoded = {};

    const uint8_t expected[] = { 0x12, 0x34, 0x56, 0x78, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 12u };

    TEST_ASSERT_EQUAL_UINT8( sizeof( expected ), Crsf::Codec::Encode_Gps( gps, payload ) );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, payload, sizeof( expected ) );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_Gps( payload, sizeof( expected ), decoded ) );
    TEST_ASSERT_EQUAL_INT32( gps.Latitude, decoded.Latitude );
    TEST_ASSERT_EQUAL_INT32( gps.Longitude, decoded.Longitude );
    TEST_ASSERT_EQUAL_UINT16( gps.GroundSpeed, decoded.GroundSpeed );
    TEST_ASSERT_EQUAL_UINT16( gps.Heading, decoded.Heading );
    TEST_ASSERT_EQUAL_UINT16( gps.Altitude, decoded.Altitude );
    TEST_ASSERT_EQUAL_UINT8( gps.Satellites, decoded.Satellites );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, Crsf::Codec::Decode_Gps( payload, sizeof( expected ) - 1u, decoded ) );
}


void Ut_Codec_GpsTime_RoundTrip( void )
{
    const crsf_GpsTime_t time    = { 2024, 11, 19, 13, 45, 59, 999 };
    crsf_GpsTime_t       decoded = {};

    const crsf_MsgLength_t len = Crsf::Codec::Encode_GpsTime( time, payload );

    TEST_ASSERT_EQUAL_UINT8( Crsf::Codec::gpsTimeLen, len );
    TEST_ASSERT_EQUAL_HEX8( 0x07u, payload[ 0 ] );
    TEST_ASSERT_EQUAL_HEX8( 0xE8u, payload[ 1 ] );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_GpsTime( payload, len, decoded ) );
    TEST_ASSERT_EQUAL_INT16( 2024, decoded.Year );
    TEST_ASSERT_EQUAL_UINT8( 11u, decoded.Month );
    TEST_ASSERT_EQUAL_UINT8( 19u, decoded.Day );
    TEST_ASSERT_EQUAL_UINT8( 13u, decoded.Hour );
    TEST_ASSERT_EQUAL_UINT8( 45u, decoded.Minute );
    TEST_ASSERT_EQUAL_UINT8( 59u, decoded.Second );
    TEST_ASSERT_EQUAL_UINT16( 999u, decoded.Millisecond );
}


void Ut_Codec_GpsExtended_RoundTrip( void )
{
    const crsf_GpsExtended_t ext     = { 3u, -100, 200, -300, 15, 25, 450, 120, 180, 0u, 9u, 14u };
    crsf_GpsExtended_t       decoded = {};

    const crsf_MsgLength_t len = Crsf::Codec::Encode_GpsExtended( ext, payload );

    TEST_ASSERT_EQUAL_UINT8( Crsf::Codec::gpsExtendedLen, len );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_GpsExtended( payload, len, decoded ) );
    TEST_ASSERT_EQUAL_UINT8( 3u, decoded.FixType );
    TEST_ASSERT_EQUAL_INT16( -100, decoded.NSpeed );
    TEST_ASSERT_EQUAL_INT16( 200, decoded.ESpeed );
    TEST_ASSERT_EQUAL_INT16( -300, decoded.VSpeed );
    TEST_ASSERT_EQUAL_INT16( 15, decoded.HSpeedAcc );
    TEST_ASSERT_EQUAL_INT16( 25, decoded.TrackAcc );
    TEST_ASSERT_EQUAL_INT16( 450, decoded.AltEllipsoid );
    TEST_ASSERT_EQUAL_INT16( 120, decoded.HAcc );
    TEST_ASSERT_EQUAL_INT16( 180, decoded.VAcc );
    TEST_ASSERT_EQUAL_UINT8( 9u, decoded.HDop );
    TEST_ASSERT_EQUAL_UINT8( 14u, decoded.VDop );
}


void Ut_Codec_Vario_RoundTrip( void )
{
    const crsf_Vario_t vario   = { -150 };
    crsf_Vario_t       decoded = {};

    TEST_ASSERT_EQUAL_UINT8( 2u, Crsf::Codec::Encode_Vario( vario, payload ) );
    TEST_ASSERT_EQUAL_HEX8( 0xFFu, payload[ 0 ] );
    TEST_ASSERT_EQUAL_HEX8( 0x6Au, payload[ 1 ] );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_Vario( payload, 2u, decoded ) );
    TEST_ASSERT_EQUAL_INT16( -150, decoded.VSpeed );
}


/** Battery capacity is 24 bit big endian */
void Ut_Codec_Battery_LayoutAndRoundTrip( void )
{
    const crsf_Battery_t battery = { 168, 125, 0x012345u, 87u };
    crsf_Battery_t       decoded = {};

    const uint8_t expected[] = { 0x00, 0xA8, 0x00, 0x7D, 0x01, 0x23, 0x45, 87u };

    TEST_ASSERT_EQUAL_UINT8( sizeof( expected ), Crsf::Codec::Encode_Battery( battery, payload ) );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, payload, sizeof( expected ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_Battery( payload, sizeof( expected ), decoded ) );
    TEST_ASSERT_EQUAL_INT16( 168, decoded.Voltage );
    TEST_ASSERT_EQUAL_INT16( 125, decoded.Current );
    TEST_ASSERT_EQUAL_HEX32( 0x012345u, decoded.CapacityUsed );
    TEST_ASSERT_EQUAL_UINT8( 87u, decoded.Remaining );
}


void Ut_Codec_Battery_CapacityLimitedTo24Bits( void )
{
    const crsf_Battery_t battery = { 0, 0, 0x01000000u, 0u };
    crsf_Battery_t       decoded = {};

    (void)Crsf::Codec::Encode_Battery( battery, payload );
    (void)Crsf::Codec::Decode_Battery( payload, Crsf::Codec::batteryLen, decoded );

    TEST_ASSERT_EQUAL_HEX32( 0x00FFFFFFu, decoded.CapacityUsed );
}


void Ut_Codec_BaroAltitude_RoundTrip( void )
{
    const crsf_BaroAltitude_t baro    = { 0x88E5u, -26 };
    crsf_BaroAltitude_t       decoded = {};

    TEST_ASSERT_EQUAL_UINT8( 3u, Crsf::Codec::Encode_BaroAltitude( baro, payload ) );
    TEST_ASSERT_EQUAL_HEX8( 0x88u, payload[ 0 ] );
    TEST_ASSERT_EQUAL_HEX8( 0xE5u, payload[ 1 ] );
    TEST_ASSERT_EQUAL_HEX8( 0xE6u, payload[ 2 ] );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_BaroAltitude( payload, 3u, decoded ) );
    TEST_ASSERT_EQUAL_HEX16( 0x88E5u, decoded.AltitudePacked );
    TEST_ASSERT_EQUAL_INT8( -26, decoded.VerticalSpeedPacked );
}


void Ut_Codec_Heartbeat_RoundTrip( void )
{
    const crsf_Heartbeat_t heartbeat = { CRSF_ADDR_FLIGHT_CONTROLLER };
    crsf_Heartbeat_t       decoded   = {};

    TEST_ASSERT_EQUAL_UINT8( 2u, Crsf::Codec::Encode_Heartbeat( heartbeat, payload ) );
    TEST_ASSERT_EQUAL_HEX8( 0x00u, payload[ 0 ] );
    TEST_ASSERT_EQUAL_HEX8( 0xC8u, payload[ 1 ] );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_Heartbeat( payload, 2u, decoded ) );
    TEST_ASSERT_EQUAL_INT16( CRSF_ADDR_FLIGHT_CONTROLLER, decoded.OriginAddress );
}


/** VTX pit mode bit fields: pit_mode bit 0, control bits 1-2, switch bits 3-6 */
void Ut_Codec_VtxTelemetry_BitFields( void )
{
    const crsf_VtxTelemetry_t vtx     = { CRSF_ADDR_VTX, 25u, 5800u, 1u, 2u, 5u };
    crsf_VtxTelemetry_t       decoded = {};

    const uint8_t expected[] = { CRSF_ADDR_VTX, 25u, 0x16, 0xA8, 0x2D };

    TEST_ASSERT_EQUAL_UINT8( sizeof( expected ), Crsf::Codec::Encode_VtxTelemetry( vtx, payload ) );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, payload, sizeof( expected ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_VtxTelemetry( payload, sizeof( expected ), decoded ) );
    TEST_ASSERT_EQUAL_UINT16( 5800u, decoded.FrequencyMhz );
    TEST_ASSERT_EQUAL_UINT8( 1u, decoded.PitMode );
    TEST_ASSERT_EQUAL_UINT8( 2u, decoded.PitModeControl );
    TEST_ASSERT_EQUAL_UINT8( 5u, decoded.PitModeSwitch );
}


void Ut_Codec_LinkStatistics_RoundTrip( void )
{
    const crsf_LinkStatistics_t stats   = { 60u, 70u, 100u, -5, 1u, 2u, 3u, 80u, 99u, 7 };
    crsf_LinkStatistics_t       decoded = {};

    const uint8_t expected[] = { 60u, 70u, 100u, 0xFB, 1u, 2u, 3u, 80u, 99u, 7u };

    TEST_ASSERT_EQUAL_UINT8( sizeof( expected ), Crsf::Codec::Encode_LinkStatistics( stats, payload ) );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, payload, sizeof( expected ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_LinkStatistics( payload, sizeof( expected ), decoded ) );
    TEST_ASSERT_EQUAL_INT8( -5, decoded.UpSnr );
    TEST_ASSERT_EQUAL_UINT8( 99u, decoded.DownLinkQuality );
    TEST_ASSERT_EQUAL_INT8( 7, decoded.DownSnr );
}


void Ut_Codec_LinkStatisticsRxTx_RoundTrip( void )
{
    const crsf_LinkStatisticsRx_t rx = { 70u, 80u, 95u, -3, 20u };
    const crsf_LinkStatisticsTx_t tx = { 71u, 81u, 96u, 4, 21u, 50u };
    crsf_LinkStatisticsRx_t       decodedRx = {};
    crsf_LinkStatisticsTx_t       decodedTx = {};

    crsf_MsgLength_t len = Crsf::Codec::Encode_LinkStatisticsRx( rx, payload );

    TEST_ASSERT_EQUAL_UINT8( 5u, len );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_LinkStatisticsRx( payload, len, decodedRx ) );
    TEST_ASSERT_EQUAL_INT8( -3, decodedRx.Snr );
    TEST_ASSERT_EQUAL_UINT8( 20u, decodedRx.RfPowerDb );

    len = Crsf::Codec::Encode_LinkStatisticsTx( tx, payload );

    TEST_ASSERT_EQUAL_UINT8( 6u, len );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_LinkStatisticsTx( payload, len, decodedTx ) );
    TEST_ASSERT_EQUAL_UINT8( 50u, decodedTx.Fps );
    TEST_ASSERT_EQUAL_UINT8( 96u, decodedTx.LinkQuality );
}


void Ut_Codec_Attitude_RoundTrip( void )
{
    const crsf_Attitude_t attitude = { -31416, 15708, 1 };
    crsf_Attitude_t       decoded  = {};

    TEST_ASSERT_EQUAL_UINT8( 6u, Crsf::Codec::Encode_Attitude( attitude, payload ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_Attitude( payload, 6u, decoded ) );
    TEST_ASSERT_EQUAL_INT16( -31416, decoded.Pitch );
    TEST_ASSERT_EQUAL_INT16( 15708, decoded.Roll );
    TEST_ASSERT_EQUAL_INT16( 1, decoded.Yaw );
}


void Ut_Codec_MavLinkFc_RoundTrip( void )
{
    const crsf_MavLinkFc_t fc      = { 1234, 0x81u, 0xDEADBEEFu, 3u, 2u };
    crsf_MavLinkFc_t       decoded = {};

    TEST_ASSERT_EQUAL_UINT8( 9u, Crsf::Codec::Encode_MavLinkFc( fc, payload ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_MavLinkFc( payload, 9u, decoded ) );
    TEST_ASSERT_EQUAL_INT16( 1234, decoded.Airspeed );
    TEST_ASSERT_EQUAL_HEX8( 0x81u, decoded.BaseMode );
    TEST_ASSERT_EQUAL_HEX32( 0xDEADBEEFu, decoded.CustomMode );
    TEST_ASSERT_EQUAL_UINT8( 3u, decoded.AutopilotType );
    TEST_ASSERT_EQUAL_UINT8( 2u, decoded.FirmwareType );
}


void Ut_Codec_FlightMode_RoundTrip( void )
{
    crsf_FlightMode_t decoded = {};

    TEST_ASSERT_EQUAL_UINT8( 5u, Crsf::Codec::Encode_FlightMode( "ACRO", payload ) );
    TEST_ASSERT_EQUAL_HEX8( 0x00u, payload[ 4 ] );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_FlightMode( payload, 5u, decoded ) );
    TEST_ASSERT_EQUAL_STRING( "ACRO", decoded.Name );

    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, Crsf::Codec::Decode_FlightMode( payload, 0u, decoded ) );
}


/** Too long flight mode is truncated and terminated, it fits into one frame */
void Ut_Codec_FlightMode_Truncated( void )
{
    char name[ 80 ];

    memset( name, 'M', sizeof( name ) - 1u );
    name[ sizeof( name ) - 1u ] = '\0';

    TEST_ASSERT_EQUAL_UINT8( CRSF_PAYLOAD_MAX_LEN, Crsf::Codec::Encode_FlightMode( name, payload ) );
    TEST_ASSERT_EQUAL_HEX8( 'M',  payload[ CRSF_PAYLOAD_MAX_LEN - 2u ] );
    TEST_ASSERT_EQUAL_HEX8( 0x00, payload[ CRSF_PAYLOAD_MAX_LEN - 1u ] );
    TEST_ASSERT_EQUAL_HEX8( 0xEE, payload[ CRSF_PAYLOAD_MAX_LEN ] );
}


void Ut_Codec_EspNow_RoundTrip( void )
{
    crsf_EspNow_t espNow  = {};
    crsf_EspNow_t decoded = {};

    espNow.SeatPosition = 2u;
    espNow.CurrentLap   = 7u;
    memcpy( espNow.LapTime1, "0:12.345      ", CRSF_ESP_NOW_LAP_TIME_LEN );
    memcpy( espNow.LapTime2, "0:11.999      ", CRSF_ESP_NOW_LAP_TIME_LEN );
    memcpy( espNow.FreeText, "BEST LAP           ", CRSF_ESP_NOW_FREE_TEXT_LEN );

    TEST_ASSERT_EQUAL_UINT8( 52u, Crsf::Codec::Encode_EspNow( espNow, payload ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_EspNow( payload, 52u, decoded ) );
    TEST_ASSERT_EQUAL_UINT8( 7u, decoded.CurrentLap );
    TEST_ASSERT_EQUAL_MEMORY( espNow.LapTime2, decoded.LapTime2, CRSF_ESP_NOW_LAP_TIME_LEN );
    TEST_ASSERT_EQUAL_MEMORY( espNow.FreeText, decoded.FreeText, CRSF_ESP_NOW_FREE_TEXT_LEN );
}


void Ut_Codec_TimingCorrection_RoundTrip( void )
{
    const crsf_TimingCorrection_t timing  = { 40000u, -250 };
    crsf_TimingCorrection_t       decoded = {};

    const crsf_MsgLength_t len = Crsf::Codec::Encode_TimingCorrection( timing, payload );

    TEST_ASSERT_EQUAL_UINT8( 9u, len );
    TEST_ASSERT_EQUAL_HEX8( CRSF_REMOTE_SUBTYPE_TIMING_CORRECTION, payload[ 0 ] );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_TimingCorrection( payload, len, decoded ) );
    TEST_ASSERT_EQUAL_UINT32( 40000u, decoded.UpdateInterval );
    TEST_ASSERT_EQUAL_INT32( -250, decoded.Offset );

    /* Other sub-type is rejected */
    payload[ 0 ] = 0x01u;
    TEST_ASSERT_EQUAL( CRSF_REQUEST_ERROR, Crsf::Codec::Decode_TimingCorrection( payload, len, decoded ) );
}


void Ut_Codec_MavLinkSysStatus_RoundTrip( void )
{
    const crsf_MavLinkSysStatus_t status  = { 0x01020304u, 0x05060708u, 0x090A0B0Cu };
    crsf_MavLinkSysStatus_t       decoded = {};

    TEST_ASSERT_EQUAL_UINT8( 12u, Crsf::Codec::Encode_MavLinkSysStatus( status, payload ) );
    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_MavLinkSysStatus( payload, 12u, decoded ) );
    TEST_ASSERT_EQUAL_HEX32( 0x090A0B0Cu, decoded.SensorHealth );
}


/** Extra bytes behind known fields (newer protocol version) are ignored */
void Ut_Codec_Decode_ExtraBytesIgnored( void )
{
    const uint8_t data[] = { 0xFF, 0x9C, 0x12, 0x34 };
    crsf_Vario_t  decoded = {};

    TEST_ASSERT_EQUAL( CRSF_REQUEST_OK, Crsf::Codec::Decode_Vario( data, sizeof( data ), decoded ) );
    TEST_ASSERT_EQUAL_INT16( -100, decoded.VSpeed );
}

/* -------------------------- Barometric packing ---------------------------- */

void Ut_Codec_AltitudePacking( void )
{
    TEST_ASSERT_EQUAL_HEX16( 10000u,  Crsf::Codec::Get_AltitudePacked( 0 ) );
    TEST_ASSERT_EQUAL_HEX16( 0u,      Crsf::Codec::Get_AltitudePacked( -10000 ) );
    TEST_ASSERT_EQUAL_HEX16( 0u,      Crsf::Codec::Get_AltitudePacked( -20000 ) );
    TEST_ASSERT_EQUAL_HEX16( 0x7FFFu, Crsf::Codec::Get_AltitudePacked( 22767 ) );
    TEST_ASSERT_EQUAL_HEX16( 0x88E5u, Crsf::Codec::Get_AltitudePacked( 22768 ) );
    TEST_ASSERT_EQUAL_HEX16( 0xFFFEu, Crsf::Codec::Get_AltitudePacked( 400000 ) );

    TEST_ASSERT_EQUAL_INT32( 0,      Crsf::Codec::Get_AltitudeDm( 10000u ) );
    TEST_ASSERT_EQUAL_INT32( -10000, Crsf::Codec::Get_AltitudeDm( 0u ) );
    TEST_ASSERT_EQUAL_INT32( 22767,  Crsf::Codec::Get_AltitudeDm( 0x7FFFu ) );
    TEST_ASSERT_EQUAL_INT32( 22770,  Crsf::Codec::Get_AltitudeDm( 0x88E5u ) );
    TEST_ASSERT_EQUAL_INT32( 327660, Crsf::Codec::Get_AltitudeDm( 0xFFFEu ) );
}


void Ut_Codec_VerticalSpeedPacking( void )
{
    TEST_ASSERT_EQUAL_INT8( 0,    Crsf::Codec::Get_VerticalSpeedPacked( 0 ) );
    TEST_ASSERT_EQUAL_INT8( 26,   Crsf::Codec::Get_VerticalSpeedPacked( 100 ) );
    TEST_ASSERT_EQUAL_INT8( -26,  Crsf::Codec::Get_VerticalSpeedPacked( -100 ) );
    TEST_ASSERT_EQUAL_INT8( 125,  Crsf::Codec::Get_VerticalSpeedPacked( 2500 ) );
    TEST_ASSERT_EQUAL_INT8( 127,  Crsf::Codec::Get_VerticalSpeedPacked( 30000 ) );
    TEST_ASSERT_EQUAL_INT8( -127, Crsf::Codec::Get_VerticalSpeedPacked( -30000 ) );

    TEST_ASSERT_EQUAL_INT16( 0,   Crsf::Codec::Get_VerticalSpeedCmS( 0 ) );
    TEST_ASSERT_EQUAL_INT16( 96,  Crsf::Codec::Get_VerticalSpeedCmS( 26 ) );
    TEST_ASSERT_EQUAL_INT16( -96, Crsf::Codec::Get_VerticalSpeedCmS( -26 ) );
}


/** Unpacked value is within precision of logarithmic scale (3 cm/s at low speed, 70 cm/s at 25 m/s) */
void Ut_Codec_VerticalSpeed_RoundTripPrecision( void )
{
    const int16_t speeds[] = { 3, 10, 50, -50, 300, -1000, 2500 };

    for( uint8_t i = 0u; i < sizeof( speeds ) / sizeof( speeds[ 0 ] ); i++ )
    {
        const int16_t unpacked  = Crsf::Codec::Get_VerticalSpeedCmS( Crsf::Codec::Get_VerticalSpeedPacked( speeds[ i ] ) );
        const int16_t tolerance = static_cast<int16_t>( 4 + ( ( speeds[ i ] < 0 ? -speeds[ i ] : speeds[ i ] ) * 3 / 100 ) );

        TEST_ASSERT_INT16_WITHIN( tolerance, speeds[ i ], unpacked );
    }
}
