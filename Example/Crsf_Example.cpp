/**
 * \author Mr.Nobody
 * \file Crsf_Example.cpp
 * \ingroup Crsf
 * \brief Example of CRSF integration on flight controller side.
 *
 * Shows composition of the whole stack:
 * 1. Physical layer (USART over Crsf_Hal.h, implemented by application).
 * 2. Transport layer (frame search, CRC, transmit queue).
 * 3. Frame handlers - RC channels with failsafe, link statistics, device
 *    parameters (configurable from the remote control menu) and direct
 *    commands.
 * 4. Application core dispatching received frames to handlers.
 * 5. Periodic telemetry sent by TelemetryTx.
 *
 * All objects are static, no dynamic memory is used.
 */

/* ============================= INCLUDES =================================== */
#include "Crsf_Example.h"                   /* Self include                   */
#include "Crsf_Phy_Usart.hpp"               /* USART physical layer           */
#include "Crsf_TL.hpp"                      /* Transport layer                */
#include "Crsf_App.hpp"                     /* Application layer core         */
#include "Crsf_App_RcChannels.hpp"          /* 0x16 RC channels               */
#include "Crsf_App_Telemetry.hpp"           /* Telemetry RX / TX              */
#include "Crsf_App_Params.hpp"              /* 0x28 - 0x2D parameters         */
#include "Crsf_App_Commands.hpp"            /* 0x32 direct commands           */
/* ========================= SYMBOLIC CONSTANTS ============================= */

/** Period of battery and attitude telemetry in ms */
static constexpr crsf_Time_t crsfExample_FastTelemetryPeriod = 100u;

/** Period of flight mode telemetry in ms */
static constexpr crsf_Time_t crsfExample_SlowTelemetryPeriod = 1000u;

/** Parameter numbers (configuration array index + 1) */
static constexpr crsf_ParamNumber_t crsfExample_ParamTuning    = 1u;
static constexpr crsf_ParamNumber_t crsfExample_ParamCalibrate = 5u;

/** Count of items in array */
#define CRSF_EXAMPLE_COUNT( array )         ( sizeof( array ) / sizeof( ( array )[ 0 ] ) )

/* ========================== LOCAL VARIABLES =============================== */

/* --------------------------- Application data ----------------------------- */

static int32_t           crsfExample_RateGain   = 100;    /* 1.00 */
static int32_t           crsfExample_FlightMode = 0;
static crsf_CmdStatus_t  crsfExample_CalibStatus = CRSF_CMD_READY;
static crsf_Battery_t    crsfExample_Battery    = { 0, 0, 0u, 0u };
static crsf_LinkStatistics_t crsfExample_LinkStats = {};

static const char * const crsfExample_ModeNames[] = { "ACRO", "ANGLE", "HORIZON" };

/* ------------------------- Parameter callbacks ---------------------------- */

static crsf_RequestState_t CrsfExample_Get_RateGain( crsf_ParamValue_t *value )
{
    value->Number = crsfExample_RateGain;
    return ( CRSF_REQUEST_OK );
}

static crsf_RequestState_t CrsfExample_Set_RateGain( const crsf_ParamValue_t *value )
{
    crsfExample_RateGain = value->Number;
    return ( CRSF_REQUEST_OK );
}

static crsf_RequestState_t CrsfExample_Get_FlightMode( crsf_ParamValue_t *value )
{
    value->Number = crsfExample_FlightMode;
    return ( CRSF_REQUEST_OK );
}

static crsf_RequestState_t CrsfExample_Set_FlightMode( const crsf_ParamValue_t *value )
{
    crsfExample_FlightMode = value->Number;
    return ( CRSF_REQUEST_OK );
}

static crsf_RequestState_t CrsfExample_Get_Version( crsf_ParamValue_t *value )
{
    value->Text = "1.0.0";
    return ( CRSF_REQUEST_OK );
}

static crsf_RequestState_t CrsfExample_Get_Calibrate( crsf_ParamValue_t *value )
{
    value->Number = crsfExample_CalibStatus;
    value->Text   = ( CRSF_CMD_PROGRESS == crsfExample_CalibStatus ) ? "Keep still" : "";
    return ( CRSF_REQUEST_OK );
}

static crsf_RequestState_t CrsfExample_Set_Calibrate( const crsf_ParamValue_t *value )
{
    if( CRSF_CMD_START == value->Number )
    {
        /* Calibration is started by application, it sets READY when finished
           and calls crsfExample_Params.Request_EntryUpdate() */
        crsfExample_CalibStatus = CRSF_CMD_PROGRESS;
    }
    else if( CRSF_CMD_CANCEL == value->Number )
    {
        crsfExample_CalibStatus = CRSF_CMD_READY;
    }
    else
    {
        /* Confirmation not used by this command */
    }

    return ( CRSF_REQUEST_OK );
}

/* -------------------------- Command callbacks ----------------------------- */

static crsf_CmdAction_t CrsfExample_ForceDisarm( const crsf_Command_t *command )
{
    (void)command;

    /* Application disarms motors here */
    return ( CRSF_CMD_ACTION_TAKEN );
}

/* ------------------------- Telemetry callbacks ---------------------------- */

static void CrsfExample_LinkStatistics( const crsf_LinkStatistics_t *data )
{
    crsfExample_LinkStats = *data;
}

/* ------------------------------ Configuration ----------------------------- */

static const crsf_DeviceInfo_t crsfExample_DeviceInfo =
{
    "EmBi FC",      /* Name             */
    0x00000001u,    /* SerialNumber     */
    0x00000001u,    /* HardwareId       */
    0x00010000u,    /* FirmwareId       */
    1u              /* ParameterVersion */
};

static const crsf_ParamConfig_t crsfExample_Params[] =
{
    /* 1 */ CRSF_PARAM_FOLDER_ITEM        ( 0u, "Tuning" ),
    /* 2 */ CRSF_PARAM_FLOAT_ITEM         ( crsfExample_ParamTuning, "Rate gain", CrsfExample_Get_RateGain, CrsfExample_Set_RateGain,
                                            50, 300, 100, 2u, 5, "x" ),
    /* 3 */ CRSF_PARAM_TEXT_SELECTION_ITEM( crsfExample_ParamTuning, "Mode", "ACRO;ANGLE;HORIZON",
                                            CrsfExample_Get_FlightMode, CrsfExample_Set_FlightMode, 0, "" ),
    /* 4 */ CRSF_PARAM_INFO_ITEM          ( 0u, "Version", CrsfExample_Get_Version ),
    /* 5 */ CRSF_PARAM_COMMAND_ITEM       ( 0u, "Calibrate", CrsfExample_Get_Calibrate, CrsfExample_Set_Calibrate, 50 ),
};

static const crsf_CommandConfig_t crsfExample_Commands[] =
{
    { CRSF_CMDID_FC, CRSF_SUBCMD_FC_FORCE_DISARM, CrsfExample_ForceDisarm },
};

static crsf_TelemetryCallbacks_t crsfExample_TelemetryCallbacks = {};

/* ----------------------------- Stack objects ------------------------------ */

static Crsf::UsartPhysicalLayer       crsfExample_PhysicalLayer;

static Crsf::TransportLayer           crsfExample_TransportLayer( crsfExample_PhysicalLayer );

static Crsf::AppLayer::RcChannels     crsfExample_RcChannels;
static Crsf::AppLayer::TelemetryRx    crsfExample_TelemetryRx( crsfExample_TelemetryCallbacks );
static Crsf::AppLayer::Params         crsfExample_ParamsHandler( crsfExample_DeviceInfo, crsfExample_Params,
                                                                 CRSF_EXAMPLE_COUNT( crsfExample_Params ) );
static Crsf::AppLayer::Commands       crsfExample_CommandsHandler( crsfExample_Commands,
                                                                   CRSF_EXAMPLE_COUNT( crsfExample_Commands ) );

static Crsf::FrameHandler * const     crsfExample_Handlers[] =
{
    &crsfExample_RcChannels, &crsfExample_TelemetryRx, &crsfExample_ParamsHandler, &crsfExample_CommandsHandler
};

static Crsf::AppLayer::AppCore        crsfExample_AppCore( crsfExample_TransportLayer, crsfExample_Handlers,
                                                           CRSF_EXAMPLE_COUNT( crsfExample_Handlers ) );

static Crsf::AppLayer::TelemetryTx    crsfExample_TelemetryTx( crsfExample_AppCore );

/** Time counters of periodic telemetry */
static crsf_Time_t crsfExample_FastTimer = 0u;
static crsf_Time_t crsfExample_SlowTimer = 0u;

/* ======================== EXPORTED FUNCTIONS ============================== */

void CrsfExample_Init( void )
{
    crsfExample_TelemetryCallbacks.LinkStatistics = CrsfExample_LinkStatistics;

    crsfExample_AppCore.Set_DeviceAddr( CRSF_ADDR_FLIGHT_CONTROLLER );
    crsfExample_PhysicalLayer.Set_Baudrate( Crsf::PhysicalLayer::baudrateFullDuplex );
}


void CrsfExample_Task( crsf_Time_t taskTimeMs )
{
    crsfExample_AppCore.Task( taskTimeMs );

    crsfExample_FastTimer += taskTimeMs;
    crsfExample_SlowTimer += taskTimeMs;

    if( crsfExample_FastTelemetryPeriod <= crsfExample_FastTimer )
    {
        crsfExample_FastTimer = 0u;

        (void)crsfExample_TelemetryTx.Send_Battery( crsfExample_Battery );
    }
    else
    {
        /* Not yet */
    }

    if( crsfExample_SlowTelemetryPeriod <= crsfExample_SlowTimer )
    {
        crsfExample_SlowTimer = 0u;

        (void)crsfExample_TelemetryTx.Send_FlightMode( crsfExample_ModeNames[ crsfExample_FlightMode ] );

        /* Report end of calibration started from remote control menu */
        if( CRSF_CMD_PROGRESS == crsfExample_CalibStatus )
        {
            crsfExample_CalibStatus = CRSF_CMD_READY;
            (void)crsfExample_ParamsHandler.Request_EntryUpdate( crsfExample_ParamCalibrate, CRSF_ADDR_REMOTE_CONTROL );
        }
        else
        {
            /* Calibration not running */
        }
    }
    else
    {
        /* Not yet */
    }
}


crsf_LinkState_t CrsfExample_Get_LinkState( void )
{
    return ( crsfExample_RcChannels.Get_LinkState() );
}


int32_t CrsfExample_Get_ChannelUs( uint8_t channelIndex )
{
    return ( crsfExample_RcChannels.Get_ChannelUs( channelIndex ) );
}


void CrsfExample_Set_Battery( int16_t voltageDv, int16_t currentDa, uint32_t capacityMah, uint8_t remainingPct )
{
    crsfExample_Battery.Voltage      = voltageDv;
    crsfExample_Battery.Current      = currentDa;
    crsfExample_Battery.CapacityUsed = capacityMah;
    crsfExample_Battery.Remaining    = remainingPct;
}
