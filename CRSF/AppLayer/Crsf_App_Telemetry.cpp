/**
 * \author Mr.Nobody
 * \file Crsf_App_Telemetry.cpp
 * \ingroup Crsf
 * \brief CRSF telemetry - reception (callbacks) and transmission of frames.
 */

/* ============================= INCLUDES =================================== */
#include "Crsf_App_Telemetry.hpp"           /* Self include                   */
#include "Crsf_App_Codec.hpp"               /* Payload encoding / decoding    */
#include "Crsf_Crc.hpp"                     /* Command CRC                    */
/* ============================= TYPEDEFS =================================== */

/* ======================= FORWARD DECLARATIONS ============================= */

/**
 * \brief Decodes payload and calls callback
 *
 * \param decoder           [in]: Payload decoder
 * \param callback          [in]: Application callback
 * \param frame             [in]: Received frame
 * \param invalidPayloadCnt [in/out]: Counter of invalid payloads
 */
template< typename DataType >
static void crsf_DecodeAndNotify( crsf_RequestState_t ( *decoder )( const uint8_t * const, crsf_MsgLength_t, DataType & ),
                                  void ( *callback )( const DataType * ),
                                  const crsf_Frame_t &frame,
                                  crsf_Counter_t &invalidPayloadCnt );

/* ========================= SYMBOLIC CONSTANTS ============================= */

/** Length of command header (command ID and sub-command ID) */
static constexpr crsf_MsgLength_t crsf_CommandHeaderLen = 2u;

/** Length of command CRC */
static constexpr crsf_MsgLength_t crsf_CommandCrcLen = 1u;

/* ============================== MACROS ==================================== */

/* ========================= EXPORTED VARIABLES ============================= */

/* ========================== LOCAL VARIABLES =============================== */

/* ======================== EXPORTED FUNCTIONS ============================== */

/* ------------------------------ TelemetryRx ------------------------------- */

/**
 * \brief Constructor of telemetry receiver
 *
 * \param callbacks [in]: Application callbacks (object shall exist during
 *                        the whole life of receiver)
 */
Crsf::AppLayer::TelemetryRx::TelemetryRx( const crsf_TelemetryCallbacks_t &callbacks ) :
    telemetryCallbacks( callbacks )
{
}


/** \brief Frame type is supported if application callback is assigned */
crsf_FrameTypeStatus_t Crsf::AppLayer::TelemetryRx::Get_FrameTypeStatus( crsf_FrameType_t frameType ) const
{
    bool supported = false;

    switch( frameType )
    {
        case CRSF_FRAMETYPE_GPS:                supported = ( nullptr != telemetryCallbacks.Gps );              break;
        case CRSF_FRAMETYPE_GPS_TIME:           supported = ( nullptr != telemetryCallbacks.GpsTime );          break;
        case CRSF_FRAMETYPE_GPS_EXTENDED:       supported = ( nullptr != telemetryCallbacks.GpsExtended );      break;
        case CRSF_FRAMETYPE_VARIO:              supported = ( nullptr != telemetryCallbacks.Vario );            break;
        case CRSF_FRAMETYPE_BATTERY:            supported = ( nullptr != telemetryCallbacks.Battery );          break;
        case CRSF_FRAMETYPE_BARO_ALTITUDE:      supported = ( nullptr != telemetryCallbacks.BaroAltitude );     break;
        case CRSF_FRAMETYPE_HEARTBEAT:          supported = ( nullptr != telemetryCallbacks.Heartbeat );        break;
        case CRSF_FRAMETYPE_VTX_TELEMETRY:      supported = ( nullptr != telemetryCallbacks.VtxTelemetry );     break;
        case CRSF_FRAMETYPE_LINK_STATISTICS:    supported = ( nullptr != telemetryCallbacks.LinkStatistics );   break;
        case CRSF_FRAMETYPE_LINK_STATISTICS_RX: supported = ( nullptr != telemetryCallbacks.LinkStatisticsRx ); break;
        case CRSF_FRAMETYPE_LINK_STATISTICS_TX: supported = ( nullptr != telemetryCallbacks.LinkStatisticsTx ); break;
        case CRSF_FRAMETYPE_ATTITUDE:           supported = ( nullptr != telemetryCallbacks.Attitude );         break;
        case CRSF_FRAMETYPE_MAVLINK_FC:         supported = ( nullptr != telemetryCallbacks.MavLinkFc );        break;
        case CRSF_FRAMETYPE_FLIGHT_MODE:        supported = ( nullptr != telemetryCallbacks.FlightMode );       break;
        case CRSF_FRAMETYPE_ESP_NOW:            supported = ( nullptr != telemetryCallbacks.EspNow );           break;
        case CRSF_FRAMETYPE_REMOTE:             supported = ( nullptr != telemetryCallbacks.TimingCorrection ); break;
        case CRSF_FRAMETYPE_MAVLINK_SYS_STATUS: supported = ( nullptr != telemetryCallbacks.MavLinkSysStatus ); break;
        default:                                supported = false;                                              break;
    }

    return ( supported ? CRSF_FRAMETYPE_SUPPORTED : CRSF_FRAMETYPE_NOT_SUPPORTED );
}


/**
 * \brief Decodes received frame and calls application callback
 *
 * \param frame  [in]: Received frame
 * \param sender [in]: Not used
 */
void Crsf::AppLayer::TelemetryRx::Process_Frame( const crsf_Frame_t &frame, FrameSender &sender )
{
    (void)sender;

    const crsf_TelemetryCallbacks_t &cb = telemetryCallbacks;

    switch( frame.Type )
    {
        case CRSF_FRAMETYPE_GPS:                crsf_DecodeAndNotify( Codec::Decode_Gps,              cb.Gps,              frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_GPS_TIME:           crsf_DecodeAndNotify( Codec::Decode_GpsTime,          cb.GpsTime,          frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_GPS_EXTENDED:       crsf_DecodeAndNotify( Codec::Decode_GpsExtended,      cb.GpsExtended,      frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_VARIO:              crsf_DecodeAndNotify( Codec::Decode_Vario,            cb.Vario,            frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_BATTERY:            crsf_DecodeAndNotify( Codec::Decode_Battery,          cb.Battery,          frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_BARO_ALTITUDE:      crsf_DecodeAndNotify( Codec::Decode_BaroAltitude,     cb.BaroAltitude,     frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_HEARTBEAT:          crsf_DecodeAndNotify( Codec::Decode_Heartbeat,        cb.Heartbeat,        frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_VTX_TELEMETRY:      crsf_DecodeAndNotify( Codec::Decode_VtxTelemetry,     cb.VtxTelemetry,     frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_LINK_STATISTICS:    crsf_DecodeAndNotify( Codec::Decode_LinkStatistics,   cb.LinkStatistics,   frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_LINK_STATISTICS_RX: crsf_DecodeAndNotify( Codec::Decode_LinkStatisticsRx, cb.LinkStatisticsRx, frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_LINK_STATISTICS_TX: crsf_DecodeAndNotify( Codec::Decode_LinkStatisticsTx, cb.LinkStatisticsTx, frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_ATTITUDE:           crsf_DecodeAndNotify( Codec::Decode_Attitude,         cb.Attitude,         frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_MAVLINK_FC:         crsf_DecodeAndNotify( Codec::Decode_MavLinkFc,        cb.MavLinkFc,        frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_FLIGHT_MODE:        crsf_DecodeAndNotify( Codec::Decode_FlightMode,       cb.FlightMode,       frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_ESP_NOW:            crsf_DecodeAndNotify( Codec::Decode_EspNow,           cb.EspNow,           frame, invalidPayloadCnt ); break;
        case CRSF_FRAMETYPE_MAVLINK_SYS_STATUS: crsf_DecodeAndNotify( Codec::Decode_MavLinkSysStatus, cb.MavLinkSysStatus, frame, invalidPayloadCnt ); break;

        case CRSF_FRAMETYPE_REMOTE:
            /* Other sub-types of remote related frames are not processed */
            if( ( 0u                                    <  frame.PayloadLen   ) &&
                ( CRSF_REMOTE_SUBTYPE_TIMING_CORRECTION == frame.Payload[ 0u ] )    )
            {
                crsf_DecodeAndNotify( Codec::Decode_TimingCorrection, cb.TimingCorrection, frame, invalidPayloadCnt );
            }
            else
            {
                /* Other sub-type */
            }
            break;

        default:
            /* Frame type is not processed */
            break;
    }
}


/** \brief Returns count of frames with invalid (too short) payload */
crsf_Counter_t Crsf::AppLayer::TelemetryRx::Get_InvalidPayloadCnt( void ) const
{
    return ( invalidPayloadCnt );
}

/* ------------------------------ TelemetryTx ------------------------------- */

/**
 * \brief Constructor of telemetry transmitter
 *
 * \param frameSender [in]: Frame sender (application core)
 */
Crsf::AppLayer::TelemetryTx::TelemetryTx( FrameSender &frameSender ) :
    sender( frameSender )
{
}


/** \brief Sends 0x02 GPS */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_Gps( const crsf_Gps_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_GPS, payload, Codec::Encode_Gps( data, payload ) ) );
}


/** \brief Sends 0x03 GPS time */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_GpsTime( const crsf_GpsTime_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_GPS_TIME, payload, Codec::Encode_GpsTime( data, payload ) ) );
}


/** \brief Sends 0x06 GPS extended */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_GpsExtended( const crsf_GpsExtended_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_GPS_EXTENDED, payload, Codec::Encode_GpsExtended( data, payload ) ) );
}


/** \brief Sends 0x07 Vario */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_Vario( const crsf_Vario_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_VARIO, payload, Codec::Encode_Vario( data, payload ) ) );
}


/** \brief Sends 0x08 Battery */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_Battery( const crsf_Battery_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_BATTERY, payload, Codec::Encode_Battery( data, payload ) ) );
}


/** \brief Sends 0x09 Barometric altitude */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_BaroAltitude( const crsf_BaroAltitude_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_BARO_ALTITUDE, payload, Codec::Encode_BaroAltitude( data, payload ) ) );
}


/** \brief Sends 0x0B Heartbeat with own device address */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_Heartbeat( void )
{
    const crsf_Heartbeat_t data = { static_cast<int16_t>( sender.Get_DeviceAddr() ) };

    return ( sender.Send_Frame( CRSF_FRAMETYPE_HEARTBEAT, payload, Codec::Encode_Heartbeat( data, payload ) ) );
}


/** \brief Sends 0x10 VTX telemetry */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_VtxTelemetry( const crsf_VtxTelemetry_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_VTX_TELEMETRY, payload, Codec::Encode_VtxTelemetry( data, payload ) ) );
}


/** \brief Sends 0x14 Link statistics */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_LinkStatistics( const crsf_LinkStatistics_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_LINK_STATISTICS, payload, Codec::Encode_LinkStatistics( data, payload ) ) );
}


/** \brief Sends 0x16 RC channels packed */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_RcChannels( const crsf_RcChannels_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_RC_CHANNELS_PACKED, payload, Codec::Encode_RcChannels( data, payload ) ) );
}


/** \brief Sends 0x1C Link statistics RX */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_LinkStatisticsRx( const crsf_LinkStatisticsRx_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_LINK_STATISTICS_RX, payload, Codec::Encode_LinkStatisticsRx( data, payload ) ) );
}


/** \brief Sends 0x1D Link statistics TX */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_LinkStatisticsTx( const crsf_LinkStatisticsTx_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_LINK_STATISTICS_TX, payload, Codec::Encode_LinkStatisticsTx( data, payload ) ) );
}


/** \brief Sends 0x1E Attitude */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_Attitude( const crsf_Attitude_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_ATTITUDE, payload, Codec::Encode_Attitude( data, payload ) ) );
}


/** \brief Sends 0x1F MAVLink FC */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_MavLinkFc( const crsf_MavLinkFc_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_MAVLINK_FC, payload, Codec::Encode_MavLinkFc( data, payload ) ) );
}


/** \brief Sends 0x21 Flight mode (name is truncated to CRSF_STRING_MAX_LEN - 1 characters) */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_FlightMode( const char * const name )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_FLIGHT_MODE, payload, Codec::Encode_FlightMode( name, payload ) ) );
}


/** \brief Sends 0x22 ESP_NOW */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_EspNow( const crsf_EspNow_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_ESP_NOW, payload, Codec::Encode_EspNow( data, payload ) ) );
}


/**
 * \brief Sends 0x3A.0x10 Timing correction (extended frame)
 *
 * \param destAddr [in]: Destination address (usually remote control)
 * \param data     [in]: Timing correction
 */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_TimingCorrection( crsf_DeviceAddr_t destAddr, const crsf_TimingCorrection_t &data )
{
    return ( sender.Send_ExtFrame( CRSF_FRAMETYPE_REMOTE, destAddr, payload, Codec::Encode_TimingCorrection( data, payload ) ) );
}


/** \brief Sends 0xAC MAVLink system status */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_MavLinkSysStatus( const crsf_MavLinkSysStatus_t &data )
{
    return ( sender.Send_Frame( CRSF_FRAMETYPE_MAVLINK_SYS_STATUS, payload, Codec::Encode_MavLinkSysStatus( data, payload ) ) );
}


/**
 * \brief Sends 0x28 Parameter ping devices
 *
 * \param destAddr [in]: Pinged device or \ref CRSF_ADDR_BROADCAST for all devices
 */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_ParamPing( crsf_DeviceAddr_t destAddr )
{
    return ( sender.Send_ExtFrame( CRSF_FRAMETYPE_PARAM_PING, destAddr, nullptr, 0u ) );
}


/**
 * \brief Sends 0x32 Direct command (command CRC is appended)
 *
 * \param destAddr     [in]: Destination address
 * \param commandId    [in]: Command ID (CRSF_CMDID_xxx)
 * \param subCommandId [in]: Sub-command ID (CRSF_SUBCMD_xxx)
 * \param data         [in]: Command data (can be null if dataLen is 0)
 * \param dataLen      [in]: Command data length (maximally CRSF_EXT_PAYLOAD_MAX_LEN - 3)
 *
 * \return \ref CRSF_REQUEST_ERROR if data are too long or the frame could not be queued
 */
crsf_RequestState_t Crsf::AppLayer::TelemetryTx::Send_Command( crsf_DeviceAddr_t destAddr, uint8_t commandId, uint8_t subCommandId,
                                                               const uint8_t * const data, crsf_MsgLength_t dataLen )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( ( CRSF_EXT_PAYLOAD_MAX_LEN - crsf_CommandHeaderLen - crsf_CommandCrcLen ) >= dataLen )
    {
        const uint8_t header[] = { CRSF_FRAMETYPE_COMMAND, destAddr, sender.Get_DeviceAddr() };

        PayloadWriter writer( payload, CRSF_EXT_PAYLOAD_MAX_LEN );

        writer.Put_U8   ( commandId );
        writer.Put_U8   ( subCommandId );
        writer.Put_Bytes( data, dataLen );

        uint8_t commandCrc = Crc8::commandCrc.Get_Crc( header, sizeof( header ) );
        commandCrc         = Crc8::commandCrc.Update_Crc( commandCrc, payload, writer.Get_Length() );

        writer.Put_U8( commandCrc );

        retState = sender.Send_ExtFrame( CRSF_FRAMETYPE_COMMAND, destAddr, payload, writer.Get_Length() );
    }
    else
    {
        /* Data too long */
    }

    return ( retState );
}

/* ========================== LOCAL FUNCTIONS =============================== */

template< typename DataType >
static void crsf_DecodeAndNotify( crsf_RequestState_t ( *decoder )( const uint8_t * const, crsf_MsgLength_t, DataType & ),
                                  void ( *callback )( const DataType * ),
                                  const crsf_Frame_t &frame,
                                  crsf_Counter_t &invalidPayloadCnt )
{
    DataType data = {};

    if( nullptr == callback )
    {
        /* Frame type is not processed */
    }
    else if( CRSF_REQUEST_OK == decoder( frame.Payload, frame.PayloadLen, data ) )
    {
        callback( &data );
    }
    else
    {
        invalidPayloadCnt++;
    }
}

/* =============================== TASKS ==================================== */
