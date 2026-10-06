/**
 * \author Mr.Nobody
 * \file Crsf_App_Bridge.hpp
 * \ingroup Crsf
 * \brief CRSF bridge - forwarding of frames between two CRSF ports.
 */

#ifndef APPLAYER_CRSF_APP_BRIDGE_HPP
#define APPLAYER_CRSF_APP_BRIDGE_HPP

/* ============================= INCLUDES =================================== */
#include "Crsf_App_Types.hpp"               /* Application layer types        */
#include "Crsf_TL.hpp"                      /* Transport layer                */
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
     * \brief Bridge of two CRSF ports
     *
     * Valid frames (checked by transport layer - sync, length, CRC) received on
     * one port are sent unchanged (incl. sync byte and CRC) to the other port.
     * Each port has its own transport layer and transmit queue, so the ports
     * can run with different baudrate or duplex mode.
     *
     * Modes of use:
     * - Standalone bridge (no local device): call \ref Task cyclically, it runs
     *   both transport layers and forwards all received frames. No application
     *   core may read frames from the same transport layers.
     * - Device with two ports: each port has its own \ref AppCore with
     *   handlers, the bridge is attached by
     *   `coreA.Set_FrameObserver( &bridge.Get_PortObserver( CRSF_BRIDGE_PORT_A ) )`
     *   (same for port B). Frames are processed locally and forwarded.
     *   \ref Task is not called in this mode. Enable \ref Set_SkipOwnFrames, so
     *   requests addressed to own device are not forwarded.
     *
     * Frames sent by the local device (responses, telemetry) are sent only on
     * the port of its application core. Loops in topology are forbidden by
     * specification (frames would be forwarded endlessly).
     */
    class Bridge
    {
    public:
        Bridge( TransportLayer &portA, TransportLayer &portB );

        void Task( crsf_Time_t taskTime );

        FrameObserver &Get_PortObserver( crsf_BridgePort_t port );

        void Set_Filter( crsf_BridgeFilter_t *filter );
        void Set_SkipOwnFrames( bool skipOwnFrames );

        crsf_BridgeStats_t Get_Stats( crsf_BridgePort_t sourcePort ) const;
        void               Reset_Stats( void );

        /** Maximal count of frames forwarded from one port in one task (standalone mode) */
        static constexpr uint8_t framesPerTaskMax = 8u;

    private:

        /** Observer of frames received on one port */
        class PortObserver : public FrameObserver
        {
        public:
            PortObserver( Bridge &bridgeObj, crsf_BridgePort_t sourcePort );

            void Observe_Frame( const crsf_Frame_t &frame ) override;

        private:
            Bridge                 &bridge;
            const crsf_BridgePort_t port;
        };

        void Forward_Frame( const crsf_Frame_t &frame, crsf_BridgePort_t sourcePort );

        /** Transport layers of ports */
        TransportLayer * const ports[ CRSF_BRIDGE_PORT_CNT ];

        /** Observers of ports */
        PortObserver observers[ CRSF_BRIDGE_PORT_CNT ];

        /** Statistics per source port */
        crsf_BridgeStats_t stats[ CRSF_BRIDGE_PORT_CNT ] = {};

        /** Application filter (optional) */
        crsf_BridgeFilter_t *bridgeFilter = nullptr;

        /** Extended frames addressed to own address of source port are not forwarded */
        bool skipOwn = false;
    };
} /* namespace AppLayer */
} /* namespace Crsf */

#endif /* APPLAYER_CRSF_APP_BRIDGE_HPP */
