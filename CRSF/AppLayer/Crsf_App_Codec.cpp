/**
 * \author Mr.Nobody
 * \file Crsf_App_Codec.cpp
 * \ingroup Crsf
 * \brief CRSF payload encoding and decoding.
 *
 * All multi-byte values are big endian. RC channels are packed as 11 bit
 * values, LSB first (channel 1 starts at bit 0 of the first byte).
 */

/* ============================= INCLUDES =================================== */
#include "Crsf_App_Codec.hpp"               /* Self include                   */
#include "Crsf_Types.h"                     /* Module types definition        */
#include <math.h>                           /* logf, expf                     */
/* ============================= TYPEDEFS =================================== */

/* ======================= FORWARD DECLARATIONS ============================= */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/** Count of bits in one byte */
static constexpr uint8_t crsf_BitsInByte = 8u;

/** Mask of one byte */
static constexpr uint32_t crsf_ByteMask = 0xFFu;

/** Mask of 24 bit value */
static constexpr uint32_t crsf_U24Mask = 0x00FFFFFFu;

/** String terminator */
static constexpr char crsf_StringEnd = '\0';

/* VTX telemetry - bit fields of the last byte */
static constexpr uint8_t crsf_VtxPitModeMask         = 0x01u; /**< pit_mode:1        */
static constexpr uint8_t crsf_VtxPitModeControlShift = 1u;    /**< pitmode_control:2 */
static constexpr uint8_t crsf_VtxPitModeControlMask  = 0x03u;
static constexpr uint8_t crsf_VtxPitModeSwitchShift  = 3u;    /**< pitmode_switch:4  */
static constexpr uint8_t crsf_VtxPitModeSwitchMask   = 0x0Fu;

/* RC channel conversion (center 992 = 1500 us, 8 ticks = 5 us) */
static constexpr int32_t crsf_ChannelCenterUs     = 1500;
static constexpr int32_t crsf_ChannelUsPerStep    = 5;
static constexpr int32_t crsf_ChannelTicksPerStep = 8;

/* Barometric altitude packing */
static constexpr int32_t  crsf_AltMinDm           = 10000;                     /**< Minimal altitude (offset) in dm       */
static constexpr int32_t  crsf_AltThresholdDm     = 0x8000 - crsf_AltMinDm;    /**< Altitude of precision change in dm    */
static constexpr int32_t  crsf_AltMaxDm           = ( 0x7FFE * 10 ) - 5;       /**< Maximal altitude in dm                */
static constexpr uint16_t crsf_AltPackedMax       = 0xFFFEu;                   /**< Maximal packed value (0xFFFF invalid) */
static constexpr uint16_t crsf_AltMeterFlag       = 0x8000u;                   /**< Meter resolution flag                 */
static constexpr uint16_t crsf_AltValueMask       = 0x7FFFu;                   /**< Value mask of packed altitude         */
static constexpr int32_t  crsf_DmInMeter          = 10;                        /**< Decimeters in one meter               */
static constexpr int32_t  crsf_DmRounding         = 5;                         /**< Rounding of dm to m                   */

/* Vertical speed packing (logarithmic scale) */
static constexpr float   crsf_VSpeedKl            = 100.0f;                    /**< Linearity constant                    */
static constexpr float   crsf_VSpeedKr            = 0.026f;                    /**< Range constant                        */
static constexpr float   crsf_VSpeedPackedMax     = 127.0f;                    /**< Maximal packed value                  */
static constexpr int16_t crsf_VSpeedMaxCmS        = 0x7FFF;                    /**< Maximal unpacked value                */

/* ============================== MACROS ==================================== */

/* ========================= EXPORTED VARIABLES ============================= */

/* ========================== LOCAL VARIABLES =============================== */

/* ======================== EXPORTED FUNCTIONS ============================== */

/* ---------------------------- PayloadWriter ------------------------------- */

/**
 * \brief Creates writer
 *
 * \param buffer     [in]: Payload buffer
 * \param bufferSize [in]: Size of payload buffer in bytes
 */
Crsf::PayloadWriter::PayloadWriter( uint8_t * const buffer, crsf_MsgLength_t bufferSize ) :
    buff( buffer ),
    size( ( nullptr != buffer ) ? bufferSize : 0u )
{
}


/** \brief Writes one byte */
void Crsf::PayloadWriter::Put_U8( uint8_t value )
{
    if( size > position )
    {
        buff[ position ] = value;
        position++;
    }
    else
    {
        overflow = true;
    }
}


/** \brief Writes 16 bit value (big endian) */
void Crsf::PayloadWriter::Put_U16( uint16_t value )
{
    Put_U8( static_cast<uint8_t>( ( value >> crsf_BitsInByte ) & crsf_ByteMask ) );
    Put_U8( static_cast<uint8_t>(   value                      & crsf_ByteMask ) );
}


/** \brief Writes lower 24 bits of value (big endian) */
void Crsf::PayloadWriter::Put_U24( uint32_t value )
{
    Put_U8 ( static_cast<uint8_t>( ( value >> ( 2u * crsf_BitsInByte ) ) & crsf_ByteMask ) );
    Put_U16( static_cast<uint16_t>( value & 0xFFFFu ) );
}


/** \brief Writes 32 bit value (big endian) */
void Crsf::PayloadWriter::Put_U32( uint32_t value )
{
    Put_U16( static_cast<uint16_t>( value >> ( 2u * crsf_BitsInByte ) ) );
    Put_U16( static_cast<uint16_t>( value & 0xFFFFu ) );
}


/**
 * \brief Writes array of bytes
 *
 * \param data   [in]: Data (null pointer writes zeros)
 * \param length [in]: Count of bytes
 */
void Crsf::PayloadWriter::Put_Bytes( const uint8_t * const data, crsf_MsgLength_t length )
{
    for( crsf_MsgLength_t byteIndex = 0u; length > byteIndex; byteIndex++ )
    {
        Put_U8( ( nullptr != data ) ? data[ byteIndex ] : 0u );
    }
}


/**
 * \brief Writes null-terminated string (including terminator)
 *
 * \param text [in]: String (null pointer writes empty string)
 */
void Crsf::PayloadWriter::Put_String( const char * const text )
{
    if( nullptr != text )
    {
        for( uint16_t charIndex = 0u; ( crsf_StringEnd != text[ charIndex ] ) && ( false == overflow ); charIndex++ )
        {
            Put_U8( static_cast<uint8_t>( text[ charIndex ] ) );
        }
    }
    else
    {
        /* Empty string */
    }

    Put_U8( static_cast<uint8_t>( crsf_StringEnd ) );
}


/** \brief Returns count of written bytes */
crsf_MsgLength_t Crsf::PayloadWriter::Get_Length( void ) const
{
    return ( position );
}


/** \brief Returns true if some data did not fit into buffer */
bool Crsf::PayloadWriter::Is_Overflow( void ) const
{
    return ( overflow );
}

/* ---------------------------- PayloadReader ------------------------------- */

/**
 * \brief Creates reader
 *
 * \param buffer       [in]: Payload
 * \param bufferLength [in]: Payload length in bytes
 */
Crsf::PayloadReader::PayloadReader( const uint8_t * const buffer, crsf_MsgLength_t bufferLength ) :
    buff( buffer ),
    length( ( nullptr != buffer ) ? bufferLength : 0u )
{
}


/** \brief Reads one byte */
uint8_t Crsf::PayloadReader::Get_U8( void )
{
    uint8_t value = 0u;

    if( length > position )
    {
        value = buff[ position ];
        position++;
    }
    else
    {
        underflow = true;
    }

    return ( value );
}


/** \brief Reads 16 bit value (big endian) */
uint16_t Crsf::PayloadReader::Get_U16( void )
{
    const uint16_t upperByte = Get_U8();
    const uint16_t lowerByte = Get_U8();

    return ( static_cast<uint16_t>( ( upperByte << crsf_BitsInByte ) | lowerByte ) );
}


/** \brief Reads 24 bit value (big endian) */
uint32_t Crsf::PayloadReader::Get_U24( void )
{
    const uint32_t upperByte  = Get_U8();
    const uint32_t lowerWord  = Get_U16();

    return ( ( upperByte << ( 2u * crsf_BitsInByte ) ) | lowerWord );
}


/** \brief Reads 32 bit value (big endian) */
uint32_t Crsf::PayloadReader::Get_U32( void )
{
    const uint32_t upperWord = Get_U16();
    const uint32_t lowerWord = Get_U16();

    return ( ( upperWord << ( 2u * crsf_BitsInByte ) ) | lowerWord );
}


/**
 * \brief Reads array of bytes
 *
 * \param data  [out]: Buffer for data
 * \param count [in]: Count of bytes
 */
void Crsf::PayloadReader::Get_Bytes( uint8_t * const data, crsf_MsgLength_t count )
{
    for( crsf_MsgLength_t byteIndex = 0u; count > byteIndex; byteIndex++ )
    {
        const uint8_t value = Get_U8();

        if( nullptr != data )
        {
            data[ byteIndex ] = value;
        }
        else
        {
            /* Data are skipped */
        }
    }
}


/**
 * \brief Reads null-terminated string
 *
 * String is read up to its terminator or to the end of payload. Characters
 * which do not fit into text buffer are skipped. Text is always terminated.
 *
 * \param text     [out]: Buffer for string
 * \param textSize  [in]: Size of buffer (including terminator)
 */
void Crsf::PayloadReader::Get_String( char * const text, crsf_MsgLength_t textSize )
{
    crsf_MsgLength_t textIndex = 0u;
    bool             reading   = ( length > position );

    if( false == reading )
    {
        underflow = true;
    }
    else
    {
        /* String is present */
    }

    while( true == reading )
    {
        const char character = static_cast<char>( Get_U8() );

        if( ( crsf_StringEnd == character ) || ( length <= position ) )
        {
            reading = false;
        }
        else
        {
            /* Continue to terminator */
        }

        if( ( crsf_StringEnd != character ) && ( nullptr != text ) && ( ( textIndex + 1u ) < textSize ) )
        {
            text[ textIndex ] = character;
            textIndex++;
        }
        else
        {
            /* Terminator or no space */
        }
    }

    if( ( nullptr != text ) && ( 0u < textSize ) )
    {
        text[ textIndex ] = crsf_StringEnd;
    }
    else
    {
        /* No buffer */
    }
}


/** \brief Returns count of not read bytes */
crsf_MsgLength_t Crsf::PayloadReader::Get_Remaining( void ) const
{
    return ( length - position );
}


/** \brief Returns pointer to the first not read byte */
const uint8_t *Crsf::PayloadReader::Get_Position( void ) const
{
    return ( ( nullptr != buff ) ? &buff[ position ] : nullptr );
}


/** \brief Returns true if reading behind the end of payload was requested */
bool Crsf::PayloadReader::Is_Underflow( void ) const
{
    return ( underflow );
}

/* ------------------------------- Encoders --------------------------------- */

/** \brief Encodes 0x02 GPS payload, returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_Gps( const crsf_Gps_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    writer.Put_U32( static_cast<uint32_t>( data.Latitude ) );
    writer.Put_U32( static_cast<uint32_t>( data.Longitude ) );
    writer.Put_U16( data.GroundSpeed );
    writer.Put_U16( data.Heading );
    writer.Put_U16( data.Altitude );
    writer.Put_U8 ( data.Satellites );

    return ( writer.Get_Length() );
}


/** \brief Encodes 0x03 GPS time payload, returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_GpsTime( const crsf_GpsTime_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    writer.Put_U16( static_cast<uint16_t>( data.Year ) );
    writer.Put_U8 ( data.Month );
    writer.Put_U8 ( data.Day );
    writer.Put_U8 ( data.Hour );
    writer.Put_U8 ( data.Minute );
    writer.Put_U8 ( data.Second );
    writer.Put_U16( data.Millisecond );

    return ( writer.Get_Length() );
}


/** \brief Encodes 0x06 GPS extended payload, returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_GpsExtended( const crsf_GpsExtended_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    writer.Put_U8 ( data.FixType );
    writer.Put_U16( static_cast<uint16_t>( data.NSpeed ) );
    writer.Put_U16( static_cast<uint16_t>( data.ESpeed ) );
    writer.Put_U16( static_cast<uint16_t>( data.VSpeed ) );
    writer.Put_U16( static_cast<uint16_t>( data.HSpeedAcc ) );
    writer.Put_U16( static_cast<uint16_t>( data.TrackAcc ) );
    writer.Put_U16( static_cast<uint16_t>( data.AltEllipsoid ) );
    writer.Put_U16( static_cast<uint16_t>( data.HAcc ) );
    writer.Put_U16( static_cast<uint16_t>( data.VAcc ) );
    writer.Put_U8 ( data.Reserved );
    writer.Put_U8 ( data.HDop );
    writer.Put_U8 ( data.VDop );

    return ( writer.Get_Length() );
}


/** \brief Encodes 0x07 Vario payload, returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_Vario( const crsf_Vario_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    writer.Put_U16( static_cast<uint16_t>( data.VSpeed ) );

    return ( writer.Get_Length() );
}


/** \brief Encodes 0x08 Battery payload (capacity is limited to 24 bits), returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_Battery( const crsf_Battery_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    writer.Put_U16( static_cast<uint16_t>( data.Voltage ) );
    writer.Put_U16( static_cast<uint16_t>( data.Current ) );
    writer.Put_U24( ( crsf_U24Mask < data.CapacityUsed ) ? crsf_U24Mask : data.CapacityUsed );
    writer.Put_U8 ( data.Remaining );

    return ( writer.Get_Length() );
}


/** \brief Encodes 0x09 Barometric altitude payload, returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_BaroAltitude( const crsf_BaroAltitude_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    writer.Put_U16( data.AltitudePacked );
    writer.Put_U8 ( static_cast<uint8_t>( data.VerticalSpeedPacked ) );

    return ( writer.Get_Length() );
}


/** \brief Encodes 0x0B Heartbeat payload, returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_Heartbeat( const crsf_Heartbeat_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    writer.Put_U16( static_cast<uint16_t>( data.OriginAddress ) );

    return ( writer.Get_Length() );
}


/** \brief Encodes 0x10 VTX telemetry payload, returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_VtxTelemetry( const crsf_VtxTelemetry_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    const uint8_t pitModeByte = static_cast<uint8_t>(
          (   data.PitMode        & crsf_VtxPitModeMask )
        | ( ( data.PitModeControl & crsf_VtxPitModeControlMask ) << crsf_VtxPitModeControlShift )
        | ( ( data.PitModeSwitch  & crsf_VtxPitModeSwitchMask  ) << crsf_VtxPitModeSwitchShift  ) );

    writer.Put_U8 ( data.OriginAddress );
    writer.Put_U8 ( data.PowerDbm );
    writer.Put_U16( data.FrequencyMhz );
    writer.Put_U8 ( pitModeByte );

    return ( writer.Get_Length() );
}


/** \brief Encodes 0x14 Link statistics payload, returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_LinkStatistics( const crsf_LinkStatistics_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    writer.Put_U8( data.UpRssiAnt1 );
    writer.Put_U8( data.UpRssiAnt2 );
    writer.Put_U8( data.UpLinkQuality );
    writer.Put_U8( static_cast<uint8_t>( data.UpSnr ) );
    writer.Put_U8( data.ActiveAntenna );
    writer.Put_U8( data.RfProfile );
    writer.Put_U8( data.UpRfPower );
    writer.Put_U8( data.DownRssi );
    writer.Put_U8( data.DownLinkQuality );
    writer.Put_U8( static_cast<uint8_t>( data.DownSnr ) );

    return ( writer.Get_Length() );
}


/** \brief Encodes 0x16 RC channels packed payload (values limited to 11 bits), returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_RcChannels( const crsf_RcChannels_t &data, uint8_t * const payload )
{
    crsf_MsgLength_t payloadLen = 0u;

    if( nullptr != payload )
    {
        uint32_t bitBuffer = 0u;
        uint8_t  bitsCount = 0u;

        for( uint8_t channelIndex = 0u; CRSF_RC_CHANNELS_CNT > channelIndex; channelIndex++ )
        {
            const uint16_t channelValue = ( CRSF_RC_CHANNEL_VALUE_MAX < data.Channels[ channelIndex ] ) ?
                                          CRSF_RC_CHANNEL_VALUE_MAX : data.Channels[ channelIndex ];

            bitBuffer |= static_cast<uint32_t>( channelValue ) << bitsCount;
            bitsCount += CRSF_RC_CHANNEL_BITS;

            while( crsf_BitsInByte <= bitsCount )
            {
                payload[ payloadLen ] = static_cast<uint8_t>( bitBuffer & crsf_ByteMask );
                payloadLen++;

                bitBuffer >>= crsf_BitsInByte;
                bitsCount  -= crsf_BitsInByte;
            }
        }
    }
    else
    {
        /* Null pointer has been assigned */
    }

    return ( payloadLen );
}


/** \brief Encodes 0x1C Link statistics RX payload, returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_LinkStatisticsRx( const crsf_LinkStatisticsRx_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    writer.Put_U8( data.RssiDb );
    writer.Put_U8( data.RssiPercent );
    writer.Put_U8( data.LinkQuality );
    writer.Put_U8( static_cast<uint8_t>( data.Snr ) );
    writer.Put_U8( data.RfPowerDb );

    return ( writer.Get_Length() );
}


/** \brief Encodes 0x1D Link statistics TX payload, returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_LinkStatisticsTx( const crsf_LinkStatisticsTx_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    writer.Put_U8( data.RssiDb );
    writer.Put_U8( data.RssiPercent );
    writer.Put_U8( data.LinkQuality );
    writer.Put_U8( static_cast<uint8_t>( data.Snr ) );
    writer.Put_U8( data.RfPowerDb );
    writer.Put_U8( data.Fps );

    return ( writer.Get_Length() );
}


/** \brief Encodes 0x1E Attitude payload, returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_Attitude( const crsf_Attitude_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    writer.Put_U16( static_cast<uint16_t>( data.Pitch ) );
    writer.Put_U16( static_cast<uint16_t>( data.Roll ) );
    writer.Put_U16( static_cast<uint16_t>( data.Yaw ) );

    return ( writer.Get_Length() );
}


/** \brief Encodes 0x1F MAVLink FC payload, returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_MavLinkFc( const crsf_MavLinkFc_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    writer.Put_U16( static_cast<uint16_t>( data.Airspeed ) );
    writer.Put_U8 ( data.BaseMode );
    writer.Put_U32( data.CustomMode );
    writer.Put_U8 ( data.AutopilotType );
    writer.Put_U8 ( data.FirmwareType );

    return ( writer.Get_Length() );
}


/**
 * \brief Encodes 0x21 Flight mode payload
 *
 * \param name    [in]: Null-terminated flight mode name, truncated to
 *                      CRSF_STRING_MAX_LEN - 1 characters
 * \param payload [out]: Payload buffer
 *
 * \return Payload length (including terminator)
 */
crsf_MsgLength_t Crsf::Codec::Encode_FlightMode( const char * const name, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_STRING_MAX_LEN - 1u );

    writer.Put_String( name );

    crsf_MsgLength_t payloadLen = writer.Get_Length();

    if( ( true == writer.Is_Overflow() ) && ( nullptr != payload ) )
    {
        /* Terminate truncated string */
        payload[ payloadLen ] = static_cast<uint8_t>( crsf_StringEnd );
        payloadLen++;
    }
    else
    {
        /* String fits */
    }

    return ( payloadLen );
}


/** \brief Encodes 0x22 ESP_NOW payload, returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_EspNow( const crsf_EspNow_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    writer.Put_U8   ( data.SeatPosition );
    writer.Put_U8   ( data.CurrentLap );
    writer.Put_Bytes( reinterpret_cast<const uint8_t *>( data.LapTime1 ), CRSF_ESP_NOW_LAP_TIME_LEN );
    writer.Put_Bytes( reinterpret_cast<const uint8_t *>( data.LapTime2 ), CRSF_ESP_NOW_LAP_TIME_LEN );
    writer.Put_Bytes( reinterpret_cast<const uint8_t *>( data.FreeText ), CRSF_ESP_NOW_FREE_TEXT_LEN );

    return ( writer.Get_Length() );
}


/**
 * \brief Encodes 0x3A.0x10 Timing correction payload (extended frame,
 *        payload starts by sub-type)
 *
 * \return Payload length
 */
crsf_MsgLength_t Crsf::Codec::Encode_TimingCorrection( const crsf_TimingCorrection_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_EXT_PAYLOAD_MAX_LEN );

    writer.Put_U8 ( CRSF_REMOTE_SUBTYPE_TIMING_CORRECTION );
    writer.Put_U32( data.UpdateInterval );
    writer.Put_U32( static_cast<uint32_t>( data.Offset ) );

    return ( writer.Get_Length() );
}


/** \brief Encodes 0xAC MAVLink system status payload, returns payload length */
crsf_MsgLength_t Crsf::Codec::Encode_MavLinkSysStatus( const crsf_MavLinkSysStatus_t &data, uint8_t * const payload )
{
    PayloadWriter writer( payload, CRSF_PAYLOAD_MAX_LEN );

    writer.Put_U32( data.SensorPresent );
    writer.Put_U32( data.SensorEnabled );
    writer.Put_U32( data.SensorHealth );

    return ( writer.Get_Length() );
}

/* ------------------------------- Decoders --------------------------------- */

/** \brief Decodes 0x02 GPS payload */
crsf_RequestState_t Crsf::Codec::Decode_Gps( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_Gps_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( gpsLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.Latitude    = static_cast<int32_t>( reader.Get_U32() );
        data.Longitude   = static_cast<int32_t>( reader.Get_U32() );
        data.GroundSpeed = reader.Get_U16();
        data.Heading     = reader.Get_U16();
        data.Altitude    = reader.Get_U16();
        data.Satellites  = reader.Get_U8();

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x03 GPS time payload */
crsf_RequestState_t Crsf::Codec::Decode_GpsTime( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_GpsTime_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( gpsTimeLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.Year        = static_cast<int16_t>( reader.Get_U16() );
        data.Month       = reader.Get_U8();
        data.Day         = reader.Get_U8();
        data.Hour        = reader.Get_U8();
        data.Minute      = reader.Get_U8();
        data.Second      = reader.Get_U8();
        data.Millisecond = reader.Get_U16();

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x06 GPS extended payload */
crsf_RequestState_t Crsf::Codec::Decode_GpsExtended( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_GpsExtended_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( gpsExtendedLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.FixType      = reader.Get_U8();
        data.NSpeed       = static_cast<int16_t>( reader.Get_U16() );
        data.ESpeed       = static_cast<int16_t>( reader.Get_U16() );
        data.VSpeed       = static_cast<int16_t>( reader.Get_U16() );
        data.HSpeedAcc    = static_cast<int16_t>( reader.Get_U16() );
        data.TrackAcc     = static_cast<int16_t>( reader.Get_U16() );
        data.AltEllipsoid = static_cast<int16_t>( reader.Get_U16() );
        data.HAcc         = static_cast<int16_t>( reader.Get_U16() );
        data.VAcc         = static_cast<int16_t>( reader.Get_U16() );
        data.Reserved     = reader.Get_U8();
        data.HDop         = reader.Get_U8();
        data.VDop         = reader.Get_U8();

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x07 Vario payload */
crsf_RequestState_t Crsf::Codec::Decode_Vario( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_Vario_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( varioLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.VSpeed = static_cast<int16_t>( reader.Get_U16() );

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x08 Battery payload */
crsf_RequestState_t Crsf::Codec::Decode_Battery( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_Battery_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( batteryLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.Voltage      = static_cast<int16_t>( reader.Get_U16() );
        data.Current      = static_cast<int16_t>( reader.Get_U16() );
        data.CapacityUsed = reader.Get_U24();
        data.Remaining    = reader.Get_U8();

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x09 Barometric altitude payload */
crsf_RequestState_t Crsf::Codec::Decode_BaroAltitude( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_BaroAltitude_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( baroAltitudeLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.AltitudePacked      = reader.Get_U16();
        data.VerticalSpeedPacked = static_cast<int8_t>( reader.Get_U8() );

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x0B Heartbeat payload */
crsf_RequestState_t Crsf::Codec::Decode_Heartbeat( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_Heartbeat_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( heartbeatLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.OriginAddress = static_cast<int16_t>( reader.Get_U16() );

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x10 VTX telemetry payload */
crsf_RequestState_t Crsf::Codec::Decode_VtxTelemetry( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_VtxTelemetry_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( vtxTelemetryLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.OriginAddress = reader.Get_U8();
        data.PowerDbm      = reader.Get_U8();
        data.FrequencyMhz  = reader.Get_U16();

        const uint8_t pitModeByte = reader.Get_U8();

        data.PitMode        =   pitModeByte                                  & crsf_VtxPitModeMask;
        data.PitModeControl = ( pitModeByte >> crsf_VtxPitModeControlShift ) & crsf_VtxPitModeControlMask;
        data.PitModeSwitch  = ( pitModeByte >> crsf_VtxPitModeSwitchShift  ) & crsf_VtxPitModeSwitchMask;

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x14 Link statistics payload */
crsf_RequestState_t Crsf::Codec::Decode_LinkStatistics( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_LinkStatistics_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( linkStatisticsLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.UpRssiAnt1      = reader.Get_U8();
        data.UpRssiAnt2      = reader.Get_U8();
        data.UpLinkQuality   = reader.Get_U8();
        data.UpSnr           = static_cast<int8_t>( reader.Get_U8() );
        data.ActiveAntenna   = reader.Get_U8();
        data.RfProfile       = reader.Get_U8();
        data.UpRfPower       = reader.Get_U8();
        data.DownRssi        = reader.Get_U8();
        data.DownLinkQuality = reader.Get_U8();
        data.DownSnr         = static_cast<int8_t>( reader.Get_U8() );

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x16 RC channels packed payload */
crsf_RequestState_t Crsf::Codec::Decode_RcChannels( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_RcChannels_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( rcChannelsLen <= payloadLen ) )
    {
        uint32_t         bitBuffer = 0u;
        uint8_t          bitsCount = 0u;
        crsf_MsgLength_t byteIndex = 0u;

        for( uint8_t channelIndex = 0u; CRSF_RC_CHANNELS_CNT > channelIndex; channelIndex++ )
        {
            while( CRSF_RC_CHANNEL_BITS > bitsCount )
            {
                bitBuffer |= static_cast<uint32_t>( payload[ byteIndex ] ) << bitsCount;
                bitsCount += crsf_BitsInByte;
                byteIndex++;
            }

            data.Channels[ channelIndex ] = static_cast<uint16_t>( bitBuffer & CRSF_RC_CHANNEL_VALUE_MAX );

            bitBuffer >>= CRSF_RC_CHANNEL_BITS;
            bitsCount  -= CRSF_RC_CHANNEL_BITS;
        }

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x1C Link statistics RX payload */
crsf_RequestState_t Crsf::Codec::Decode_LinkStatisticsRx( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_LinkStatisticsRx_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( linkStatisticsRxLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.RssiDb      = reader.Get_U8();
        data.RssiPercent = reader.Get_U8();
        data.LinkQuality = reader.Get_U8();
        data.Snr         = static_cast<int8_t>( reader.Get_U8() );
        data.RfPowerDb   = reader.Get_U8();

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x1D Link statistics TX payload */
crsf_RequestState_t Crsf::Codec::Decode_LinkStatisticsTx( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_LinkStatisticsTx_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( linkStatisticsTxLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.RssiDb      = reader.Get_U8();
        data.RssiPercent = reader.Get_U8();
        data.LinkQuality = reader.Get_U8();
        data.Snr         = static_cast<int8_t>( reader.Get_U8() );
        data.RfPowerDb   = reader.Get_U8();
        data.Fps         = reader.Get_U8();

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x1E Attitude payload */
crsf_RequestState_t Crsf::Codec::Decode_Attitude( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_Attitude_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( attitudeLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.Pitch = static_cast<int16_t>( reader.Get_U16() );
        data.Roll  = static_cast<int16_t>( reader.Get_U16() );
        data.Yaw   = static_cast<int16_t>( reader.Get_U16() );

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x1F MAVLink FC payload */
crsf_RequestState_t Crsf::Codec::Decode_MavLinkFc( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_MavLinkFc_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( mavLinkFcLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.Airspeed      = static_cast<int16_t>( reader.Get_U16() );
        data.BaseMode      = reader.Get_U8();
        data.CustomMode    = reader.Get_U32();
        data.AutopilotType = reader.Get_U8();
        data.FirmwareType  = reader.Get_U8();

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/**
 * \brief Decodes 0x21 Flight mode payload
 *
 * Missing terminator is tolerated (string ends with payload). Empty payload
 * is invalid.
 */
crsf_RequestState_t Crsf::Codec::Decode_FlightMode( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_FlightMode_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( 0u < payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        reader.Get_String( data.Name, CRSF_STRING_MAX_LEN );

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x22 ESP_NOW payload */
crsf_RequestState_t Crsf::Codec::Decode_EspNow( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_EspNow_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( espNowLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.SeatPosition = reader.Get_U8();
        data.CurrentLap   = reader.Get_U8();
        reader.Get_Bytes( reinterpret_cast<uint8_t *>( data.LapTime1 ), CRSF_ESP_NOW_LAP_TIME_LEN );
        reader.Get_Bytes( reinterpret_cast<uint8_t *>( data.LapTime2 ), CRSF_ESP_NOW_LAP_TIME_LEN );
        reader.Get_Bytes( reinterpret_cast<uint8_t *>( data.FreeText ), CRSF_ESP_NOW_FREE_TEXT_LEN );

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}


/** \brief Decodes 0x3A Remote related payload of sub-type 0x10 Timing correction */
crsf_RequestState_t Crsf::Codec::Decode_TimingCorrection( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_TimingCorrection_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr                               != payload      ) &&
        ( timingCorrectionLen                   <= payloadLen   ) &&
        ( CRSF_REMOTE_SUBTYPE_TIMING_CORRECTION == payload[ 0u ] )   )
    {
        PayloadReader reader( payload, payloadLen );

        (void)reader.Get_U8();
        data.UpdateInterval = reader.Get_U32();
        data.Offset         = static_cast<int32_t>( reader.Get_U32() );

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short or other sub-type */
    }

    return ( retState );
}


/** \brief Decodes 0xAC MAVLink system status payload */
crsf_RequestState_t Crsf::Codec::Decode_MavLinkSysStatus( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_MavLinkSysStatus_t &data )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( nullptr != payload ) && ( mavLinkSysStatusLen <= payloadLen ) )
    {
        PayloadReader reader( payload, payloadLen );

        data.SensorPresent = reader.Get_U32();
        data.SensorEnabled = reader.Get_U32();
        data.SensorHealth  = reader.Get_U32();

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Payload too short */
    }

    return ( retState );
}

/* ------------------------------ Conversions ------------------------------- */

/**
 * \brief Converts raw RC channel value to pulse width
 *
 * \param channelValue [in]: Raw channel value (992 = center)
 *
 * \return Pulse width in us ((x - 992) * 5 / 8 + 1500)
 */
int32_t Crsf::Codec::Get_ChannelUs( uint16_t channelValue )
{
    return ( ( ( static_cast<int32_t>( channelValue ) - static_cast<int32_t>( CRSF_RC_CHANNEL_VALUE_CENTER ) )
               * crsf_ChannelUsPerStep / crsf_ChannelTicksPerStep ) + crsf_ChannelCenterUs );
}


/**
 * \brief Converts pulse width to raw RC channel value
 *
 * \param pulseUs [in]: Pulse width in us
 *
 * \return Raw channel value ((x - 1500) * 8 / 5 + 992), limited to 0 - 2047
 */
uint16_t Crsf::Codec::Get_ChannelValue( int32_t pulseUs )
{
    int32_t channelValue = ( ( pulseUs - crsf_ChannelCenterUs ) * crsf_ChannelTicksPerStep / crsf_ChannelUsPerStep )
                           + static_cast<int32_t>( CRSF_RC_CHANNEL_VALUE_CENTER );

    if( 0 > channelValue )
    {
        channelValue = 0;
    }
    else if( static_cast<int32_t>( CRSF_RC_CHANNEL_VALUE_MAX ) < channelValue )
    {
        channelValue = static_cast<int32_t>( CRSF_RC_CHANNEL_VALUE_MAX );
    }
    else
    {
        /* Value in range */
    }

    return ( static_cast<uint16_t>( channelValue ) );
}


/**
 * \brief Packs altitude for 0x09 Barometric altitude frame
 *
 * Altitude below 2276.7 m is sent in dm with offset 10000 dm, higher altitude
 * in meters with MSB set. Range is -1000 m ... 32766 m.
 *
 * \param altitudeDm [in]: Altitude above start point in dm
 *
 * \return Packed altitude
 */
uint16_t Crsf::Codec::Get_AltitudePacked( int32_t altitudeDm )
{
    uint16_t altitudePacked = 0u;

    if( -crsf_AltMinDm > altitudeDm )
    {
        altitudePacked = 0u;
    }
    else if( crsf_AltMaxDm < altitudeDm )
    {
        altitudePacked = crsf_AltPackedMax;
    }
    else if( crsf_AltThresholdDm > altitudeDm )
    {
        altitudePacked = static_cast<uint16_t>( altitudeDm + crsf_AltMinDm );
    }
    else
    {
        altitudePacked = static_cast<uint16_t>( ( ( altitudeDm + crsf_DmRounding ) / crsf_DmInMeter ) | crsf_AltMeterFlag );
    }

    return ( altitudePacked );
}


/**
 * \brief Unpacks altitude of 0x09 Barometric altitude frame
 *
 * \param altitudePacked [in]: Packed altitude
 *
 * \return Altitude above start point in dm
 */
int32_t Crsf::Codec::Get_AltitudeDm( uint16_t altitudePacked )
{
    int32_t altitudeDm = 0;

    if( 0u != ( altitudePacked & crsf_AltMeterFlag ) )
    {
        altitudeDm = static_cast<int32_t>( altitudePacked & crsf_AltValueMask ) * crsf_DmInMeter;
    }
    else
    {
        altitudeDm = static_cast<int32_t>( altitudePacked ) - crsf_AltMinDm;
    }

    return ( altitudeDm );
}


/**
 * \brief Packs vertical speed for 0x09 Barometric altitude frame
 *
 * Logarithmic scale: packed = ln( |v| / 100 + 1 ) / 0.026 (sign of v). Range is
 * approximately +-2500 cm/s, precision 3 cm/s at low speed.
 *
 * \param verticalSpeedCmS [in]: Vertical speed in cm/s
 *
 * \return Packed vertical speed
 */
int8_t Crsf::Codec::Get_VerticalSpeedPacked( int16_t verticalSpeedCmS )
{
    const float absSpeed = ( 0 > verticalSpeedCmS ) ? -static_cast<float>( verticalSpeedCmS )
                                                    :  static_cast<float>( verticalSpeedCmS );
    float       packed   = logf( ( absSpeed / crsf_VSpeedKl ) + 1.0f ) / crsf_VSpeedKr;

    if( crsf_VSpeedPackedMax < packed )
    {
        packed = crsf_VSpeedPackedMax;
    }
    else
    {
        /* Value in range */
    }

    const int8_t packedAbs = static_cast<int8_t>( packed );

    return ( ( 0 > verticalSpeedCmS ) ? static_cast<int8_t>( -packedAbs ) : packedAbs );
}


/**
 * \brief Unpacks vertical speed of 0x09 Barometric altitude frame
 *
 * v = ( e^( |packed| * 0.026 ) - 1 ) * 100 (sign of packed). Note: the formula
 * in the specification misplaces the parenthesis, this is the exact inverse
 * function of packing.
 *
 * \param verticalSpeedPacked [in]: Packed vertical speed
 *
 * \return Vertical speed in cm/s
 */
int16_t Crsf::Codec::Get_VerticalSpeedCmS( int8_t verticalSpeedPacked )
{
    const float absPacked = ( 0 > verticalSpeedPacked ) ? -static_cast<float>( verticalSpeedPacked )
                                                        :  static_cast<float>( verticalSpeedPacked );
    float       speed     = ( expf( absPacked * crsf_VSpeedKr ) - 1.0f ) * crsf_VSpeedKl;

    if( static_cast<float>( crsf_VSpeedMaxCmS ) < speed )
    {
        speed = static_cast<float>( crsf_VSpeedMaxCmS );
    }
    else
    {
        /* Value in range */
    }

    const int16_t speedAbs = static_cast<int16_t>( speed );

    return ( ( 0 > verticalSpeedPacked ) ? static_cast<int16_t>( -speedAbs ) : speedAbs );
}

/* ========================== LOCAL FUNCTIONS =============================== */

/* =============================== TASKS ==================================== */
