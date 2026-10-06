/**
 * \author Mr.Nobody
 * \file UtCrsf_FakePhy.hpp
 * \ingroup UnitTests
 * \brief Fake physical layer and reference CRC of CRSF unit tests.
 *
 * Fake allows to inject received byte stream (optionally delivered in small
 * pieces) and records transmitted frames. Reference CRC uses lookup tables
 * copied from the CRSF specification.
 */

#ifndef CRSF_UNITTESTS_UTCRSF_FAKEPHY_HPP
#define CRSF_UNITTESTS_UTCRSF_FAKEPHY_HPP

/* ============================= INCLUDES =================================== */
#include "Crsf_Phy.hpp"                     /* Physical layer interface       */
#include "Crsf_Types.h"                     /* Module types definition        */
/* ============================= TYPEDEFS =================================== */

namespace UtCrsf
{
    /** Frame CRC (polynomial 0xD5) by table from specification */
    uint8_t SpecCrc8( const uint8_t *data, uint16_t len );

    /** Command CRC (polynomial 0xBA) by table from specification */
    uint8_t SpecCommandCrc8( const uint8_t *data, uint16_t len );

    /** Frame CRC table from specification */
    extern const uint8_t SpecCrcTable[ 256 ];

    /** Command CRC table from specification */
    extern const uint8_t SpecCommandCrcTable[ 256 ];

    /** Transmitted frame recorded by fake */
    typedef struct
    {
        uint8_t  Data[ CRSF_FRAME_MAX_LEN ];
        uint16_t Length;
    }   txRecord_t;


    class FakePhy : public Crsf::PhysicalLayer
    {
    public:

        void             Set_Transmission_Start  ( const uint8_t *msgBuffer, crsf_MsgLength_t msgLen ) override;
        crsf_TxStatus_t  Get_Transmission_Status ( void ) override;
        crsf_MsgLength_t Get_RxData              ( uint8_t *buffer, crsf_MsgLength_t maxLen ) override;
        void             Set_Baudrate            ( crsf_Baudrate_t baudrate ) override;
        void             PhyInit                 ( void ) override;

        /* ------------------------- Test helpers --------------------------- */

        /** Appends bytes to received stream */
        void Inject( const uint8_t *data, uint16_t len );

        /** Appends frame with short header (reference CRC) to received stream */
        void Inject_Frame( uint8_t type, const uint8_t *payload, uint8_t payloadLen, uint8_t syncByte = CRSF_SYNC_BYTE );

        /** Appends frame with extended header (reference CRC) to received stream */
        void Inject_ExtFrame( uint8_t type, uint8_t dest, uint8_t orig, const uint8_t *payload, uint8_t payloadLen,
                              uint8_t syncByte = CRSF_SYNC_BYTE );

        /** Count of not read bytes of received stream */
        uint16_t Get_RxPending( void ) const;

        /** Clears recorded transmitted frames */
        void Clear_Tx( void );

        /** Maximal count of recorded transmitted frames */
        static constexpr uint8_t txRecordsMax = 32u;

        /** Size of received stream buffer */
        static constexpr uint16_t rxStreamSize = 1024u;

        /** Received stream */
        uint8_t  rxStream[ rxStreamSize ] = { 0u };
        uint16_t rxWritePos = 0u;
        uint16_t rxReadPos  = 0u;

        /** Maximal count of bytes delivered by one Get_RxData call */
        crsf_MsgLength_t rxChunkMax = 0xFFu;

        /** Recorded transmitted frames */
        txRecord_t txFrames[ txRecordsMax ] = {};
        uint8_t    txFramesCnt = 0u;

        /** Transmission status reported to transport layer */
        crsf_TxStatus_t txStatus = CRSF_TX_FINISHED;

        /** Transmission is reported as finished immediately */
        bool txAutoFinish = true;

        /* Call counters */
        uint32_t txStartCnt = 0u;
        uint32_t initCnt    = 0u;
        uint32_t rxReadCnt  = 0u;

        /** Configured baudrate */
        crsf_Baudrate_t baudrate = 0u;
    };
}

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* ========================= EXPORTED MACROS ================================ */

/* ========================= EXPORTED VARIABLES ============================= */

/* ======================== EXPORTED FUNCTIONS ============================== */

#endif /* CRSF_UNITTESTS_UTCRSF_FAKEPHY_HPP */
