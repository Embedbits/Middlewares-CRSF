/**
 * \author Mr.Nobody
 * \file Crsf_App.hpp
 * \ingroup Crsf
 * \brief CRSF application layer core header file
 *
 */

#ifndef APPLAYER_CRSF_APP_HPP
#define APPLAYER_CRSF_APP_HPP

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
     * \brief Application layer core
     *
     * - Reads received frames from transport layer and passes them to all
     *   handlers supporting the frame type.
     * - Extended frames are passed to handlers only if they are addressed to
     *   own device or broadcast.
     * - Optional frame callback and frame observer are called for every
     *   received frame (eg. \ref Crsf::AppLayer::Bridge forwarding frames to
     *   other CRSF port).
     * - Provides \ref Crsf::FrameSender interface to handlers and application
     *   (origin of extended frames is own device address).
     */
    class AppCore : public FrameSender
    {
    public:
        AppCore( TransportLayer &transportLayerObj,
                 FrameHandler * const frameHandlers[],
                 uint8_t handlersCount );

        void Task( crsf_Time_t taskTime );

        crsf_RequestState_t Send_Frame   ( crsf_FrameType_t frameType,
                                           const uint8_t * const payload, crsf_MsgLength_t payloadLen ) override;
        crsf_RequestState_t Send_ExtFrame( crsf_FrameType_t frameType, crsf_DeviceAddr_t destAddr,
                                           const uint8_t * const payload, crsf_MsgLength_t payloadLen ) override;
        crsf_DeviceAddr_t   Get_DeviceAddr( void ) const override;

        void Set_DeviceAddr( crsf_DeviceAddr_t deviceAddr );

        void Set_FrameCallback( crsf_FrameCallback_t *callback );
        void Set_FrameObserver( FrameObserver *observer );

        /** Maximal count of frames processed in one task (other frames wait for next task) */
        static constexpr uint8_t framesPerTaskMax = 8u;

    private:

        void Dispatch_Frame( const crsf_Frame_t &frame );

        /** Transport layer object reference */
        TransportLayer &transportLayer;

        /** Pointer to array of frame handlers */
        FrameHandler * const *handlers;

        /** Count of frame handlers */
        uint8_t handlersCnt = 0u;

        /** Callback of all received frames (optional) */
        crsf_FrameCallback_t *frameCallback = nullptr;

        /** Observer of all received frames (optional) */
        FrameObserver *frameObserver = nullptr;
    };
} /* namespace AppLayer */
} /* namespace Crsf */

#endif /* APPLAYER_CRSF_APP_HPP */
