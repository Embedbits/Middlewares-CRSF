/**
 * \author Mr.Nobody
 * \file Crsf_Crc.cpp
 * \ingroup Crsf
 * \brief CRSF CRC-8 calculation functionality.
 */

/* ============================= INCLUDES =================================== */
#include "Crsf_Crc.hpp"                     /* Self include                   */
#include "Crsf_Types.h"                     /* Module C types definition      */
/* ============================= TYPEDEFS =================================== */

/* ======================= FORWARD DECLARATIONS ============================= */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* ============================== MACROS ==================================== */

/* ========================= EXPORTED VARIABLES ============================= */

/** Frame CRC calculator (table is generated at compile time) */
constexpr Crsf::Crc8 Crsf::Crc8::frameCrc( Crsf::Crc8::framePolynomial );

/** Direct Commands CRC calculator (table is generated at compile time) */
constexpr Crsf::Crc8 Crsf::Crc8::commandCrc( Crsf::Crc8::commandPolynomial );

/* ========================== LOCAL VARIABLES =============================== */

/* ======================== EXPORTED FUNCTIONS ============================== */

/**
 * \brief Calculates CRC value of buffer
 *
 * \param buff       [in]: Pointer to buffer
 * \param buffLength [in]: Count of bytes used for calculation
 *
 * \return CRC value. Initial value (0) is returned in case of null pointer.
 */
uint8_t Crsf::Crc8::Get_Crc( const uint8_t * const buff, crsf_MsgLength_t buffLength ) const
{
    return ( Update_Crc( crcInitialValue, buff, buffLength ) );
}


/**
 * \brief Continues CRC calculation over next part of data
 *
 * \param crc        [in]: CRC of previous data (0 for first part)
 * \param buff       [in]: Pointer to buffer
 * \param buffLength [in]: Count of bytes used for calculation
 *
 * \return Updated CRC value. Input CRC is returned in case of null pointer.
 */
uint8_t Crsf::Crc8::Update_Crc( uint8_t crc, const uint8_t * const buff, crsf_MsgLength_t buffLength ) const
{
    uint8_t crcRegister = crc;

    if( nullptr != buff )
    {
        for( crsf_MsgLength_t buffIndex = 0u; buffLength > buffIndex; buffIndex++ )
        {
            crcRegister = table[ crcRegister ^ buff[ buffIndex ] ];
        }
    }
    else
    {
        /* Null pointer has been assigned */
    }

    return ( crcRegister );
}


/**
 * \brief Returns item of lookup table (used for verification)
 *
 * \param index [in]: Table index
 *
 * \return Table item
 */
uint8_t Crsf::Crc8::Get_TableItem( uint8_t index ) const
{
    return ( table[ index ] );
}

/* ========================== LOCAL FUNCTIONS =============================== */

/* =============================== TASKS ==================================== */
