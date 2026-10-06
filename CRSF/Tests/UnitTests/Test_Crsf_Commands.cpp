/**
 * \author Mr.Nobody
 * \file Test_Crsf_Commands.cpp
 * \ingroup UnitTests
 * \brief Unit tests of CRSF direct commands (command CRC, dispatching, Command ACK).
 */

/* ============================= INCLUDES =================================== */
#include "unity.h"                          /* Unity testing framework        */
#include "UtCrsf_Helpers.hpp"               /* Test stack                     */
#include "Crsf_App_Commands.hpp"            /* Code under test                */
#include <string.h>                         /* memcpy                         */
/* ========================== LOCAL VARIABLES =============================== */

static uint32_t       disarmCnt   = 0u;
static uint32_t       powerCnt    = 0u;
static uint32_t       ackCnt      = 0u;
static crsf_Command_t lastCommand = {};
static uint8_t        lastData[ CRSF_EXT_PAYLOAD_MAX_LEN ];

/* ========================== LOCAL FUNCTIONS =============================== */

static crsf_CmdAction_t ForceDisarm( const crsf_Command_t *command )
{
    disarmCnt++;
    lastCommand = *command;
    return ( CRSF_CMD_ACTION_TAKEN );
}

static crsf_CmdAction_t SetPower( const crsf_Command_t *command )
{
    powerCnt++;
    lastCommand = *command;
    memcpy( lastData, command->Data, command->DataLen );
    return ( CRSF_CMD_ACTION_TAKEN );
}

static crsf_CmdAction_t AckReceived( const crsf_Command_t *command )
{
    ackCnt++;
    lastCommand = *command;
    memcpy( lastData, command->Data, command->DataLen );
    return ( CRSF_CMD_ACTION_NONE );
}

static const crsf_CommandConfig_t commandsConfig[] =
{
    { CRSF_CMDID_FC,  CRSF_SUBCMD_FC_FORCE_DISARM, ForceDisarm },
    { CRSF_CMDID_VTX, CRSF_SUBCMD_VTX_SET_POWER,   SetPower    },
    { CRSF_CMDID_ACK, CRSF_CMDID_BLUETOOTH,        AckReceived },
};

/* ========================== HELPER FUNCTIONS ============================== */

/**
 * \brief Injects command frame with command CRC (computed by specification table)
 *
 * \param crcCorrect [in]: False corrupts command CRC (frame CRC stays valid)
 */
static void InjectCommand( UtCrsf::FakePhy &phy, uint8_t dest, uint8_t cmdId, uint8_t subCmdId,
                           const uint8_t *data, uint8_t dataLen, bool crcCorrect = true )
{
    uint8_t crcData[ 64 ] = { CRSF_FRAMETYPE_COMMAND, dest, UtCrsf::HostAddr, cmdId, subCmdId };
    uint8_t payload[ 64 ] = { cmdId, subCmdId };

    if( 0u < dataLen )
    {
        memcpy( &crcData[ 5 ], data, dataLen );
        memcpy( &payload[ 2 ], data, dataLen );
    }

    payload[ 2u + dataLen ] = UtCrsf::SpecCommandCrc8( crcData, static_cast<uint16_t>( 5u + dataLen ) );

    if( false == crcCorrect )
    {
        payload[ 2u + dataLen ] ^= 0x01u;
    }

    phy.Inject_ExtFrame( CRSF_FRAMETYPE_COMMAND, dest, UtCrsf::HostAddr, payload, static_cast<uint8_t>( 3u + dataLen ) );
}


/** Checks Command ACK frame */
static void CheckAck( const UtCrsf::txRecord_t &record, uint8_t cmdId, uint8_t subCmdId, crsf_CmdAction_t action )
{
    const UtCrsf::parsedFrame_t frame = UtCrsf::Parse_Tx( record, true );

    TEST_ASSERT_EQUAL_HEX8( CRSF_FRAMETYPE_COMMAND, frame.Type );
    TEST_ASSERT_EQUAL_HEX8( UtCrsf::HostAddr, frame.Dest );
    TEST_ASSERT_EQUAL_HEX8( UtCrsf::OwnAddr, frame.Orig );

    const uint8_t expected[] = { CRSF_CMDID_ACK, cmdId, subCmdId, static_cast<uint8_t>( action ), 0x00 };

    TEST_ASSERT_EQUAL_UINT8( sizeof( expected ) + 1u, frame.PayloadLen );
    TEST_ASSERT_EQUAL_HEX8_ARRAY( expected, frame.Payload, sizeof( expected ) );

    /* Command CRC over type, destination, origin and payload without CRC */
    uint8_t crcData[ 64 ] = { CRSF_FRAMETYPE_COMMAND, frame.Dest, frame.Orig };

    memcpy( &crcData[ 3 ], frame.Payload, sizeof( expected ) );

    TEST_ASSERT_EQUAL_HEX8( UtCrsf::SpecCommandCrc8( crcData, 3u + sizeof( expected ) ), frame.Payload[ sizeof( expected ) ] );
}

/* ======================== EXPORTED FUNCTIONS ============================== */

void setUp( void )
{
    disarmCnt = 0u;
    powerCnt  = 0u;
    ackCnt    = 0u;
}


void tearDown( void )
{
}


/** Configured command is executed and acknowledged with action taken */
void Ut_Commands_ExecutedAndAcknowledged( void )
{
    Crsf::AppLayer::Commands    commands( commandsConfig, 3u );
    Crsf::FrameHandler * const  handlers[] = { &commands };
    UtCrsf::Stack               stack( handlers, 1u );

    InjectCommand( stack.phy, UtCrsf::OwnAddr, CRSF_CMDID_FC, CRSF_SUBCMD_FC_FORCE_DISARM, nullptr, 0u );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 1u, disarmCnt );
    TEST_ASSERT_EQUAL_HEX8( UtCrsf::HostAddr, lastCommand.OrigAddr );
    TEST_ASSERT_EQUAL_HEX8( UtCrsf::OwnAddr, lastCommand.DestAddr );
    TEST_ASSERT_EQUAL_UINT8( 0u, lastCommand.DataLen );

    TEST_ASSERT_EQUAL_UINT8( 1u, stack.phy.txFramesCnt );
    CheckAck( stack.phy.txFrames[ 0 ], CRSF_CMDID_FC, CRSF_SUBCMD_FC_FORCE_DISARM, CRSF_CMD_ACTION_TAKEN );
}


/** Command data (without command CRC) are passed to callback */
void Ut_Commands_DataPassed( void )
{
    Crsf::AppLayer::Commands    commands( commandsConfig, 3u );
    Crsf::FrameHandler * const  handlers[] = { &commands };
    UtCrsf::Stack               stack( handlers, 1u );
    const uint8_t               power[] = { 25u };

    InjectCommand( stack.phy, UtCrsf::OwnAddr, CRSF_CMDID_VTX, CRSF_SUBCMD_VTX_SET_POWER, power, sizeof( power ) );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 1u, powerCnt );
    TEST_ASSERT_EQUAL_UINT8( 1u, lastCommand.DataLen );
    TEST_ASSERT_EQUAL_UINT8( 25u, lastData[ 0 ] );
}


/** Not configured command is acknowledged with action none */
void Ut_Commands_UnknownCommandAckNone( void )
{
    Crsf::AppLayer::Commands    commands( commandsConfig, 3u );
    Crsf::FrameHandler * const  handlers[] = { &commands };
    UtCrsf::Stack               stack( handlers, 1u );

    InjectCommand( stack.phy, UtCrsf::OwnAddr, CRSF_CMDID_LED, CRSF_SUBCMD_LED_SET_DEFAULT, nullptr, 0u );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 0u, disarmCnt );
    TEST_ASSERT_EQUAL_UINT8( 1u, stack.phy.txFramesCnt );
    CheckAck( stack.phy.txFrames[ 0 ], CRSF_CMDID_LED, CRSF_SUBCMD_LED_SET_DEFAULT, CRSF_CMD_ACTION_NONE );
}


/** Command with invalid command CRC is ignored and counted */
void Ut_Commands_InvalidCommandCrcIgnored( void )
{
    Crsf::AppLayer::Commands    commands( commandsConfig, 3u );
    Crsf::FrameHandler * const  handlers[] = { &commands };
    UtCrsf::Stack               stack( handlers, 1u );

    InjectCommand( stack.phy, UtCrsf::OwnAddr, CRSF_CMDID_FC, CRSF_SUBCMD_FC_FORCE_DISARM, nullptr, 0u, false );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 0u, disarmCnt );
    TEST_ASSERT_EQUAL_UINT8( 0u, stack.phy.txFramesCnt );
    TEST_ASSERT_EQUAL_UINT32( 1u, commands.Get_CrcErrorCnt() );
}


/** Broadcast command is executed but not acknowledged */
void Ut_Commands_BroadcastNotAcknowledged( void )
{
    Crsf::AppLayer::Commands    commands( commandsConfig, 3u );
    Crsf::FrameHandler * const  handlers[] = { &commands };
    UtCrsf::Stack               stack( handlers, 1u );

    InjectCommand( stack.phy, CRSF_ADDR_BROADCAST, CRSF_CMDID_FC, CRSF_SUBCMD_FC_FORCE_DISARM, nullptr, 0u );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 1u, disarmCnt );
    TEST_ASSERT_EQUAL_UINT8( 0u, stack.phy.txFramesCnt );
}


/** ACK can be disabled */
void Ut_Commands_AckDisabled( void )
{
    Crsf::AppLayer::Commands    commands( commandsConfig, 3u, false );
    Crsf::FrameHandler * const  handlers[] = { &commands };
    UtCrsf::Stack               stack( handlers, 1u );

    InjectCommand( stack.phy, UtCrsf::OwnAddr, CRSF_CMDID_FC, CRSF_SUBCMD_FC_FORCE_DISARM, nullptr, 0u );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 1u, disarmCnt );
    TEST_ASSERT_EQUAL_UINT8( 0u, stack.phy.txFramesCnt );
}


/** Received Command ACK is passed to ACK callback and never answered */
void Ut_Commands_AckReceived( void )
{
    Crsf::AppLayer::Commands    commands( commandsConfig, 3u );
    Crsf::FrameHandler * const  handlers[] = { &commands };
    UtCrsf::Stack               stack( handlers, 1u );
    const uint8_t               ackData[] = { CRSF_SUBCMD_BT_RESET, CRSF_CMD_ACTION_TAKEN, 0x00 };

    InjectCommand( stack.phy, UtCrsf::OwnAddr, CRSF_CMDID_ACK, CRSF_CMDID_BLUETOOTH, ackData, sizeof( ackData ) );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 1u, ackCnt );
    TEST_ASSERT_EQUAL_UINT8( sizeof( ackData ), lastCommand.DataLen );
    TEST_ASSERT_EQUAL_UINT8( CRSF_CMD_ACTION_TAKEN, lastData[ 1 ] );
    TEST_ASSERT_EQUAL_UINT8( 0u, stack.phy.txFramesCnt );
}


/** Too short command frame is ignored */
void Ut_Commands_ShortFrameIgnored( void )
{
    Crsf::AppLayer::Commands    commands( commandsConfig, 3u );
    Crsf::FrameHandler * const  handlers[] = { &commands };
    UtCrsf::Stack               stack( handlers, 1u );
    const uint8_t               payload[] = { CRSF_CMDID_FC, 0x00 };

    stack.phy.Inject_ExtFrame( CRSF_FRAMETYPE_COMMAND, UtCrsf::OwnAddr, UtCrsf::HostAddr, payload, sizeof( payload ) );
    stack.Run();

    TEST_ASSERT_EQUAL_UINT32( 0u, disarmCnt );
    TEST_ASSERT_EQUAL_UINT8( 0u, stack.phy.txFramesCnt );
    TEST_ASSERT_EQUAL_UINT32( 0u, commands.Get_CrcErrorCnt() );
}
