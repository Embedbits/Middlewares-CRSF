/**
 * \author Mr.Nobody
 * \file Crsf_App_Commands.hpp
 * \ingroup Crsf
 * \brief CRSF 0x32 Direct commands - reception and Command ACK.
 */

#ifndef APPLAYER_CRSF_APP_COMMANDS_HPP
#define APPLAYER_CRSF_APP_COMMANDS_HPP

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
     * \brief Receiver of direct commands
     *
     * - Command frame: command ID, sub-command ID, data, command CRC (polynomial
     *   0xBA over frame type, destination, origin, command ID, sub-command ID
     *   and data). Frame with invalid command CRC is ignored.
     * - Command is passed to callback configured for its command ID and
     *   sub-command ID.
     * - Command addressed directly to own device (not broadcast) is answered
     *   by Command ACK (0x32.0xFF) with action returned by callback, not
     *   configured command is acknowledged with \ref CRSF_CMD_ACTION_NONE.
     * - Received Command ACK is passed to callback configured with command ID
     *   \ref CRSF_CMDID_ACK and sub-command ID equal to acknowledged command
     *   ID (data: sub-command ID, action, information). ACK is never answered.
     */
    class Commands : public FrameHandler
    {
    public:
        Commands( const crsf_CommandConfig_t commandsConfig[], uint8_t commandsCount, bool ackEnabled = true );

        crsf_FrameTypeStatus_t Get_FrameTypeStatus( crsf_FrameType_t frameType ) const override;
        void                   Process_Frame( const crsf_Frame_t &frame, FrameSender &sender ) override;

        crsf_Counter_t Get_CrcErrorCnt( void ) const;

    private:

        void Send_Ack( const crsf_Command_t &command, crsf_CmdAction_t action, FrameSender &sender );

        /** Configuration of supported commands */
        const crsf_CommandConfig_t * const commands;

        /** Count of supported commands */
        const uint8_t commandsCnt;

        /** Command ACK is sent */
        const bool ackActive;

        /** Count of commands with invalid command CRC */
        crsf_Counter_t crcErrorCnt = 0u;
    };
} /* namespace AppLayer */
} /* namespace Crsf */

#endif /* APPLAYER_CRSF_APP_COMMANDS_HPP */
