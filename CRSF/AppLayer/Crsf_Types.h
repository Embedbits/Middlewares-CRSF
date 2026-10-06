/**
 * \author Mr.Nobody
 * \file Crsf_Types.h
 * \ingroup Crsf
 * \brief CRSF (Crossfire) module common (C compatible) types definition.
 *
 * Types shared by all layers of the module (physical, transport and
 * application layer) - device addresses, frame types, frame description and
 * decoded payloads of all broadcast frames. The file is C compatible, so it
 * can be included also by C modules (eg. HAL layer used by physical layer
 * implementation or application data callbacks).
 *
 * Payload structures contain raw protocol values (units are described at each
 * field), conversions are done by the application.
 */

#ifndef CRSF_TYPES_H
#define CRSF_TYPES_H

/* ============================= INCLUDES =================================== */
#include "stdint.h"                             /* Standard types definition  */
/* ========================= SYMBOLIC CONSTANTS ============================= */

/** Null pointer definition */
#define CRSF_NULL_PTR                                        ((void*)0u)

/* ------------------------------ Frame layout ------------------------------ */

/** Serial sync byte (first byte of frame) */
#define CRSF_SYNC_BYTE                                       ( 0xC8u )

/** Maximal length of CRSF frame in bytes - sync byte, length, type, payload and CRC */
#define CRSF_FRAME_MAX_LEN                                   ( 64u )

/** Count of bytes in front of the frame length field - sync byte and length */
#define CRSF_FRAME_HEADER_LEN                                ( 2u )

/** Count of bytes of frame type field */
#define CRSF_FRAME_TYPE_LEN                                  ( 1u )

/** Count of bytes of CRC appended to every frame */
#define CRSF_FRAME_CRC_LEN                                   ( 1u )

/** Count of bytes of extended header - destination and origin address */
#define CRSF_FRAME_EXT_HEADER_LEN                            ( 2u )

/** Minimal valid value of frame length field (type and CRC) */
#define CRSF_FRAME_LENGTH_MIN                                ( CRSF_FRAME_TYPE_LEN + CRSF_FRAME_CRC_LEN )

/** Maximal valid value of frame length field */
#define CRSF_FRAME_LENGTH_MAX                                ( CRSF_FRAME_MAX_LEN - CRSF_FRAME_HEADER_LEN )

/** Maximal payload length of broadcast (short header) frame */
#define CRSF_PAYLOAD_MAX_LEN                                 ( CRSF_FRAME_LENGTH_MAX - CRSF_FRAME_TYPE_LEN - CRSF_FRAME_CRC_LEN )

/** Maximal payload length of extended header frame */
#define CRSF_EXT_PAYLOAD_MAX_LEN                             ( CRSF_PAYLOAD_MAX_LEN - CRSF_FRAME_EXT_HEADER_LEN )

/* ---------------------------- Device addresses ---------------------------- */

#define CRSF_ADDR_BROADCAST                                  ( 0x00u ) /**< Broadcast address                       */
#define CRSF_ADDR_CLOUD                                      ( 0x0Eu ) /**< Cloud                                   */
#define CRSF_ADDR_USB                                        ( 0x10u ) /**< USB device                              */
#define CRSF_ADDR_BLUETOOTH                                  ( 0x12u ) /**< Bluetooth module / WiFi                 */
#define CRSF_ADDR_WIFI_RECEIVER                              ( 0x13u ) /**< Wi-Fi receiver (mobile game/simulator)  */
#define CRSF_ADDR_VIDEO_RECEIVER                             ( 0x14u ) /**< Video receiver                          */
#define CRSF_ADDR_DYNAMIC_FIRST                              ( 0x20u ) /**< Dynamic address space for NAT - first   */
#define CRSF_ADDR_DYNAMIC_LAST                               ( 0x7Fu ) /**< Dynamic address space for NAT - last    */
#define CRSF_ADDR_OSD                                        ( 0x80u ) /**< OSD / TBS CORE PNP PRO                  */
#define CRSF_ADDR_ESC_1                                      ( 0x90u ) /**< ESC 1 (ESC 2 - 8 follow: 0x91 - 0x97)   */
#define CRSF_ADDR_ESC_8                                      ( 0x97u ) /**< ESC 8                                   */
#define CRSF_ADDR_CURRENT_SENSOR                             ( 0xC0u ) /**< Voltage / current sensor                */
#define CRSF_ADDR_GPS                                        ( 0xC2u ) /**< GPS                                     */
#define CRSF_ADDR_BLACKBOX                                   ( 0xC4u ) /**< TBS Blackbox                            */
#define CRSF_ADDR_FLIGHT_CONTROLLER                          ( 0xC8u ) /**< Flight controller                       */
#define CRSF_ADDR_RACE_TAG                                   ( 0xCCu ) /**< Race tag                                */
#define CRSF_ADDR_VTX                                        ( 0xCEu ) /**< VTX                                     */
#define CRSF_ADDR_REMOTE_CONTROL                             ( 0xEAu ) /**< Remote control (handset)                */
#define CRSF_ADDR_RC_RECEIVER                                ( 0xECu ) /**< R/C receiver / Crossfire Rx             */
#define CRSF_ADDR_RC_TRANSMITTER                             ( 0xEEu ) /**< R/C transmitter module / Crossfire Tx   */

/* ------------------------------ Frame types ------------------------------- */

/* Broadcast frames (short header) */
#define CRSF_FRAMETYPE_GPS                                   ( 0x02u ) /**< GPS                                     */
#define CRSF_FRAMETYPE_GPS_TIME                              ( 0x03u ) /**< GPS time                                */
#define CRSF_FRAMETYPE_GPS_EXTENDED                          ( 0x06u ) /**< GPS extended                            */
#define CRSF_FRAMETYPE_VARIO                                 ( 0x07u ) /**< Vario sensor                            */
#define CRSF_FRAMETYPE_BATTERY                               ( 0x08u ) /**< Battery sensor                          */
#define CRSF_FRAMETYPE_BARO_ALTITUDE                         ( 0x09u ) /**< Barometric altitude and vertical speed  */
#define CRSF_FRAMETYPE_HEARTBEAT                             ( 0x0Bu ) /**< Heartbeat                               */
#define CRSF_FRAMETYPE_VTX_TELEMETRY                         ( 0x10u ) /**< VTX telemetry                           */
#define CRSF_FRAMETYPE_LINK_STATISTICS                       ( 0x14u ) /**< Link statistics                         */
#define CRSF_FRAMETYPE_RC_CHANNELS_PACKED                    ( 0x16u ) /**< RC channels packed payload              */
#define CRSF_FRAMETYPE_SUBSET_RC_CHANNELS                    ( 0x17u ) /**< Subset RC channels packed (discouraged) */
#define CRSF_FRAMETYPE_LINK_STATISTICS_RX                    ( 0x1Cu ) /**< Link statistics RX                      */
#define CRSF_FRAMETYPE_LINK_STATISTICS_TX                    ( 0x1Du ) /**< Link statistics TX                      */
#define CRSF_FRAMETYPE_ATTITUDE                              ( 0x1Eu ) /**< Attitude                                */
#define CRSF_FRAMETYPE_MAVLINK_FC                            ( 0x1Fu ) /**< MAVLink FC                              */
#define CRSF_FRAMETYPE_FLIGHT_MODE                           ( 0x21u ) /**< Flight mode                             */
#define CRSF_FRAMETYPE_ESP_NOW                               ( 0x22u ) /**< ESP_NOW messages                        */

/** Highest frame type with short header (types above use extended header, see \ref Crsf_Get_HeaderType) */
#define CRSF_FRAMETYPE_SHORT_HEADER_LAST                     ( 0x27u )

/* Extended frames (destination and origin address) */
#define CRSF_FRAMETYPE_PARAM_PING                            ( 0x28u ) /**< Parameter ping devices                  */
#define CRSF_FRAMETYPE_PARAM_DEVICE_INFO                     ( 0x29u ) /**< Parameter device information            */
#define CRSF_FRAMETYPE_PARAM_ENTRY                           ( 0x2Bu ) /**< Parameter settings (entry)              */
#define CRSF_FRAMETYPE_PARAM_READ                            ( 0x2Cu ) /**< Parameter settings (read)               */
#define CRSF_FRAMETYPE_PARAM_WRITE                           ( 0x2Du ) /**< Parameter value (write)                 */
#define CRSF_FRAMETYPE_COMMAND                               ( 0x32u ) /**< Direct commands                         */
#define CRSF_FRAMETYPE_LOGGING                               ( 0x34u ) /**< Logging (short header)                  */
#define CRSF_FRAMETYPE_REMOTE                                ( 0x3Au ) /**< Remote related frames                   */
#define CRSF_FRAMETYPE_GAME                                  ( 0x3Cu ) /**< Game                                    */
#define CRSF_FRAMETYPE_MSP_REQUEST                           ( 0x7Au ) /**< MSP request                             */
#define CRSF_FRAMETYPE_MSP_RESPONSE                          ( 0x7Bu ) /**< MSP response                            */
#define CRSF_FRAMETYPE_MAVLINK_ENVELOPE                      ( 0xAAu ) /**< MAVLink envelope (short header)         */
#define CRSF_FRAMETYPE_MAVLINK_SYS_STATUS                    ( 0xACu ) /**< MAVLink system status (short header)    */

/** Remote related frame (0x3A) sub-type timing correction ("CRSFshot") */
#define CRSF_REMOTE_SUBTYPE_TIMING_CORRECTION                ( 0x10u )

/* --------------------------- Payload properties --------------------------- */

/** Count of channels in RC channels packed frame */
#define CRSF_RC_CHANNELS_CNT                                 ( 16u )

/** Count of bits of one channel in RC channels packed frame */
#define CRSF_RC_CHANNEL_BITS                                 ( 11u )

/** Maximal raw value of RC channel (11 bits) */
#define CRSF_RC_CHANNEL_VALUE_MAX                            ( 0x07FFu )

/** Raw value of RC channel center (1500 us) */
#define CRSF_RC_CHANNEL_VALUE_CENTER                         ( 992u )

/** Size of buffer for null-terminated string carried in one frame (flight mode) */
#define CRSF_STRING_MAX_LEN                                  ( CRSF_PAYLOAD_MAX_LEN )

/** Count of characters of ESP_NOW lap time fields */
#define CRSF_ESP_NOW_LAP_TIME_LEN                            ( 15u )

/** Count of characters of ESP_NOW free text field */
#define CRSF_ESP_NOW_FREE_TEXT_LEN                           ( 20u )

/* ============================= TYPEDEFS =================================== */

/** Enumeration used to signal request processing state */
typedef enum
{
    CRSF_REQUEST_ERROR = 0u, /**< Processing request failed  */
    CRSF_REQUEST_OK          /**< Processing request succeed */
}   crsf_RequestState_t;


/** \brief Type used to signal device address (see CRSF_ADDR_xxx) */
typedef uint8_t crsf_DeviceAddr_t;


/** \brief Type used to signal frame type (see CRSF_FRAMETYPE_xxx) */
typedef uint8_t crsf_FrameType_t;


/** \brief Type used to signal frame / payload length in bytes */
typedef uint8_t crsf_MsgLength_t;


/** \brief Type used to signal time in ms [0.001s] */
typedef uint32_t crsf_Time_t;


/** \brief Type used to signal actual value in counter */
typedef uint32_t crsf_Counter_t;


/** \brief Type used to signal baudrate configuration in bit/s */
typedef uint32_t crsf_Baudrate_t;


/** \brief Enumeration used to signal actual status of frame transmission */
typedef enum
{
    CRSF_TX_ACTIVE = 0u, /**< Frame is being transmitted */
    CRSF_TX_FINISHED     /**< Frame has been transmitted */
}   crsf_TxStatus_t;


/** \brief Enumeration used to signal header type of frame */
typedef enum
{
    CRSF_HEADER_SHORT = 0u, /**< Broadcast frame - sync, length, type, payload, CRC  */
    CRSF_HEADER_EXTENDED    /**< Extended frame - destination and origin follow type */
}   crsf_HeaderType_t;


/** \brief Enumeration used to signal state of the link (RC channels reception) */
typedef enum
{
    CRSF_LINK_LOST = 0u, /**< No RC channels received within failsafe timeout */
    CRSF_LINK_ACTIVE     /**< RC channels are received                        */
}   crsf_LinkState_t;


/**
 * \brief Description of received (valid) frame
 *
 * Pointers refer to transport layer buffer and are valid until the next frame
 * is read from the transport layer.
 */
typedef struct
{
    crsf_DeviceAddr_t SyncByte;   /**< First byte of the frame (sync byte or address)              */
    crsf_FrameType_t  Type;       /**< Frame type                                                  */
    crsf_HeaderType_t HeaderType; /**< Short or extended header                                    */
    crsf_DeviceAddr_t DestAddr;   /**< Destination address (\ref CRSF_ADDR_BROADCAST for short)    */
    crsf_DeviceAddr_t OrigAddr;   /**< Origin address (\ref CRSF_ADDR_BROADCAST for short)         */
    const uint8_t    *Payload;    /**< Payload (behind extended header)                            */
    crsf_MsgLength_t  PayloadLen; /**< Payload length in bytes                                     */
    const uint8_t    *Raw;        /**< Complete frame (sync byte ... CRC), eg. for routing         */
    crsf_MsgLength_t  RawLen;     /**< Complete frame length in bytes                              */
}   crsf_Frame_t;


/** \brief Communication statistics counters of the transport layer */
typedef struct
{
    crsf_Counter_t FramesRcvdCnt;    /**< Count of received valid frames                           */
    crsf_Counter_t CrcErrorCnt;      /**< Count of frames with invalid CRC                         */
    crsf_Counter_t LengthErrorCnt;   /**< Count of invalid frame length fields                     */
    crsf_Counter_t DiscardedByteCnt; /**< Count of bytes discarded while searching for sync byte   */
    crsf_Counter_t FramesSentCnt;    /**< Count of transmitted frames                              */
    crsf_Counter_t TxOverflowCnt;    /**< Count of frames rejected because transmit queue was full */
}   crsf_StatsCounter_t;

/* ---------------------------- Frame payloads ------------------------------ */

/** \brief 0x02 GPS */
typedef struct
{
    int32_t  Latitude;    /**< Degree / 10 000 000                 */
    int32_t  Longitude;   /**< Degree / 10 000 000                 */
    uint16_t GroundSpeed; /**< km/h / 100 (per specification)      */
    uint16_t Heading;     /**< Degree / 100                        */
    uint16_t Altitude;    /**< Meter, offset 1000 m                */
    uint8_t  Satellites;  /**< Count of satellites in view         */
}   crsf_Gps_t;


/** \brief 0x03 GPS time */
typedef struct
{
    int16_t  Year;        /**< Year        */
    uint8_t  Month;       /**< Month       */
    uint8_t  Day;         /**< Day         */
    uint8_t  Hour;        /**< Hour        */
    uint8_t  Minute;      /**< Minute      */
    uint8_t  Second;      /**< Second      */
    uint16_t Millisecond; /**< Millisecond */
}   crsf_GpsTime_t;


/** \brief 0x06 GPS extended */
typedef struct
{
    uint8_t FixType;      /**< Current GPS fix quality                   */
    int16_t NSpeed;       /**< Northward speed [cm/s] (north = positive) */
    int16_t ESpeed;       /**< Eastward speed [cm/s] (east = positive)   */
    int16_t VSpeed;       /**< Vertical speed [cm/s] (up = positive)     */
    int16_t HSpeedAcc;    /**< Horizontal speed accuracy [cm/s]          */
    int16_t TrackAcc;     /**< Heading accuracy [0.1 degree]             */
    int16_t AltEllipsoid; /**< Height above GPS ellipsoid [m]            */
    int16_t HAcc;         /**< Horizontal accuracy [cm]                  */
    int16_t VAcc;         /**< Vertical accuracy [cm]                    */
    uint8_t Reserved;     /**< Reserved                                  */
    uint8_t HDop;         /**< Horizontal dilution of precision [0.1]    */
    uint8_t VDop;         /**< Vertical dilution of precision [0.1]      */
}   crsf_GpsExtended_t;


/** \brief 0x07 Vario sensor */
typedef struct
{
    int16_t VSpeed;       /**< Vertical speed [cm/s] */
}   crsf_Vario_t;


/**
 * \brief 0x08 Battery sensor
 *
 * Specification defines voltage LSB 10 uV and current LSB 10 uA, common
 * implementations (Betaflight, EdgeTX) use 0.1 V and 0.1 A.
 */
typedef struct
{
    int16_t  Voltage;      /**< Voltage                                */
    int16_t  Current;      /**< Current                                */
    uint32_t CapacityUsed; /**< Capacity used [mAh], 24 bits on bus    */
    uint8_t  Remaining;    /**< Battery remaining [%]                  */
}   crsf_Battery_t;


/** \brief 0x09 Barometric altitude and vertical speed (packed, see Crsf::Codec) */
typedef struct
{
    uint16_t AltitudePacked;      /**< Altitude above start point (packed)     */
    int8_t   VerticalSpeedPacked; /**< Vertical speed (logarithmic, packed)    */
}   crsf_BaroAltitude_t;


/** \brief 0x0B Heartbeat */
typedef struct
{
    int16_t OriginAddress; /**< Origin device address */
}   crsf_Heartbeat_t;


/** \brief 0x10 VTX telemetry */
typedef struct
{
    uint8_t  OriginAddress;  /**< Origin device address                         */
    uint8_t  PowerDbm;       /**< VTX power [dBm]                               */
    uint16_t FrequencyMhz;   /**< VTX frequency [MHz]                           */
    uint8_t  PitMode;        /**< 0 = Off, 1 = On (bit 0)                       */
    uint8_t  PitModeControl; /**< 0 = Off, 1 = On, 2 = Switch, 3 = Failsafe     */
    uint8_t  PitModeSwitch;  /**< 0 = Ch5, 1 = Ch5 Inv, ... , 15 = Ch12 Inv     */
}   crsf_VtxTelemetry_t;


/** \brief 0x14 Link statistics (uplink = ground to UAV) */
typedef struct
{
    uint8_t UpRssiAnt1;      /**< Uplink RSSI antenna 1 [dBm * -1]          */
    uint8_t UpRssiAnt2;      /**< Uplink RSSI antenna 2 [dBm * -1]          */
    uint8_t UpLinkQuality;   /**< Uplink package success rate [%]           */
    int8_t  UpSnr;           /**< Uplink SNR [dB]                           */
    uint8_t ActiveAntenna;   /**< Number of currently best antenna          */
    uint8_t RfProfile;       /**< RF profile (0 = 4 fps, 1 = 50, 2 = 150)   */
    uint8_t UpRfPower;       /**< Uplink RF power (enumeration)             */
    uint8_t DownRssi;        /**< Downlink RSSI [dBm * -1]                  */
    uint8_t DownLinkQuality; /**< Downlink package success rate [%]         */
    int8_t  DownSnr;         /**< Downlink SNR [dB]                         */
}   crsf_LinkStatistics_t;


/** \brief 0x16 RC channels packed (raw 11 bit values, center 992 = 1500 us) */
typedef struct
{
    uint16_t Channels[ CRSF_RC_CHANNELS_CNT ]; /**< Raw channel values [0 - 2047] */
}   crsf_RcChannels_t;


/** \brief 0x1C Link statistics RX */
typedef struct
{
    uint8_t RssiDb;      /**< RSSI [dBm * -1]                 */
    uint8_t RssiPercent; /**< RSSI [%]                        */
    uint8_t LinkQuality; /**< Package success rate [%]        */
    int8_t  Snr;         /**< SNR [dB]                        */
    uint8_t RfPowerDb;   /**< RF power [dBm]                  */
}   crsf_LinkStatisticsRx_t;


/** \brief 0x1D Link statistics TX */
typedef struct
{
    uint8_t RssiDb;      /**< RSSI [dBm * -1]                 */
    uint8_t RssiPercent; /**< RSSI [%]                        */
    uint8_t LinkQuality; /**< Package success rate [%]        */
    int8_t  Snr;         /**< SNR [dB]                        */
    uint8_t RfPowerDb;   /**< RF power [dBm]                  */
    uint8_t Fps;         /**< RF frames per second [fps / 10] */
}   crsf_LinkStatisticsTx_t;


/** \brief 0x1E Attitude (angles in range -180 ... +180 degree) */
typedef struct
{
    int16_t Pitch; /**< Pitch angle [100 urad] */
    int16_t Roll;  /**< Roll angle [100 urad]  */
    int16_t Yaw;   /**< Yaw angle [100 urad]   */
}   crsf_Attitude_t;


/** \brief 0x1F MAVLink FC */
typedef struct
{
    int16_t  Airspeed;      /**< Airspeed                                   */
    uint8_t  BaseMode;      /**< Vehicle mode flags (MAV_MODE_FLAG)         */
    uint32_t CustomMode;    /**< Autopilot specific flags                   */
    uint8_t  AutopilotType; /**< FC type (MAV_AUTOPILOT)                    */
    uint8_t  FirmwareType;  /**< Vehicle type (MAV_TYPE)                    */
}   crsf_MavLinkFc_t;


/** \brief 0x21 Flight mode */
typedef struct
{
    char Name[ CRSF_STRING_MAX_LEN ]; /**< Null-terminated flight mode name */
}   crsf_FlightMode_t;


/** \brief 0x22 ESP_NOW messages (text fields are not null-terminated) */
typedef struct
{
    uint8_t SeatPosition;                            /**< VAL1 - seat position of the pilot */
    uint8_t CurrentLap;                              /**< VAL2 - current pilot's lap        */
    char    LapTime1[ CRSF_ESP_NOW_LAP_TIME_LEN ];   /**< VAL3 - lap time current / split   */
    char    LapTime2[ CRSF_ESP_NOW_LAP_TIME_LEN ];   /**< VAL4 - lap time current / split   */
    char    FreeText[ CRSF_ESP_NOW_FREE_TEXT_LEN ];  /**< Free text at bottom of the screen */
}   crsf_EspNow_t;


/** \brief 0x3A.0x10 Timing correction ("CRSFshot", RC-sync) */
typedef struct
{
    uint32_t UpdateInterval; /**< Update interval [100 ns]                                  */
    int32_t  Offset;         /**< Offset [100 ns], positive = data came too early           */
}   crsf_TimingCorrection_t;


/** \brief 0xAC MAVLink system status sensor */
typedef struct
{
    uint32_t SensorPresent; /**< MAV_SYS_STATUS_SENSOR present bitmap */
    uint32_t SensorEnabled; /**< MAV_SYS_STATUS_SENSOR enabled bitmap */
    uint32_t SensorHealth;  /**< MAV_SYS_STATUS_SENSOR health bitmap  */
}   crsf_MavLinkSysStatus_t;

/* ========================= EXPORTED MACROS ================================ */

/**
 * \brief Checks if frame of given type has extended header
 *
 * Frames with type 0x28 and higher have extended header (destination and
 * origin address), except of explicitly mentioned short header frames
 * (Logging, MAVLink envelope, MAVLink system status).
 *
 * \param frameType [in]: Frame type
 *
 * \return Header type of the frame
 */
static inline crsf_HeaderType_t Crsf_Get_HeaderType( crsf_FrameType_t frameType )
{
    crsf_HeaderType_t headerType = CRSF_HEADER_SHORT;

    if( ( CRSF_FRAMETYPE_SHORT_HEADER_LAST  <  frameType ) &&
        ( CRSF_FRAMETYPE_LOGGING            != frameType ) &&
        ( CRSF_FRAMETYPE_MAVLINK_ENVELOPE   != frameType ) &&
        ( CRSF_FRAMETYPE_MAVLINK_SYS_STATUS != frameType )    )
    {
        headerType = CRSF_HEADER_EXTENDED;
    }

    return ( headerType );
}

/* ========================= EXPORTED VARIABLES ============================= */

#endif /* CRSF_TYPES_H */
