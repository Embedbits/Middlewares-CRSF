/**
 * \author Mr.Nobody
 * \file Crsf_App_Codec.hpp
 * \ingroup Crsf
 * \brief CRSF payload encoding and decoding.
 *
 * - \ref Crsf::PayloadWriter / \ref Crsf::PayloadReader - bounds checked
 *   big endian access to payload buffer.
 * - \ref Crsf::Codec - encoding (structure -> payload) and decoding
 *   (payload -> structure) of broadcast frames and helper conversions.
 *
 * Encoders write into payload buffer of at least \ref CRSF_PAYLOAD_MAX_LEN
 * bytes and return payload length. Decoders accept payload longer than
 * expected (extra fields of newer protocol versions are ignored) and fail if
 * payload is shorter than expected.
 */

#ifndef APPLAYER_CRSF_APP_CODEC_HPP
#define APPLAYER_CRSF_APP_CODEC_HPP

/* ============================= INCLUDES =================================== */
#include "Crsf_Types.h"                     /* Module types definition        */
/* ============================= TYPEDEFS =================================== */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* ========================= EXPORTED MACROS ================================ */

/* ========================= EXPORTED VARIABLES ============================= */

/* ======================== EXPORTED FUNCTIONS ============================== */

namespace Crsf
{
    /**
     * \brief Big endian writer into payload buffer
     *
     * Writing behind the end of buffer is ignored and signaled by
     * \ref Is_Overflow.
     */
    class PayloadWriter
    {
    public:
        PayloadWriter( uint8_t * const buffer, crsf_MsgLength_t bufferSize );

        void Put_U8    ( uint8_t value );
        void Put_U16   ( uint16_t value );
        void Put_U24   ( uint32_t value );
        void Put_U32   ( uint32_t value );
        void Put_Bytes ( const uint8_t * const data, crsf_MsgLength_t length );
        void Put_String( const char * const text );

        crsf_MsgLength_t Get_Length( void ) const;
        bool             Is_Overflow( void ) const;

    private:
        uint8_t * const        buff;
        const crsf_MsgLength_t size;
        crsf_MsgLength_t       position = 0u;
        bool                   overflow = false;
    };


    /**
     * \brief Big endian reader of payload buffer
     *
     * Reading behind the end of payload returns 0 and it is signaled by
     * \ref Is_Underflow.
     */
    class PayloadReader
    {
    public:
        PayloadReader( const uint8_t * const buffer, crsf_MsgLength_t bufferLength );

        uint8_t  Get_U8 ( void );
        uint16_t Get_U16( void );
        uint32_t Get_U24( void );
        uint32_t Get_U32( void );
        void     Get_Bytes ( uint8_t * const data, crsf_MsgLength_t count );
        void     Get_String( char * const text, crsf_MsgLength_t textSize );

        crsf_MsgLength_t Get_Remaining( void ) const;
        const uint8_t   *Get_Position( void ) const;
        bool             Is_Underflow( void ) const;

    private:
        const uint8_t * const  buff;
        const crsf_MsgLength_t length;
        crsf_MsgLength_t       position  = 0u;
        bool                   underflow = false;
    };


namespace Codec
{
    /* ------------------------- Payload lengths ---------------------------- */

    static constexpr crsf_MsgLength_t gpsLen               = 15u; /**< 0x02 payload length */
    static constexpr crsf_MsgLength_t gpsTimeLen           = 9u;  /**< 0x03 payload length */
    static constexpr crsf_MsgLength_t gpsExtendedLen       = 20u; /**< 0x06 payload length */
    static constexpr crsf_MsgLength_t varioLen             = 2u;  /**< 0x07 payload length */
    static constexpr crsf_MsgLength_t batteryLen           = 8u;  /**< 0x08 payload length */
    static constexpr crsf_MsgLength_t baroAltitudeLen      = 3u;  /**< 0x09 payload length */
    static constexpr crsf_MsgLength_t heartbeatLen         = 2u;  /**< 0x0B payload length */
    static constexpr crsf_MsgLength_t vtxTelemetryLen      = 5u;  /**< 0x10 payload length */
    static constexpr crsf_MsgLength_t linkStatisticsLen    = 10u; /**< 0x14 payload length */
    static constexpr crsf_MsgLength_t rcChannelsLen        = 22u; /**< 0x16 payload length */
    static constexpr crsf_MsgLength_t linkStatisticsRxLen  = 5u;  /**< 0x1C payload length */
    static constexpr crsf_MsgLength_t linkStatisticsTxLen  = 6u;  /**< 0x1D payload length */
    static constexpr crsf_MsgLength_t attitudeLen          = 6u;  /**< 0x1E payload length */
    static constexpr crsf_MsgLength_t mavLinkFcLen         = 9u;  /**< 0x1F payload length */
    static constexpr crsf_MsgLength_t espNowLen            = 52u; /**< 0x22 payload length */
    static constexpr crsf_MsgLength_t timingCorrectionLen  = 9u;  /**< 0x3A.0x10 payload length (incl. sub-type) */
    static constexpr crsf_MsgLength_t mavLinkSysStatusLen  = 12u; /**< 0xAC payload length */

    /* ---------------------------- Encoders -------------------------------- */

    crsf_MsgLength_t Encode_Gps             ( const crsf_Gps_t &data,              uint8_t * const payload );
    crsf_MsgLength_t Encode_GpsTime         ( const crsf_GpsTime_t &data,          uint8_t * const payload );
    crsf_MsgLength_t Encode_GpsExtended     ( const crsf_GpsExtended_t &data,      uint8_t * const payload );
    crsf_MsgLength_t Encode_Vario           ( const crsf_Vario_t &data,            uint8_t * const payload );
    crsf_MsgLength_t Encode_Battery         ( const crsf_Battery_t &data,          uint8_t * const payload );
    crsf_MsgLength_t Encode_BaroAltitude    ( const crsf_BaroAltitude_t &data,     uint8_t * const payload );
    crsf_MsgLength_t Encode_Heartbeat       ( const crsf_Heartbeat_t &data,        uint8_t * const payload );
    crsf_MsgLength_t Encode_VtxTelemetry    ( const crsf_VtxTelemetry_t &data,     uint8_t * const payload );
    crsf_MsgLength_t Encode_LinkStatistics  ( const crsf_LinkStatistics_t &data,   uint8_t * const payload );
    crsf_MsgLength_t Encode_RcChannels      ( const crsf_RcChannels_t &data,       uint8_t * const payload );
    crsf_MsgLength_t Encode_LinkStatisticsRx( const crsf_LinkStatisticsRx_t &data, uint8_t * const payload );
    crsf_MsgLength_t Encode_LinkStatisticsTx( const crsf_LinkStatisticsTx_t &data, uint8_t * const payload );
    crsf_MsgLength_t Encode_Attitude        ( const crsf_Attitude_t &data,         uint8_t * const payload );
    crsf_MsgLength_t Encode_MavLinkFc       ( const crsf_MavLinkFc_t &data,        uint8_t * const payload );
    crsf_MsgLength_t Encode_FlightMode      ( const char * const name,             uint8_t * const payload );
    crsf_MsgLength_t Encode_EspNow          ( const crsf_EspNow_t &data,           uint8_t * const payload );
    crsf_MsgLength_t Encode_TimingCorrection( const crsf_TimingCorrection_t &data, uint8_t * const payload );
    crsf_MsgLength_t Encode_MavLinkSysStatus( const crsf_MavLinkSysStatus_t &data, uint8_t * const payload );

    /* ---------------------------- Decoders -------------------------------- */

    crsf_RequestState_t Decode_Gps             ( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_Gps_t &data );
    crsf_RequestState_t Decode_GpsTime         ( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_GpsTime_t &data );
    crsf_RequestState_t Decode_GpsExtended     ( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_GpsExtended_t &data );
    crsf_RequestState_t Decode_Vario           ( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_Vario_t &data );
    crsf_RequestState_t Decode_Battery         ( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_Battery_t &data );
    crsf_RequestState_t Decode_BaroAltitude    ( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_BaroAltitude_t &data );
    crsf_RequestState_t Decode_Heartbeat       ( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_Heartbeat_t &data );
    crsf_RequestState_t Decode_VtxTelemetry    ( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_VtxTelemetry_t &data );
    crsf_RequestState_t Decode_LinkStatistics  ( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_LinkStatistics_t &data );
    crsf_RequestState_t Decode_RcChannels      ( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_RcChannels_t &data );
    crsf_RequestState_t Decode_LinkStatisticsRx( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_LinkStatisticsRx_t &data );
    crsf_RequestState_t Decode_LinkStatisticsTx( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_LinkStatisticsTx_t &data );
    crsf_RequestState_t Decode_Attitude        ( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_Attitude_t &data );
    crsf_RequestState_t Decode_MavLinkFc       ( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_MavLinkFc_t &data );
    crsf_RequestState_t Decode_FlightMode      ( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_FlightMode_t &data );
    crsf_RequestState_t Decode_EspNow          ( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_EspNow_t &data );
    crsf_RequestState_t Decode_TimingCorrection( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_TimingCorrection_t &data );
    crsf_RequestState_t Decode_MavLinkSysStatus( const uint8_t * const payload, crsf_MsgLength_t payloadLen, crsf_MavLinkSysStatus_t &data );

    /* ---------------------------- Conversions ----------------------------- */

    int32_t  Get_ChannelUs        ( uint16_t channelValue );
    uint16_t Get_ChannelValue     ( int32_t pulseUs );

    uint16_t Get_AltitudePacked   ( int32_t altitudeDm );
    int32_t  Get_AltitudeDm       ( uint16_t altitudePacked );
    int8_t   Get_VerticalSpeedPacked( int16_t verticalSpeedCmS );
    int16_t  Get_VerticalSpeedCmS ( int8_t verticalSpeedPacked );

} /* namespace Codec */
} /* namespace Crsf */

#endif /* APPLAYER_CRSF_APP_CODEC_HPP */
