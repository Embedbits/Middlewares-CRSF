/**
 * \author Mr.Nobody
 * \file Crsf_App_Telemetry.hpp
 * \ingroup Crsf
 * \brief CRSF telemetry - reception (callbacks) and transmission of frames.
 */

#ifndef APPLAYER_CRSF_APP_TELEMETRY_HPP
#define APPLAYER_CRSF_APP_TELEMETRY_HPP

/* ============================= INCLUDES =================================== */
#include "Crsf_App_Types.hpp"               /* Application layer types        */
/* ============================= TYPEDEFS =================================== */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* ========================= EXPORTED MACROS ================================ */

/* ========================= EXPORTED VARIABLES ============================= */

/* ======================== EXPORTED FUNCTIONS ============================== */

namespace Crsf
{
namespace AppLayer
{
    /**
     * \brief Receiver of telemetry frames
     *
     * Decodes received frames and passes them to application callbacks
     * (\ref crsf_TelemetryCallbacks_t). Frame type without callback is not
     * processed. Frames with too short payload are ignored and counted.
     */
    class TelemetryRx : public FrameHandler
    {
    public:
        explicit TelemetryRx( const crsf_TelemetryCallbacks_t &callbacks );

        crsf_FrameTypeStatus_t Get_FrameTypeStatus( crsf_FrameType_t frameType ) const override;
        void                   Process_Frame( const crsf_Frame_t &frame, FrameSender &sender ) override;

        crsf_Counter_t Get_InvalidPayloadCnt( void ) const;

    private:

        /** Application callbacks */
        const crsf_TelemetryCallbacks_t &telemetryCallbacks;

        /** Count of frames with invalid payload */
        crsf_Counter_t invalidPayloadCnt = 0u;
    };


    /**
     * \brief Transmitter of telemetry and other frames
     *
     * Encodes data structures and sends them by frame sender (application
     * core). Every function returns \ref CRSF_REQUEST_ERROR if the frame could
     * not be queued (transmit queue full).
     */
    class TelemetryTx
    {
    public:
        explicit TelemetryTx( FrameSender &frameSender );

        crsf_RequestState_t Send_Gps             ( const crsf_Gps_t &data );
        crsf_RequestState_t Send_GpsTime         ( const crsf_GpsTime_t &data );
        crsf_RequestState_t Send_GpsExtended     ( const crsf_GpsExtended_t &data );
        crsf_RequestState_t Send_Vario           ( const crsf_Vario_t &data );
        crsf_RequestState_t Send_Battery         ( const crsf_Battery_t &data );
        crsf_RequestState_t Send_BaroAltitude    ( const crsf_BaroAltitude_t &data );
        crsf_RequestState_t Send_Heartbeat       ( void );
        crsf_RequestState_t Send_VtxTelemetry    ( const crsf_VtxTelemetry_t &data );
        crsf_RequestState_t Send_LinkStatistics  ( const crsf_LinkStatistics_t &data );
        crsf_RequestState_t Send_RcChannels      ( const crsf_RcChannels_t &data );
        crsf_RequestState_t Send_LinkStatisticsRx( const crsf_LinkStatisticsRx_t &data );
        crsf_RequestState_t Send_LinkStatisticsTx( const crsf_LinkStatisticsTx_t &data );
        crsf_RequestState_t Send_Attitude        ( const crsf_Attitude_t &data );
        crsf_RequestState_t Send_MavLinkFc       ( const crsf_MavLinkFc_t &data );
        crsf_RequestState_t Send_FlightMode      ( const char * const name );
        crsf_RequestState_t Send_EspNow          ( const crsf_EspNow_t &data );
        crsf_RequestState_t Send_TimingCorrection( crsf_DeviceAddr_t destAddr, const crsf_TimingCorrection_t &data );
        crsf_RequestState_t Send_MavLinkSysStatus( const crsf_MavLinkSysStatus_t &data );
        crsf_RequestState_t Send_ParamPing       ( crsf_DeviceAddr_t destAddr );
        crsf_RequestState_t Send_Command         ( crsf_DeviceAddr_t destAddr, uint8_t commandId, uint8_t subCommandId,
                                                   const uint8_t * const data, crsf_MsgLength_t dataLen );

    private:

        /** Frame sender (application core) */
        FrameSender &sender;

        /** Payload buffer */
        uint8_t payload[ CRSF_PAYLOAD_MAX_LEN ] = { 0u };
    };
} /* namespace AppLayer */
} /* namespace Crsf */

#endif /* APPLAYER_CRSF_APP_TELEMETRY_HPP */
