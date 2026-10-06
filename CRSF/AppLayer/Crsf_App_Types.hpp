/**
 * \author Mr.Nobody
 * \file Crsf_App_Types.hpp
 * \ingroup Crsf
 * \brief CRSF module application layer types definition file
 *
 * - \ref Crsf::FrameSender  - interface used by handlers to send frames
 *   (implemented by \ref Crsf::AppLayer::AppCore).
 * - \ref Crsf::FrameHandler - base class of received frames handlers.
 * - Application callbacks and configuration of handlers.
 */

#ifndef APPLAYER_CRSF_APP_TYPES_HPP
#define APPLAYER_CRSF_APP_TYPES_HPP

/* ============================= INCLUDES =================================== */
#include "Crsf_Types.h"                     /* Module public types definition */
#include "stdint.h"                         /* Standard types definition      */
/* ============================= TYPEDEFS =================================== */

/** \brief Enumeration used to signal if handler processes frame type */
typedef enum
{
    CRSF_FRAMETYPE_NOT_SUPPORTED = 0u, /**< Frame type is not processed by handler */
    CRSF_FRAMETYPE_SUPPORTED           /**< Frame type is processed by handler     */
}   crsf_FrameTypeStatus_t;


/** \brief Callback called for every received valid frame (eg. logging) */
typedef void ( crsf_FrameCallback_t ) ( const crsf_Frame_t *frame );

/* -------------------------------- Bridge ---------------------------------- */

/** \brief Port of bridge */
typedef enum
{
    CRSF_BRIDGE_PORT_A = 0u, /**< First port         */
    CRSF_BRIDGE_PORT_B,      /**< Second port        */
    CRSF_BRIDGE_PORT_CNT     /**< Count of ports     */
}   crsf_BridgePort_t;


/** \brief Decision of bridge filter */
typedef enum
{
    CRSF_BRIDGE_DROP = 0u, /**< Frame is not forwarded */
    CRSF_BRIDGE_FORWARD    /**< Frame is forwarded     */
}   crsf_BridgeDecision_t;


/** \brief Bridge filter of application - decides if frame received on source port is forwarded */
typedef crsf_BridgeDecision_t ( crsf_BridgeFilter_t ) ( const crsf_Frame_t *frame, crsf_BridgePort_t sourcePort );


/** \brief Bridge statistics of one direction (frames received on the port) */
typedef struct
{
    crsf_Counter_t ForwardedCnt; /**< Frames forwarded to the other port                    */
    crsf_Counter_t FilteredCnt;  /**< Frames not forwarded (filter, addressed to own device) */
    crsf_Counter_t OverflowCnt;  /**< Frames lost - transmit queue of the other port full   */
}   crsf_BridgeStats_t;

/* ----------------------------- Telemetry RX ------------------------------- */

typedef void ( crsf_GpsCallback_t )              ( const crsf_Gps_t *data );
typedef void ( crsf_GpsTimeCallback_t )          ( const crsf_GpsTime_t *data );
typedef void ( crsf_GpsExtendedCallback_t )      ( const crsf_GpsExtended_t *data );
typedef void ( crsf_VarioCallback_t )            ( const crsf_Vario_t *data );
typedef void ( crsf_BatteryCallback_t )          ( const crsf_Battery_t *data );
typedef void ( crsf_BaroAltitudeCallback_t )     ( const crsf_BaroAltitude_t *data );
typedef void ( crsf_HeartbeatCallback_t )        ( const crsf_Heartbeat_t *data );
typedef void ( crsf_VtxTelemetryCallback_t )     ( const crsf_VtxTelemetry_t *data );
typedef void ( crsf_LinkStatisticsCallback_t )   ( const crsf_LinkStatistics_t *data );
typedef void ( crsf_LinkStatisticsRxCallback_t ) ( const crsf_LinkStatisticsRx_t *data );
typedef void ( crsf_LinkStatisticsTxCallback_t ) ( const crsf_LinkStatisticsTx_t *data );
typedef void ( crsf_AttitudeCallback_t )         ( const crsf_Attitude_t *data );
typedef void ( crsf_MavLinkFcCallback_t )        ( const crsf_MavLinkFc_t *data );
typedef void ( crsf_FlightModeCallback_t )       ( const crsf_FlightMode_t *data );
typedef void ( crsf_EspNowCallback_t )           ( const crsf_EspNow_t *data );
typedef void ( crsf_TimingCorrectionCallback_t ) ( const crsf_TimingCorrection_t *data );
typedef void ( crsf_MavLinkSysStatusCallback_t ) ( const crsf_MavLinkSysStatus_t *data );


/**
 * \brief Callbacks of received telemetry frames (\ref Crsf::AppLayer::TelemetryRx)
 *
 * Frame type with null callback is not processed. Callback is called only for
 * frame with valid payload length.
 */
typedef struct
{
    crsf_GpsCallback_t              *Gps;              /**< 0x02 GPS                     */
    crsf_GpsTimeCallback_t          *GpsTime;          /**< 0x03 GPS time                */
    crsf_GpsExtendedCallback_t      *GpsExtended;      /**< 0x06 GPS extended            */
    crsf_VarioCallback_t            *Vario;            /**< 0x07 Vario                   */
    crsf_BatteryCallback_t          *Battery;          /**< 0x08 Battery                 */
    crsf_BaroAltitudeCallback_t     *BaroAltitude;     /**< 0x09 Barometric altitude     */
    crsf_HeartbeatCallback_t        *Heartbeat;        /**< 0x0B Heartbeat               */
    crsf_VtxTelemetryCallback_t     *VtxTelemetry;     /**< 0x10 VTX telemetry           */
    crsf_LinkStatisticsCallback_t   *LinkStatistics;   /**< 0x14 Link statistics         */
    crsf_LinkStatisticsRxCallback_t *LinkStatisticsRx; /**< 0x1C Link statistics RX      */
    crsf_LinkStatisticsTxCallback_t *LinkStatisticsTx; /**< 0x1D Link statistics TX      */
    crsf_AttitudeCallback_t         *Attitude;         /**< 0x1E Attitude                */
    crsf_MavLinkFcCallback_t        *MavLinkFc;        /**< 0x1F MAVLink FC              */
    crsf_FlightModeCallback_t       *FlightMode;       /**< 0x21 Flight mode             */
    crsf_EspNowCallback_t           *EspNow;           /**< 0x22 ESP_NOW                 */
    crsf_TimingCorrectionCallback_t *TimingCorrection; /**< 0x3A.0x10 Timing correction  */
    crsf_MavLinkSysStatusCallback_t *MavLinkSysStatus; /**< 0xAC MAVLink system status   */
}   crsf_TelemetryCallbacks_t;

/* ------------------------------ RC channels ------------------------------- */

/** \brief Callback called with every received RC channels frame */
typedef void ( crsf_RcChannelsCallback_t ) ( const crsf_RcChannels_t *channels );


/** \brief Callback called when the link state changes (failsafe) */
typedef void ( crsf_LinkStateCallback_t ) ( crsf_LinkState_t linkState );

/* ------------------------------ Parameters -------------------------------- */

/** \brief Parameter number (0 is root folder, configured parameters start with 1) */
typedef uint8_t crsf_ParamNumber_t;


/** \brief Parameter data type (bits 6 - 0 of data type field) */
typedef enum
{
    CRSF_PARAM_UINT8          = 0u,   /**< Deprecated, not supported (use FLOAT) */
    CRSF_PARAM_INT8           = 1u,   /**< Deprecated, not supported (use FLOAT) */
    CRSF_PARAM_UINT16         = 2u,   /**< Deprecated, not supported (use FLOAT) */
    CRSF_PARAM_INT16          = 3u,   /**< Deprecated, not supported (use FLOAT) */
    CRSF_PARAM_UINT32         = 4u,   /**< Deprecated, not supported (use FLOAT) */
    CRSF_PARAM_INT32          = 5u,   /**< Deprecated, not supported (use FLOAT) */
    CRSF_PARAM_FLOAT          = 8u,   /**< Number with decimal point (INT32 + decimal point) */
    CRSF_PARAM_TEXT_SELECTION = 9u,   /**< Selection of one text option          */
    CRSF_PARAM_STRING         = 10u,  /**< Editable text                         */
    CRSF_PARAM_FOLDER         = 11u,  /**< Folder of parameters                  */
    CRSF_PARAM_INFO           = 12u,  /**< Read only text                        */
    CRSF_PARAM_COMMAND        = 13u,  /**< Command executed by the device        */
    CRSF_PARAM_OUT_OF_RANGE   = 127u  /**< Requested parameter does not exist    */
}   crsf_ParamType_t;


/** \brief Status / action of COMMAND parameter */
typedef enum
{
    CRSF_CMD_READY               = 0u, /**< Feedback - command is ready                */
    CRSF_CMD_START               = 1u, /**< Input    - start command                   */
    CRSF_CMD_PROGRESS            = 2u, /**< Feedback - command is in progress          */
    CRSF_CMD_CONFIRMATION_NEEDED = 3u, /**< Feedback - host shall confirm or cancel    */
    CRSF_CMD_CONFIRM             = 4u, /**< Input    - confirmed by user               */
    CRSF_CMD_CANCEL              = 5u, /**< Input    - cancelled by user               */
    CRSF_CMD_POLL                = 6u  /**< Input    - request of actual status        */
}   crsf_CmdStatus_t;


/** \brief Visibility of parameter (hidden bit of data type) */
typedef enum
{
    CRSF_PARAM_VISIBLE = 0u, /**< Parameter is shown by host  */
    CRSF_PARAM_HIDDEN        /**< Parameter is hidden by host */
}   crsf_ParamVisibility_t;


/**
 * \brief Value of parameter exchanged with application
 *
 * | Type           | Number                     | Text                     |
 * |----------------|----------------------------|--------------------------|
 * | FLOAT          | value (with decimal point) | -                        |
 * | TEXT_SELECTION | index of selected option   | -                        |
 * | STRING         | -                          | null-terminated value    |
 * | INFO           | -                          | null-terminated info     |
 * | COMMAND        | status / action            | info (getter only)       |
 */
typedef struct
{
    int32_t     Number; /**< Numeric value                                         */
    const char *Text;   /**< Text value (getter provides pointer to static string) */
}   crsf_ParamValue_t;


/** \brief Parameter getter. Return \ref CRSF_REQUEST_ERROR if the value is not available. */
typedef crsf_RequestState_t ( crsf_ParamGetter_t ) ( crsf_ParamValue_t *value );


/** \brief Parameter setter. Return \ref CRSF_REQUEST_ERROR if the value is not accepted. */
typedef crsf_RequestState_t ( crsf_ParamSetter_t ) ( const crsf_ParamValue_t *value );


/** \brief Returns actual visibility of parameter (parameters can be shown / hidden dynamically) */
typedef crsf_ParamVisibility_t ( crsf_ParamVisibilityGetter_t ) ( void );


/**
 * \brief Configuration of one parameter
 *
 * Configuration array is indexed by parameter number - 1 (array item 0 is
 * parameter 1). Parameter 0 is root folder created by the module. Use
 * CRSF_PARAM_xxx macros for configuration of items.
 *
 * | Field        | FLOAT       | TEXT_SELECTION   | STRING     | FOLDER | INFO   | COMMAND           |
 * |--------------|-------------|------------------|------------|--------|--------|-------------------|
 * | Options      | -           | "Opt1;Opt2;Opt3" | -          | -      | -      | -                 |
 * | Unit         | unit        | unit             | -          | -      | -      | -                 |
 * | Min          | minimum     | -                | -          | -      | -      | -                 |
 * | Max          | maximum     | -                | max length | -      | -      | timeout [100 ms]  |
 * | Default      | default     | default index    | -          | -      | -      | -                 |
 * | Step         | step        | -                | -          | -      | -      | -                 |
 * | DecimalPoint | digits      | -                | -          | -      | -      | -                 |
 * | Getter       | required    | required         | required   | -      | required | required        |
 * | Setter       | optional    | optional         | optional   | -      | -      | required          |
 *
 * Parameter without setter is read only (write request is answered by
 * actual value).
 */
typedef struct
{
    crsf_ParamNumber_t            ParentFolder;      /**< Parameter number of parent folder, 0 = root */
    crsf_ParamType_t              Type;              /**< Parameter data type                         */
    const char                   *Name;              /**< Null-terminated parameter name              */
    const char                   *Options;           /**< Text selection options (semicolon delimited)*/
    const char                   *Unit;              /**< Null-terminated unit                        */
    int32_t                       Min;               /**< Minimal value                               */
    int32_t                       Max;               /**< Maximal value / length / timeout            */
    int32_t                       Default;           /**< Default value                               */
    int32_t                       Step;              /**< Recommended step                            */
    uint8_t                       DecimalPoint;      /**< Count of digits behind decimal point        */
    crsf_ParamGetter_t           *Getter;            /**< Value getter                                */
    crsf_ParamSetter_t           *Setter;            /**< Value setter (null = read only)             */
    crsf_ParamVisibilityGetter_t *VisibilityGetter;  /**< Visibility getter (null = always visible)   */
}   crsf_ParamConfig_t;


/** \brief Device information (0x29 Parameter device information) */
typedef struct
{
    const char *Name;             /**< Null-terminated device name (also name of root folder) */
    uint32_t    SerialNumber;     /**< Serial number                                          */
    uint32_t    HardwareId;       /**< Hardware ID                                            */
    uint32_t    FirmwareId;       /**< Firmware ID                                            */
    uint8_t     ParameterVersion; /**< Parameter version number (change when list changes)    */
}   crsf_DeviceInfo_t;

/* ---------------------------- Direct commands ----------------------------- */

/** \brief Result of command execution reported in Command ACK */
typedef enum
{
    CRSF_CMD_ACTION_NONE  = 0u, /**< Not a correct command or no function on that command */
    CRSF_CMD_ACTION_TAKEN = 1u  /**< Target already took action                           */
}   crsf_CmdAction_t;


/** \brief Received direct command passed to application */
typedef struct
{
    crsf_DeviceAddr_t OrigAddr;     /**< Sender of the command                         */
    crsf_DeviceAddr_t DestAddr;     /**< Destination (own address or broadcast)        */
    uint8_t           CommandId;    /**< Command ID (eg. 0x01 FC commands)             */
    uint8_t           SubCommandId; /**< Sub-command ID (eg. 0x01 Force disarm)        */
    const uint8_t    *Data;         /**< Command data behind sub-command (without CRC) */
    crsf_MsgLength_t  DataLen;      /**< Command data length                           */
}   crsf_Command_t;


/** \brief Command handler of application */
typedef crsf_CmdAction_t ( crsf_CommandCallback_t ) ( const crsf_Command_t *command );


/** \brief Configuration of one supported direct command */
typedef struct
{
    uint8_t                 CommandId;    /**< Command ID                     */
    uint8_t                 SubCommandId; /**< Sub-command ID                 */
    crsf_CommandCallback_t *Callback;     /**< Command handler of application */
}   crsf_CommandConfig_t;

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* -------------------------- Direct command IDs ---------------------------- */

#define CRSF_CMDID_FC                                        ( 0x01u ) /**< FC commands          */
#define CRSF_CMDID_BLUETOOTH                                 ( 0x03u ) /**< Bluetooth commands   */
#define CRSF_CMDID_OSD                                       ( 0x05u ) /**< OSD commands         */
#define CRSF_CMDID_VTX                                       ( 0x08u ) /**< VTX commands         */
#define CRSF_CMDID_LED                                       ( 0x09u ) /**< LED commands         */
#define CRSF_CMDID_GENERAL                                   ( 0x0Au ) /**< General commands     */
#define CRSF_CMDID_CROSSFIRE                                 ( 0x10u ) /**< Crossfire commands   */
#define CRSF_CMDID_FLOW_CONTROL                              ( 0x20u ) /**< Flow control frame   */
#define CRSF_CMDID_SCREEN                                    ( 0x22u ) /**< Screen commands      */
#define CRSF_CMDID_ACK                                       ( 0xFFu ) /**< Command ACK          */

#define CRSF_SUBCMD_FC_FORCE_DISARM                          ( 0x01u ) /**< FC: Force disarm                    */
#define CRSF_SUBCMD_FC_SCALE_CHANNEL                         ( 0x02u ) /**< FC: Scale channel                   */
#define CRSF_SUBCMD_BT_RESET                                 ( 0x01u ) /**< Bluetooth: Reset                    */
#define CRSF_SUBCMD_BT_ENABLE                                ( 0x02u ) /**< Bluetooth: Enable (uint8)           */
#define CRSF_SUBCMD_BT_ECHO                                  ( 0x64u ) /**< Bluetooth: Echo                     */
#define CRSF_SUBCMD_OSD_SEND_BUTTONS                         ( 0x01u ) /**< OSD: Send buttons (uint8 bitwise)   */
#define CRSF_SUBCMD_VTX_SET_FREQUENCY                        ( 0x02u ) /**< VTX: Set frequency (uint16 MHz)     */
#define CRSF_SUBCMD_VTX_PITMODE_ON_POWER_UP                  ( 0x04u ) /**< VTX: Enable pit mode on power up    */
#define CRSF_SUBCMD_VTX_POWER_UP_FROM_PITMODE                ( 0x05u ) /**< VTX: Power up from pit mode         */
#define CRSF_SUBCMD_VTX_SET_DYNAMIC_POWER                    ( 0x06u ) /**< VTX: Set dynamic power (uint8 dBm)  */
#define CRSF_SUBCMD_VTX_SET_POWER                            ( 0x08u ) /**< VTX: Set power (uint8 dBm)          */
#define CRSF_SUBCMD_LED_SET_DEFAULT                          ( 0x01u ) /**< LED: Set to default                 */
#define CRSF_SUBCMD_LED_OVERRIDE_COLOR                       ( 0x02u ) /**< LED: Override color                 */
#define CRSF_SUBCMD_LED_OVERRIDE_PULSE                       ( 0x03u ) /**< LED: Override pulse                 */
#define CRSF_SUBCMD_LED_OVERRIDE_BLINK                       ( 0x04u ) /**< LED: Override blink                 */
#define CRSF_SUBCMD_LED_OVERRIDE_SHIFT                       ( 0x05u ) /**< LED: Override shift                 */
#define CRSF_SUBCMD_GENERAL_SPEED_PROPOSAL                   ( 0x70u ) /**< General: Protocol speed proposal    */
#define CRSF_SUBCMD_GENERAL_SPEED_RESPONSE                   ( 0x71u ) /**< General: Protocol speed response    */
#define CRSF_SUBCMD_CRSF_BIND                                ( 0x01u ) /**< Crossfire: Set receiver in bind mode*/
#define CRSF_SUBCMD_CRSF_CANCEL_BIND                         ( 0x02u ) /**< Crossfire: Cancel bind mode         */
#define CRSF_SUBCMD_CRSF_SET_BIND_ID                         ( 0x03u ) /**< Crossfire: Set bind ID              */
#define CRSF_SUBCMD_CRSF_MODEL_SELECTION                     ( 0x05u ) /**< Crossfire: Model selection          */
#define CRSF_SUBCMD_CRSF_CURRENT_MODEL_QUERY                 ( 0x06u ) /**< Crossfire: Current model query      */
#define CRSF_SUBCMD_CRSF_CURRENT_MODEL_REPLY                 ( 0x07u ) /**< Crossfire: Current model reply      */
#define CRSF_SUBCMD_FLOW_SUBSCRIBE                           ( 0x01u ) /**< Flow control: Subscribe             */
#define CRSF_SUBCMD_FLOW_UNSUBSCRIBE                         ( 0x02u ) /**< Flow control: Unsubscribe           */
#define CRSF_SUBCMD_SCREEN_POPUP_START                       ( 0x01u ) /**< Screen: PopUp message start         */
#define CRSF_SUBCMD_SCREEN_SELECTION_RETURN                  ( 0x02u ) /**< Screen: Selection return value      */

/* ------------------------ Parameter configuration ------------------------- */

/** FLOAT parameter (value = Number / 10^decimalPoint) */
#define CRSF_PARAM_FLOAT_ITEM( parent, name, getter, setter, min, max, def, decimalPoint, step, unit ) \
    { (parent), CRSF_PARAM_FLOAT, (name), nullptr, (unit), (min), (max), (def), (step), (decimalPoint), (getter), (setter), nullptr }

/** TEXT_SELECTION parameter (options "Opt1;Opt2;Opt3", minimum 0 and maximum are derived from options) */
#define CRSF_PARAM_TEXT_SELECTION_ITEM( parent, name, options, getter, setter, def, unit ) \
    { (parent), CRSF_PARAM_TEXT_SELECTION, (name), (options), (unit), 0, 0, (def), 0, 0u, (getter), (setter), nullptr }

/** STRING parameter */
#define CRSF_PARAM_STRING_ITEM( parent, name, getter, setter, maxLength ) \
    { (parent), CRSF_PARAM_STRING, (name), nullptr, nullptr, 0, (maxLength), 0, 0, 0u, (getter), (setter), nullptr }

/** FOLDER parameter (children are all parameters with this folder as parent) */
#define CRSF_PARAM_FOLDER_ITEM( parent, name ) \
    { (parent), CRSF_PARAM_FOLDER, (name), nullptr, nullptr, 0, 0, 0, 0, 0u, nullptr, nullptr, nullptr }

/** INFO parameter (read only text) */
#define CRSF_PARAM_INFO_ITEM( parent, name, getter ) \
    { (parent), CRSF_PARAM_INFO, (name), nullptr, nullptr, 0, 0, 0, 0, 0u, (getter), nullptr, nullptr }

/** COMMAND parameter (timeout in 100 ms) */
#define CRSF_PARAM_COMMAND_ITEM( parent, name, getter, setter, timeout ) \
    { (parent), CRSF_PARAM_COMMAND, (name), nullptr, nullptr, 0, (timeout), 0, 0, 0u, (getter), (setter), nullptr }

/* ========================= EXPORTED MACROS ================================ */

/* ========================= EXPORTED VARIABLES ============================= */

/* ======================== EXPORTED FUNCTIONS ============================== */

namespace Crsf
{
    /**
     * \brief Interface used by handlers to send frames
     *
     * Implemented by \ref Crsf::AppLayer::AppCore over transport layer.
     */
    class FrameSender
    {
    public:
        virtual ~FrameSender() = default;

        /** \brief Sends frame with short header */
        virtual crsf_RequestState_t Send_Frame   ( crsf_FrameType_t frameType,
                                                   const uint8_t * const payload, crsf_MsgLength_t payloadLen ) = 0;

        /** \brief Sends frame with extended header, origin is own device address */
        virtual crsf_RequestState_t Send_ExtFrame( crsf_FrameType_t frameType, crsf_DeviceAddr_t destAddr,
                                                   const uint8_t * const payload, crsf_MsgLength_t payloadLen ) = 0;

        /** \brief Returns own device address */
        virtual crsf_DeviceAddr_t   Get_DeviceAddr( void ) const = 0;
    };


    /**
     * \brief Observer of all received valid frames
     *
     * Called by \ref Crsf::AppLayer::AppCore for every received frame before
     * it is dispatched to handlers, also for frames addressed to other devices
     * (eg. \ref Crsf::AppLayer::Bridge).
     */
    class FrameObserver
    {
    public:
        virtual ~FrameObserver() = default;

        /** \brief Processes received frame (pointers valid only during the call) */
        virtual void Observe_Frame( const crsf_Frame_t &frame ) = 0;
    };


    /**
     * \brief Base class of received frames handlers
     *
     * Application core passes every received frame (extended frames only if
     * addressed to own device or broadcast) to all handlers supporting its
     * type.
     */
    class FrameHandler
    {
    public:
        virtual ~FrameHandler() = default;

        /** \brief Returns \ref CRSF_FRAMETYPE_SUPPORTED if handler processes frame type */
        virtual crsf_FrameTypeStatus_t Get_FrameTypeStatus( crsf_FrameType_t frameType ) const = 0;

        /**
         * \brief Processes received frame
         *
         * \param frame  [in]: Received frame
         * \param sender [in]: Interface used for response
         */
        virtual void Process_Frame( const crsf_Frame_t &frame, FrameSender &sender ) = 0;

        /**
         * \brief Cyclic task of handler (called by application core)
         *
         * \param taskTime [in]: Period of task calling in ms [0.001s]
         * \param sender   [in]: Interface used for sending
         */
        virtual void Task( crsf_Time_t taskTime, FrameSender &sender )
        {
            (void)taskTime;
            (void)sender;
        }
    };
} /* namespace Crsf */

#endif /* APPLAYER_CRSF_APP_TYPES_HPP */
