/**
 * \author Mr.Nobody
 * \file Crsf_App_Commands.cpp
 * \ingroup Crsf
 * \brief CRSF 0x32 Direct commands - reception and Command ACK.
 */

/* ============================= INCLUDES =================================== */
#include "Crsf_App_Commands.hpp"            /* Self include                   */
#include "Crsf_App_Codec.hpp"               /* Payload writer                 */
#include "Crsf_Crc.hpp"                     /* Command CRC                    */
/* ============================= TYPEDEFS =================================== */

/* ======================= FORWARD DECLARATIONS ============================= */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/** Position of command ID in payload */
static constexpr crsf_MsgLength_t crsf_CommandIdPos = 0u;

/** Position of sub-command ID in payload */
static constexpr crsf_MsgLength_t crsf_SubCommandIdPos = 1u;

/** Position of command data in payload */
static constexpr crsf_MsgLength_t crsf_CommandDataPos = 2u;

/** Length of command CRC */
static constexpr crsf_MsgLength_t crsf_CommandCrcLen = 1u;

/** Minimal payload length - command ID, sub-command ID and command CRC */
static constexpr crsf_MsgLength_t crsf_CommandMinLen = crsf_CommandDataPos + crsf_CommandCrcLen;

/* ============================== MACROS ==================================== */

/* ========================= EXPORTED VARIABLES ============================= */

/* ========================== LOCAL VARIABLES =============================== */

/* ======================== EXPORTED FUNCTIONS ============================== */

/**
 * \brief Constructor of direct commands receiver
 *
 * \param commandsConfig [in]: Supported commands (can be null if count is 0)
 * \param commandsCount  [in]: Count of supported commands
 * \param ackEnabled     [in]: Commands addressed to own device are acknowledged
 */
Crsf::AppLayer::Commands::Commands( const crsf_CommandConfig_t commandsConfig[], uint8_t commandsCount, bool ackEnabled ) :
    commands( commandsConfig ),
    commandsCnt( ( nullptr != commandsConfig ) ? commandsCount : 0u ),
    ackActive( ackEnabled )
{
}


/** \brief Handler processes only \ref CRSF_FRAMETYPE_COMMAND */
crsf_FrameTypeStatus_t Crsf::AppLayer::Commands::Get_FrameTypeStatus( crsf_FrameType_t frameType ) const
{
    return ( ( CRSF_FRAMETYPE_COMMAND == frameType ) ? CRSF_FRAMETYPE_SUPPORTED : CRSF_FRAMETYPE_NOT_SUPPORTED );
}


/**
 * \brief Checks command CRC, executes command and sends ACK
 *
 * \param frame  [in]: Received frame
 * \param sender [in]: Interface used for ACK
 */
void Crsf::AppLayer::Commands::Process_Frame( const crsf_Frame_t &frame, FrameSender &sender )
{
    if( crsf_CommandMinLen <= frame.PayloadLen )
    {
        const uint8_t          header[]   = { frame.Type, frame.DestAddr, frame.OrigAddr };
        const crsf_MsgLength_t dataLength = frame.PayloadLen - crsf_CommandCrcLen;

        uint8_t commandCrc = Crc8::commandCrc.Get_Crc( header, sizeof( header ) );
        commandCrc         = Crc8::commandCrc.Update_Crc( commandCrc, frame.Payload, dataLength );

        if( commandCrc == frame.Payload[ dataLength ] )
        {
            const crsf_Command_t command =
            {
                frame.OrigAddr,
                frame.DestAddr,
                frame.Payload[ crsf_CommandIdPos ],
                frame.Payload[ crsf_SubCommandIdPos ],
                &frame.Payload[ crsf_CommandDataPos ],
                static_cast<crsf_MsgLength_t>( dataLength - crsf_CommandDataPos )
            };

            crsf_CmdAction_t action = CRSF_CMD_ACTION_NONE;

            for( uint8_t commandIndex = 0u; commandsCnt > commandIndex; commandIndex++ )
            {
                if( ( commands[ commandIndex ].CommandId    == command.CommandId    ) &&
                    ( commands[ commandIndex ].SubCommandId == command.SubCommandId ) &&
                    ( nullptr                               != commands[ commandIndex ].Callback ) )
                {
                    action = commands[ commandIndex ].Callback( &command );
                }
                else
                {
                    /* Other command */
                }
            }

            if( ( true                    == ackActive         ) &&
                ( CRSF_CMDID_ACK          != command.CommandId ) &&
                ( sender.Get_DeviceAddr() == frame.DestAddr    )    )
            {
                Send_Ack( command, action, sender );
            }
            else
            {
                /* ACK not requested */
            }
        }
        else
        {
            crcErrorCnt++;
        }
    }
    else
    {
        /* Frame too short */
    }
}


/** \brief Returns count of commands with invalid command CRC */
crsf_Counter_t Crsf::AppLayer::Commands::Get_CrcErrorCnt( void ) const
{
    return ( crcErrorCnt );
}

/* ========================== LOCAL FUNCTIONS =============================== */

/**
 * \brief Sends Command ACK to origin of command
 *
 * \param command [in]: Acknowledged command
 * \param action  [in]: Result of execution
 * \param sender  [in]: Interface used for sending
 */
void Crsf::AppLayer::Commands::Send_Ack( const crsf_Command_t &command, crsf_CmdAction_t action, FrameSender &sender )
{
    const uint8_t header[] = { CRSF_FRAMETYPE_COMMAND, command.OrigAddr, sender.Get_DeviceAddr() };

    uint8_t       payload[ CRSF_EXT_PAYLOAD_MAX_LEN ] = { 0u };
    PayloadWriter writer( payload, CRSF_EXT_PAYLOAD_MAX_LEN );

    writer.Put_U8    ( CRSF_CMDID_ACK );
    writer.Put_U8    ( command.CommandId );
    writer.Put_U8    ( command.SubCommandId );
    writer.Put_U8    ( static_cast<uint8_t>( action ) );
    writer.Put_String( nullptr );

    uint8_t commandCrc = Crc8::commandCrc.Get_Crc( header, sizeof( header ) );
    commandCrc         = Crc8::commandCrc.Update_Crc( commandCrc, payload, writer.Get_Length() );

    writer.Put_U8( commandCrc );

    (void)sender.Send_ExtFrame( CRSF_FRAMETYPE_COMMAND, command.OrigAddr, payload, writer.Get_Length() );
}

/* =============================== TASKS ==================================== */
