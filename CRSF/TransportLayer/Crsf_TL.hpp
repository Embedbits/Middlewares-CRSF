/**
 * \author Mr.Nobody
 * \file Crsf_TL.hpp
 * \ingroup Crsf
 * \brief CRSF Transport Layer (TL) header file.
 *
 * Transport layer finds frames in received byte stream, checks them (length,
 * CRC) and creates and transmits frames requested by the application layer:
 *
 * - Frame starts by sync byte. Accepted sync bytes are serial sync byte
 *   \ref CRSF_SYNC_BYTE, broadcast address \ref CRSF_ADDR_BROADCAST and own
 *   device address (\ref Crsf::TransportLayer::Set_DeviceAddr), or any byte
 *   in \ref CRSF_SYNC_ANY mode (transparent bridge).
 * - Frame length field out of range 2 - 62 or invalid CRC drops only the
 *   first byte and the search for sync byte continues with the next one, so
 *   valid frame following a damaged one is not lost.
 * - Transmitted frames are stored in a queue and they are sent one by one.
 */

#ifndef CRSF_TL_HPP
#define CRSF_TL_HPP
/* ============================= INCLUDES =================================== */
#include "Crsf_Types.h"                     /* Module types definition        */
#include "Crsf_Phy.hpp"                     /* Physical layer definition      */
/* ============================= TYPEDEFS =================================== */

/** \brief Actual state of transport layer */
typedef enum
{
    CRSF_TL_INIT = 0u, /**< Physical layer not initialized yet */
    CRSF_TL_ACTIVE     /**< Transport layer is running         */
}   crsf_TransportLayerState_t;


/** rief Accepted sync bytes of received frames */
typedef enum
{
    CRSF_SYNC_STANDARD = 0u, /**< Serial sync byte, broadcast address and own device address     */
    CRSF_SYNC_ANY            /**< Any byte (transparent bridge) - frame is checked by length and CRC */
}   crsf_SyncMode_t;

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* ========================= EXPORTED MACROS ================================ */

/* ========================= EXPORTED VARIABLES ============================= */

/* ======================== EXPORTED FUNCTIONS ============================== */

namespace Crsf
{
    class TransportLayer
    {
    public:
        TransportLayer( PhysicalLayer &physicalLayerObj );

        void Task( crsf_Time_t taskTime );

        crsf_RequestState_t Get_Frame( crsf_Frame_t &frame );

        crsf_RequestState_t Send_Frame   ( crsf_FrameType_t frameType,
                                           const uint8_t * const payload, crsf_MsgLength_t payloadLen );
        crsf_RequestState_t Send_ExtFrame( crsf_FrameType_t frameType,
                                           crsf_DeviceAddr_t destAddr, crsf_DeviceAddr_t origAddr,
                                           const uint8_t * const payload, crsf_MsgLength_t payloadLen );
        crsf_RequestState_t Send_RawFrame( const uint8_t * const frame, crsf_MsgLength_t frameLen );

        void              Set_DeviceAddr( crsf_DeviceAddr_t deviceAddr );
        crsf_DeviceAddr_t Get_DeviceAddr( void ) const;

        void              Set_TxSyncByte( crsf_DeviceAddr_t syncByte );
        crsf_DeviceAddr_t Get_TxSyncByte( void ) const;

        void            Set_SyncMode( crsf_SyncMode_t mode );
        crsf_SyncMode_t Get_SyncMode( void ) const;

        uint8_t Get_TxQueueFreeCnt( void ) const;

        crsf_Time_t Get_TimeSinceLastFrame( void ) const;

        crsf_StatsCounter_t Get_StatCounter( void ) const;
        void                Reset_StatCounter( void );

        crsf_TransportLayerState_t Get_ModuleState( void ) const;

        /** Count of frames which can wait for transmission */
        static constexpr uint8_t txQueueLen = 4u;

    private:

        /* ====================== SYMBOLIC CONSTANTS ======================== */

        /** Size of reception buffer - two complete frames */
        static constexpr crsf_MsgLength_t rxBufferSize = 2u * CRSF_FRAME_MAX_LEN;

        /** Position of frame length in frame */
        static constexpr crsf_MsgLength_t lengthPos = 1u;

        /** Position of frame type in frame */
        static constexpr crsf_MsgLength_t typePos = 2u;

        /** Position of destination address in extended frame */
        static constexpr crsf_MsgLength_t destAddrPos = 3u;

        /** Position of origin address in extended frame */
        static constexpr crsf_MsgLength_t origAddrPos = 4u;

        /** Maximal time since last frame (saturation of counter) */
        static constexpr crsf_Time_t timeSinceLastFrameMax = 0xFFFFFFFFu;

        /** Time without new byte after which incomplete data are resynchronized in ms.
         * Complete frame lasts 1.6 ms at 400 kBd, so the gap means the frame is broken. */
        static constexpr crsf_Time_t rxStallTimeout = 5u;

        /* ========================= TYPEDEFS =============================== */

        /** \brief Result of attempt to find frame at the beginning of reception buffer */
        typedef enum
        {
            PARSE_FRAME_FOUND = 0u, /**< Valid frame is at the beginning of buffer */
            PARSE_NEED_MORE_DATA,   /**< Buffer does not contain complete frame    */
            PARSE_BYTE_DROPPED      /**< Invalid byte was dropped, try again       */
        }   parseResult_t;


        /** \brief One frame waiting for transmission */
        typedef struct
        {
            uint8_t          Data[ CRSF_FRAME_MAX_LEN ]; /**< Complete frame */
            crsf_MsgLength_t Length;                     /**< Frame length   */
        }   txFrame_t;

        /* ==================== FORWARD DECLARATIONS ======================== */

        void          Read_PhyData( void );
        parseResult_t Parse_Frame( void );
        void          Drop_RxBytes( crsf_MsgLength_t count );
        bool          Is_SyncByte( uint8_t byte ) const;
        void          Fill_FrameDescription( crsf_Frame_t &frame ) const;
        txFrame_t    *Reserve_TxFrame( void );
        void          Commit_TxFrame( void );
        void          Process_Transmission( void );

        /* ======================= LOCAL VARIABLES ========================== */

        /** Object of physical layer handler */
        PhysicalLayer &physicalLayer;

        /** Actual state of transport layer */
        crsf_TransportLayerState_t moduleState = CRSF_TL_INIT;

        /** Own device address (accepted as sync byte, origin of sent frames) */
        crsf_DeviceAddr_t deviceAddress = CRSF_ADDR_FLIGHT_CONTROLLER;

        /** Sync byte of transmitted frames */
        crsf_DeviceAddr_t txSyncByte = CRSF_SYNC_BYTE;

        /** Accepted sync bytes of received frames */
        crsf_SyncMode_t syncMode = CRSF_SYNC_STANDARD;

        /** Reception buffer (byte stream) */
        uint8_t rxBuffer[ rxBufferSize ] = { 0u };

        /** Count of bytes in reception buffer */
        crsf_MsgLength_t rxCount = 0u;

        /** Copy of last valid frame provided to application layer */
        uint8_t frameBuffer[ CRSF_FRAME_MAX_LEN ] = { 0u };

        /** Length of frame in frame buffer */
        crsf_MsgLength_t frameLength = 0u;

        /** Transmit queue (ring buffer) */
        txFrame_t txQueue[ txQueueLen ] = {};

        /** Index of the oldest frame in transmit queue */
        uint8_t txHead = 0u;

        /** Count of frames in transmit queue */
        uint8_t txCount = 0u;

        /** Frame at the head of queue is being transmitted */
        bool txActive = false;

        /** Time since last valid frame in ms [0.001s] */
        crsf_Time_t timeSinceLastFrame = timeSinceLastFrameMax;

        /** Time since the last received byte while reception buffer is not empty in ms */
        crsf_Time_t rxStallTime = 0u;

        /** Structure with communication statistic counters */
        crsf_StatsCounter_t statsCounter = { 0u, 0u, 0u, 0u, 0u, 0u };
    };
} /* namespace Crsf */

#endif /* CRSF_TL_HPP */
