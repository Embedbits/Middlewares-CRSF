/**
 * \author Mr.Nobody
 * \file Crsf_App_Params.hpp
 * \ingroup Crsf
 * \brief CRSF device parameters - 0x28 ping, 0x29 device information,
 *        0x2B parameter entry, 0x2C parameter read, 0x2D parameter write.
 */

#ifndef APPLAYER_CRSF_APP_PARAMS_HPP
#define APPLAYER_CRSF_APP_PARAMS_HPP

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
     * \brief Parameters of device (configuration by host, eg. remote control)
     *
     * - Ping (to own address or broadcast) is answered by device information.
     * - Parameter read is answered by parameter entry. Entry longer than one
     *   frame is split into chunks of \ref chunkSizeMax bytes; the entry is
     *   built when chunk 0 is requested and the following chunks are sent from
     *   the same (cached) entry, so all chunks are consistent.
     * - Parameter 0 is root folder named by device name, it contains all
     *   parameters with parent folder 0.
     * - Parameter number out of range is answered by entry of type
     *   \ref CRSF_PARAM_OUT_OF_RANGE.
     * - Parameter write calls setter (value out of range is not passed to
     *   setter) and it is answered by the actual parameter entry.
     * - Responses are sent to origin of the request.
     */
    class Params : public FrameHandler
    {
    public:
        Params( const crsf_DeviceInfo_t &deviceInfo,
                const crsf_ParamConfig_t paramsConfig[],
                uint8_t paramsCount );

        crsf_FrameTypeStatus_t Get_FrameTypeStatus( crsf_FrameType_t frameType ) const override;
        void                   Process_Frame( const crsf_Frame_t &frame, FrameSender &sender ) override;
        void                   Task( crsf_Time_t taskTime, FrameSender &sender ) override;

        crsf_RequestState_t Request_EntryUpdate( crsf_ParamNumber_t paramNumber, crsf_DeviceAddr_t destAddr );

        /** Maximal size of parameter entry (all chunks) */
        static constexpr uint8_t entrySizeMax = 255u;

        /** Maximal size of entry data in one frame (number and chunks remaining precede it) */
        static constexpr crsf_MsgLength_t chunkSizeMax = CRSF_EXT_PAYLOAD_MAX_LEN - 2u;

        /** Maximal count of configured parameters (0xFF terminates folder children list) */
        static constexpr uint8_t paramsCountMax = 254u;

        /** Mark of the end of folder children list */
        static constexpr uint8_t folderListEnd = 0xFFu;

        /** Hidden bit of data type */
        static constexpr uint8_t hiddenFlag = 0x80u;

    private:

        void Process_Ping ( const crsf_Frame_t &frame, FrameSender &sender );
        void Process_Read ( const crsf_Frame_t &frame, FrameSender &sender );
        void Process_Write( const crsf_Frame_t &frame, FrameSender &sender );

        void Send_Entry ( crsf_DeviceAddr_t destAddr, crsf_ParamNumber_t paramNumber, uint8_t chunkNumber, FrameSender &sender );
        void Build_Entry( crsf_ParamNumber_t paramNumber );
        void Write_Value( const crsf_ParamConfig_t &config, const uint8_t * const data, crsf_MsgLength_t dataLen );

        uint8_t Get_OptionsCount( const char * const options ) const;

        /** Device information */
        const crsf_DeviceInfo_t &deviceInformation;

        /** Configuration of parameters */
        const crsf_ParamConfig_t * const params;

        /** Count of parameters */
        const uint8_t paramsCnt;

        /** Entry of the last read parameter */
        uint8_t entryBuffer[ entrySizeMax ] = { 0u };

        /** Length of entry in entry buffer */
        uint8_t entryLength = 0u;

        /** Parameter number of entry in entry buffer */
        crsf_ParamNumber_t entryParamNumber = 0u;

        /** Entry buffer contains valid entry */
        bool entryValid = false;

        /** Entry update requested by application */
        bool updatePending = false;

        /** Parameter number of requested entry update */
        crsf_ParamNumber_t updateParamNumber = 0u;

        /** Destination of requested entry update */
        crsf_DeviceAddr_t updateDestAddr = CRSF_ADDR_BROADCAST;
    };
} /* namespace AppLayer */
} /* namespace Crsf */

#endif /* APPLAYER_CRSF_APP_PARAMS_HPP */
