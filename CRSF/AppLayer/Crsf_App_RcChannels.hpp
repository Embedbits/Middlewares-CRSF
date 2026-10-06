/**
 * \author Mr.Nobody
 * \file Crsf_App_RcChannels.hpp
 * \ingroup Crsf
 * \brief CRSF 0x16 RC channels packed - receiver with failsafe detection.
 */

#ifndef APPLAYER_CRSF_APP_RCCHANNELS_HPP
#define APPLAYER_CRSF_APP_RCCHANNELS_HPP

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
     * \brief Receiver of RC channels
     *
     * Stores the last received channels. RC channels frame is not sent in case
     * of failsafe (failsafe type "cut"), so the link is reported as lost when
     * no frame is received within failsafe timeout (1 s recommended by
     * specification). Channels keep the last received values.
     */
    class RcChannels : public FrameHandler
    {
    public:
        explicit RcChannels( crsf_Time_t failsafeTimeoutMs = failsafeTimeoutDefault );

        crsf_FrameTypeStatus_t Get_FrameTypeStatus( crsf_FrameType_t frameType ) const override;
        void                   Process_Frame( const crsf_Frame_t &frame, FrameSender &sender ) override;
        void                   Task( crsf_Time_t taskTime, FrameSender &sender ) override;

        void Set_ChannelsCallback ( crsf_RcChannelsCallback_t *callback );
        void Set_LinkStateCallback( crsf_LinkStateCallback_t *callback );

        crsf_LinkState_t         Get_LinkState( void ) const;
        const crsf_RcChannels_t &Get_Channels( void ) const;
        uint16_t                 Get_Channel( uint8_t channelIndex ) const;
        int32_t                  Get_ChannelUs( uint8_t channelIndex ) const;
        crsf_Counter_t           Get_FramesCnt( void ) const;

        /** Default failsafe timeout in ms [0.001s] */
        static constexpr crsf_Time_t failsafeTimeoutDefault = 1000u;

    private:

        void Change_LinkState( crsf_LinkState_t newState );

        /** Failsafe timeout in ms */
        const crsf_Time_t failsafeTimeout;

        /** Time since last frame in ms */
        crsf_Time_t timeSinceFrame = 0u;

        /** Actual link state */
        crsf_LinkState_t linkState = CRSF_LINK_LOST;

        /** Last received channels (center value before the first frame) */
        crsf_RcChannels_t channels = {};

        /** Count of received valid RC channels frames */
        crsf_Counter_t framesCnt = 0u;

        /** Application callback of new channels (optional) */
        crsf_RcChannelsCallback_t *channelsCallback = nullptr;

        /** Application callback of link state change (optional) */
        crsf_LinkStateCallback_t *linkStateCallback = nullptr;
    };
} /* namespace AppLayer */
} /* namespace Crsf */

#endif /* APPLAYER_CRSF_APP_RCCHANNELS_HPP */
