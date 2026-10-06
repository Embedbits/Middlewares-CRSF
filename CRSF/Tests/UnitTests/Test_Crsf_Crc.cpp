/**
 * \author Mr.Nobody
 * \file Test_Crsf_Crc.cpp
 * \ingroup UnitTests
 * \brief Unit tests of CRSF CRC-8 calculation.
 */

/* ============================= INCLUDES =================================== */
#include "unity.h"                          /* Unity testing framework        */
#include "Crsf_Crc.hpp"                     /* Code under test                */
#include "UtCrsf_FakePhy.hpp"               /* Specification tables           */
/* ======================== EXPORTED FUNCTIONS ============================== */

void setUp( void )
{
}


void tearDown( void )
{
}


/** Generated frame CRC table is equal to crc8tab of specification */
void Ut_Crc_FrameTable_EqualsSpecification( void )
{
    for( uint16_t index = 0u; 256u > index; index++ )
    {
        TEST_ASSERT_EQUAL_HEX8( UtCrsf::SpecCrcTable[ index ],
                                Crsf::Crc8::frameCrc.Get_TableItem( static_cast<uint8_t>( index ) ) );
    }
}


/** Generated command CRC table is equal to command_crc8tab of specification */
void Ut_Crc_CommandTable_EqualsSpecification( void )
{
    for( uint16_t index = 0u; 256u > index; index++ )
    {
        TEST_ASSERT_EQUAL_HEX8( UtCrsf::SpecCommandCrcTable[ index ],
                                Crsf::Crc8::commandCrc.Get_TableItem( static_cast<uint8_t>( index ) ) );
    }
}


/** CRC-8/DVB-S2 (polynomial 0xD5) check value of "123456789" is 0xBC */
void Ut_Crc_Frame_StandardCheckValue( void )
{
    const uint8_t data[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

    TEST_ASSERT_EQUAL_HEX8( 0xBCu, Crsf::Crc8::frameCrc.Get_Crc( data, sizeof( data ) ) );
}


/** Command CRC equals specification table implementation */
void Ut_Crc_Command_EqualsSpecificationAlgorithm( void )
{
    const uint8_t data[] = { 0x32, 0xC8, 0xEA, 0x01, 0x01, 0x55, 0xAA };

    TEST_ASSERT_EQUAL_HEX8( UtCrsf::SpecCommandCrc8( data, sizeof( data ) ),
                            Crsf::Crc8::commandCrc.Get_Crc( data, sizeof( data ) ) );
}


/** Calculation in parts gives the same result as in one step */
void Ut_Crc_Update_ContinuesCalculation( void )
{
    const uint8_t data[] = { 0x16, 0xE0, 0x03, 0x1F, 0xF8, 0xC0, 0x07, 0x3E };

    const uint8_t partCrc = Crsf::Crc8::frameCrc.Get_Crc( data, 3u );

    TEST_ASSERT_EQUAL_HEX8( Crsf::Crc8::frameCrc.Get_Crc( data, sizeof( data ) ),
                            Crsf::Crc8::frameCrc.Update_Crc( partCrc, &data[ 3 ], sizeof( data ) - 3u ) );
}


void Ut_Crc_EmptyBufferReturnsInitValue( void )
{
    const uint8_t data[] = { 0x55 };

    TEST_ASSERT_EQUAL_HEX8( 0x00u, Crsf::Crc8::frameCrc.Get_Crc( data, 0u ) );
}


void Ut_Crc_NullPointerKeepsCrc( void )
{
    TEST_ASSERT_EQUAL_HEX8( 0x00u, Crsf::Crc8::frameCrc.Get_Crc( nullptr, 5u ) );
    TEST_ASSERT_EQUAL_HEX8( 0x5Au, Crsf::Crc8::frameCrc.Update_Crc( 0x5Au, nullptr, 5u ) );
}
