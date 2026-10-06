/**
 * \author Mr.Nobody
 * \file UtCrsf_Helpers.hpp
 * \ingroup UnitTests
 * \brief Helpers of CRSF application layer unit tests.
 */

#ifndef CRSF_UNITTESTS_UTCRSF_HELPERS_HPP
#define CRSF_UNITTESTS_UTCRSF_HELPERS_HPP

/* ============================= INCLUDES =================================== */
#include "unity.h"                          /* Unity testing framework        */
#include "UtCrsf_FakePhy.hpp"               /* Fake physical layer            */
#include "Crsf_App.hpp"                     /* Application layer core         */
#include "Crsf_TL.hpp"                      /* Transport layer                */
/* ========================= SYMBOLIC CONSTANTS ============================= */

namespace UtCrsf
{
    /** Address of device under test */
    static constexpr crsf_DeviceAddr_t OwnAddr = CRSF_ADDR_FLIGHT_CONTROLLER;

    /** Address of host (configuring device) */
    static constexpr crsf_DeviceAddr_t HostAddr = CRSF_ADDR_REMOTE_CONTROL;

    /** Parsed transmitted frame */
    typedef struct
    {
        uint8_t        Sync;
        uint8_t        Type;
        uint8_t        Dest;
        uint8_t        Orig;
        const uint8_t *Payload;
        uint8_t        PayloadLen;
    }   parsedFrame_t;


    /**
     * \brief Checks transmitted frame (length field, CRC by specification table) and parses it
     *
     * \param record   [in]: Transmitted frame
     * \param extended [in]: Frame has extended header
     *
     * \return Parsed frame
     */
    static inline parsedFrame_t Parse_Tx( const txRecord_t &record, bool extended )
    {
        parsedFrame_t parsed = {};

        TEST_ASSERT_TRUE( 4u <= record.Length );
        TEST_ASSERT_TRUE( CRSF_FRAME_MAX_LEN >= record.Length );
        TEST_ASSERT_EQUAL_UINT8( record.Length - 2u, record.Data[ 1 ] );
        TEST_ASSERT_EQUAL_HEX8( SpecCrc8( &record.Data[ 2 ], record.Length - 3u ), record.Data[ record.Length - 1u ] );

        parsed.Sync = record.Data[ 0 ];
        parsed.Type = record.Data[ 2 ];

        if( extended )
        {
            TEST_ASSERT_TRUE( 6u <= record.Length );

            parsed.Dest       = record.Data[ 3 ];
            parsed.Orig       = record.Data[ 4 ];
            parsed.Payload    = &record.Data[ 5 ];
            parsed.PayloadLen = static_cast<uint8_t>( record.Length - 6u );
        }
        else
        {
            parsed.Payload    = &record.Data[ 3 ];
            parsed.PayloadLen = static_cast<uint8_t>( record.Length - 4u );
        }

        return ( parsed );
    }


    /** Complete stack - fake physical layer, transport layer and application core */
    class Stack
    {
    public:

        Stack( Crsf::FrameHandler * const handlers[], uint8_t handlersCnt ) :
            tl( phy ),
            core( tl, handlers, handlersCnt )
        {
            /* Initialization of physical layer */
            core.Task( taskTime );
        }

        /** Runs count of application core tasks */
        void Run( uint8_t tasksCount = 1u )
        {
            for( uint8_t taskIndex = 0u; tasksCount > taskIndex; taskIndex++ )
            {
                core.Task( taskTime );
            }
        }

        /** Task period in ms */
        static constexpr crsf_Time_t taskTime = 1u;

        FakePhy                  phy;
        Crsf::TransportLayer     tl;
        Crsf::AppLayer::AppCore  core;
    };
}

#endif /* CRSF_UNITTESTS_UTCRSF_HELPERS_HPP */
