/**
 * \author Mr.Nobody
 * \file Crsf_App_Params.cpp
 * \ingroup Crsf
 * \brief CRSF device parameters - 0x28 ping, 0x29 device information,
 *        0x2B parameter entry, 0x2C parameter read, 0x2D parameter write.
 *
 * Parameter entry (all chunks together):
 *
 * | Type           | Entry                                                                     |
 * |----------------|---------------------------------------------------------------------------|
 * | all            | parent folder, data type (bit 7 = hidden), name\0, type specific part:    |
 * | FLOAT          | value, min, max, default (INT32), decimal point (UINT8), step (INT32), unit\0 |
 * | TEXT_SELECTION | options\0, value, min, max, default (UINT8), unit\0                       |
 * | STRING         | value\0, maximal length (UINT8)                                           |
 * | FOLDER         | children parameter numbers, 0xFF                                          |
 * | INFO           | info\0                                                                    |
 * | COMMAND        | status (UINT8), timeout (UINT8, 100 ms), info\0                           |
 * | OUT_OF_RANGE   | -                                                                         |
 */

/* ============================= INCLUDES =================================== */
#include "Crsf_App_Params.hpp"              /* Self include                   */
#include "Crsf_App_Codec.hpp"               /* Payload writer / reader        */
/* ============================= TYPEDEFS =================================== */

/* ======================= FORWARD DECLARATIONS ============================= */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/** Length of 0x2C Parameter read payload (parameter number and chunk number) */
static constexpr crsf_MsgLength_t crsf_ParamReadLen = 2u;

/** Minimal length of 0x2D Parameter write payload (parameter number) */
static constexpr crsf_MsgLength_t crsf_ParamWriteMinLen = 1u;

/** Length of entry frame header (parameter number and chunks remaining) */
static constexpr crsf_MsgLength_t crsf_EntryHeaderLen = 2u;

/** Root folder parameter number */
static constexpr crsf_ParamNumber_t crsf_RootFolder = 0u;

/** Separator of text selection options */
static constexpr char crsf_OptionsSeparator = ';';

/** String terminator */
static constexpr char crsf_StringEnd = '\0';

/** Maximal value of UINT8 field */
static constexpr int32_t crsf_Uint8Max = 0xFF;

/* ============================== MACROS ==================================== */

/* ========================= EXPORTED VARIABLES ============================= */

/* ========================== LOCAL VARIABLES =============================== */

/* ======================== EXPORTED FUNCTIONS ============================== */

/**
 * \brief Constructor of parameters handler
 *
 * \param deviceInfo   [in]: Device information (object shall exist during the
 *                           whole life of handler)
 * \param paramsConfig [in]: Configuration of parameters, item 0 is parameter 1
 * \param paramsCount  [in]: Count of parameters (maximally \ref paramsCountMax)
 */
Crsf::AppLayer::Params::Params( const crsf_DeviceInfo_t &deviceInfo,
                                const crsf_ParamConfig_t paramsConfig[],
                                uint8_t paramsCount ) :
    deviceInformation( deviceInfo ),
    params( paramsConfig ),
    paramsCnt( ( nullptr == paramsConfig ) ? 0u : ( ( paramsCountMax < paramsCount ) ? paramsCountMax : paramsCount ) )
{
}


/** \brief Handler processes ping, parameter read and parameter write */
crsf_FrameTypeStatus_t Crsf::AppLayer::Params::Get_FrameTypeStatus( crsf_FrameType_t frameType ) const
{
    return ( ( ( CRSF_FRAMETYPE_PARAM_PING  == frameType ) ||
               ( CRSF_FRAMETYPE_PARAM_READ  == frameType ) ||
               ( CRSF_FRAMETYPE_PARAM_WRITE == frameType )    ) ? CRSF_FRAMETYPE_SUPPORTED : CRSF_FRAMETYPE_NOT_SUPPORTED );
}


/**
 * \brief Processes parameter request
 *
 * \param frame  [in]: Received frame
 * \param sender [in]: Interface used for response
 */
void Crsf::AppLayer::Params::Process_Frame( const crsf_Frame_t &frame, FrameSender &sender )
{
    switch( frame.Type )
    {
        case CRSF_FRAMETYPE_PARAM_PING:  Process_Ping ( frame, sender ); break;
        case CRSF_FRAMETYPE_PARAM_READ:  Process_Read ( frame, sender ); break;
        case CRSF_FRAMETYPE_PARAM_WRITE: Process_Write( frame, sender ); break;
        default:                         /* Not processed */             break;
    }
}


/**
 * \brief Sends entry update requested by \ref Request_EntryUpdate
 *
 * \param taskTime [in]: Not used
 * \param sender   [in]: Interface used for sending
 */
void Crsf::AppLayer::Params::Task( crsf_Time_t taskTime, FrameSender &sender )
{
    (void)taskTime;

    if( true == updatePending )
    {
        updatePending = false;

        Send_Entry( updateDestAddr, updateParamNumber, 0u, sender );
    }
    else
    {
        /* No update requested */
    }
}


/**
 * \brief Requests sending of actual parameter entry (chunk 0) in next task
 *
 * Used eg. when status of COMMAND parameter changes (PROGRESS -> READY).
 *
 * \param paramNumber [in]: Parameter number
 * \param destAddr    [in]: Destination (host which wrote the parameter)
 *
 * \return \ref CRSF_REQUEST_ERROR for invalid parameter number
 */
crsf_RequestState_t Crsf::AppLayer::Params::Request_EntryUpdate( crsf_ParamNumber_t paramNumber, crsf_DeviceAddr_t destAddr )
{
    crsf_RequestState_t retState = CRSF_REQUEST_ERROR;

    if( paramsCnt >= paramNumber )
    {
        updateParamNumber = paramNumber;
        updateDestAddr    = destAddr;
        updatePending     = true;

        retState = CRSF_REQUEST_OK;
    }
    else
    {
        /* Invalid parameter number */
    }

    return ( retState );
}

/* ========================== LOCAL FUNCTIONS =============================== */

/** \brief Answers ping by device information */
void Crsf::AppLayer::Params::Process_Ping( const crsf_Frame_t &frame, FrameSender &sender )
{
    uint8_t       payload[ CRSF_EXT_PAYLOAD_MAX_LEN ] = { 0u };
    PayloadWriter writer( payload, CRSF_EXT_PAYLOAD_MAX_LEN );

    writer.Put_String( deviceInformation.Name );
    writer.Put_U32   ( deviceInformation.SerialNumber );
    writer.Put_U32   ( deviceInformation.HardwareId );
    writer.Put_U32   ( deviceInformation.FirmwareId );
    writer.Put_U8    ( paramsCnt );
    writer.Put_U8    ( deviceInformation.ParameterVersion );

    (void)sender.Send_ExtFrame( CRSF_FRAMETYPE_PARAM_DEVICE_INFO, frame.OrigAddr, payload, writer.Get_Length() );
}


/** \brief Answers parameter read by requested chunk of parameter entry */
void Crsf::AppLayer::Params::Process_Read( const crsf_Frame_t &frame, FrameSender &sender )
{
    if( crsf_ParamReadLen <= frame.PayloadLen )
    {
        PayloadReader reader( frame.Payload, frame.PayloadLen );

        const crsf_ParamNumber_t paramNumber = reader.Get_U8();
        const uint8_t            chunkNumber = reader.Get_U8();

        Send_Entry( frame.OrigAddr, paramNumber, chunkNumber, sender );
    }
    else
    {
        /* Invalid request */
    }
}


/** \brief Writes parameter value and answers by actual parameter entry */
void Crsf::AppLayer::Params::Process_Write( const crsf_Frame_t &frame, FrameSender &sender )
{
    if( crsf_ParamWriteMinLen <= frame.PayloadLen )
    {
        const crsf_ParamNumber_t paramNumber = frame.Payload[ 0u ];

        if( ( crsf_RootFolder != paramNumber ) && ( paramsCnt >= paramNumber ) )
        {
            Write_Value( params[ paramNumber - 1u ],
                         &frame.Payload[ crsf_ParamWriteMinLen ],
                         frame.PayloadLen - crsf_ParamWriteMinLen );
        }
        else
        {
            /* Root folder or not existing parameter cannot be written */
        }

        entryValid = false;

        Send_Entry( frame.OrigAddr, paramNumber, 0u, sender );
    }
    else
    {
        /* Invalid request */
    }
}


/**
 * \brief Sends one chunk of parameter entry
 *
 * \param destAddr    [in]: Destination address
 * \param paramNumber [in]: Parameter number
 * \param chunkNumber [in]: Chunk number (chunk 0 rebuilds the entry)
 * \param sender      [in]: Interface used for sending
 */
void Crsf::AppLayer::Params::Send_Entry( crsf_DeviceAddr_t destAddr, crsf_ParamNumber_t paramNumber,
                                          uint8_t chunkNumber, FrameSender &sender )
{
    if( ( 0u == chunkNumber ) || ( false == entryValid ) || ( entryParamNumber != paramNumber ) )
    {
        Build_Entry( paramNumber );
    }
    else
    {
        /* Following chunk of cached entry */
    }

    const uint8_t chunksCount = ( 0u == entryLength ) ? 1u
                                                      : static_cast<uint8_t>( ( entryLength + chunkSizeMax - 1u ) / chunkSizeMax );

    if( chunksCount > chunkNumber )
    {
        const uint16_t         chunkOffset = static_cast<uint16_t>( chunkNumber ) * chunkSizeMax;
        const crsf_MsgLength_t chunkLength = ( ( entryLength - chunkOffset ) > chunkSizeMax ) ?
                                             chunkSizeMax : static_cast<crsf_MsgLength_t>( entryLength - chunkOffset );

        uint8_t       payload[ CRSF_EXT_PAYLOAD_MAX_LEN ] = { 0u };
        PayloadWriter writer( payload, CRSF_EXT_PAYLOAD_MAX_LEN );

        writer.Put_U8   ( paramNumber );
        writer.Put_U8   ( static_cast<uint8_t>( chunksCount - chunkNumber - 1u ) );
        writer.Put_Bytes( &entryBuffer[ chunkOffset ], chunkLength );

        (void)sender.Send_ExtFrame( CRSF_FRAMETYPE_PARAM_ENTRY, destAddr, payload, crsf_EntryHeaderLen + chunkLength );
    }
    else
    {
        /* Chunk does not exist */
    }
}


/**
 * \brief Builds complete entry of parameter into entry buffer
 *
 * \param paramNumber [in]: Parameter number
 */
void Crsf::AppLayer::Params::Build_Entry( crsf_ParamNumber_t paramNumber )
{
    PayloadWriter writer( entryBuffer, entrySizeMax );

    if( crsf_RootFolder == paramNumber )
    {
        writer.Put_U8    ( crsf_RootFolder );
        writer.Put_U8    ( CRSF_PARAM_FOLDER );
        writer.Put_String( deviceInformation.Name );

        for( uint8_t paramIndex = 0u; paramsCnt > paramIndex; paramIndex++ )
        {
            if( crsf_RootFolder == params[ paramIndex ].ParentFolder )
            {
                writer.Put_U8( paramIndex + 1u );
            }
            else
            {
                /* Parameter in other folder */
            }
        }

        writer.Put_U8( folderListEnd );
    }
    else if( paramsCnt < paramNumber )
    {
        writer.Put_U8    ( crsf_RootFolder );
        writer.Put_U8    ( CRSF_PARAM_OUT_OF_RANGE );
        writer.Put_String( nullptr );
    }
    else
    {
        const crsf_ParamConfig_t &config = params[ paramNumber - 1u ];

        crsf_ParamValue_t value    = { config.Default, nullptr };
        uint8_t           typeByte = static_cast<uint8_t>( config.Type );

        if( ( nullptr          != config.VisibilityGetter ) &&
            ( CRSF_PARAM_HIDDEN == config.VisibilityGetter() ) )
        {
            typeByte |= hiddenFlag;
        }
        else
        {
            /* Parameter is visible */
        }

        if( ( nullptr          != config.Getter         ) &&
            ( CRSF_REQUEST_OK  != config.Getter( &value ) )    )
        {
            /* Value not available - default is reported */
            value.Number = config.Default;
            value.Text   = nullptr;
        }
        else
        {
            /* Value read */
        }

        writer.Put_U8    ( config.ParentFolder );
        writer.Put_U8    ( typeByte );
        writer.Put_String( config.Name );

        switch( config.Type )
        {
            case CRSF_PARAM_FLOAT:
                writer.Put_U32   ( static_cast<uint32_t>( value.Number ) );
                writer.Put_U32   ( static_cast<uint32_t>( config.Min ) );
                writer.Put_U32   ( static_cast<uint32_t>( config.Max ) );
                writer.Put_U32   ( static_cast<uint32_t>( config.Default ) );
                writer.Put_U8    ( config.DecimalPoint );
                writer.Put_U32   ( static_cast<uint32_t>( config.Step ) );
                writer.Put_String( config.Unit );
                break;

            case CRSF_PARAM_TEXT_SELECTION:
                writer.Put_String( config.Options );
                writer.Put_U8    ( static_cast<uint8_t>( value.Number ) );
                writer.Put_U8    ( 0u );
                writer.Put_U8    ( static_cast<uint8_t>( Get_OptionsCount( config.Options ) - 1u ) );
                writer.Put_U8    ( static_cast<uint8_t>( config.Default ) );
                writer.Put_String( config.Unit );
                break;

            case CRSF_PARAM_STRING:
                writer.Put_String( value.Text );
                writer.Put_U8    ( static_cast<uint8_t>( config.Max ) );
                break;

            case CRSF_PARAM_FOLDER:
                for( uint8_t paramIndex = 0u; paramsCnt > paramIndex; paramIndex++ )
                {
                    if( ( paramNumber == params[ paramIndex ].ParentFolder ) &&
                        ( paramNumber != ( paramIndex + 1u ) )                  )
                    {
                        writer.Put_U8( paramIndex + 1u );
                    }
                    else
                    {
                        /* Parameter in other folder */
                    }
                }

                writer.Put_U8( folderListEnd );
                break;

            case CRSF_PARAM_INFO:
                writer.Put_String( value.Text );
                break;

            case CRSF_PARAM_COMMAND:
                writer.Put_U8    ( static_cast<uint8_t>( value.Number ) );
                writer.Put_U8    ( static_cast<uint8_t>( config.Max ) );
                writer.Put_String( value.Text );
                break;

            default:
                /* Deprecated integer types are not supported - reported as out of range */
                entryBuffer[ 1u ] = CRSF_PARAM_OUT_OF_RANGE;
                break;
        }
    }

    entryLength      = writer.Get_Length();
    entryParamNumber = paramNumber;
    entryValid       = true;
}


/**
 * \brief Checks written value and passes it to setter
 *
 * \param config  [in]: Parameter configuration
 * \param data    [in]: Written data (behind parameter number)
 * \param dataLen [in]: Length of written data
 */
void Crsf::AppLayer::Params::Write_Value( const crsf_ParamConfig_t &config, const uint8_t * const data, crsf_MsgLength_t dataLen )
{
    PayloadReader     reader( data, dataLen );
    crsf_ParamValue_t value       = { 0, nullptr };
    bool              valueValid  = false;
    char              text[ CRSF_EXT_PAYLOAD_MAX_LEN ] = { crsf_StringEnd };

    switch( config.Type )
    {
        case CRSF_PARAM_FLOAT:
            value.Number = static_cast<int32_t>( reader.Get_U32() );
            valueValid   = ( false == reader.Is_Underflow() ) &&
                           ( config.Min <= value.Number ) && ( config.Max >= value.Number );
            break;

        case CRSF_PARAM_TEXT_SELECTION:
            value.Number = reader.Get_U8();
            valueValid   = ( false == reader.Is_Underflow() ) &&
                           ( Get_OptionsCount( config.Options ) > value.Number );
            break;

        case CRSF_PARAM_STRING:
        {
            /* String is truncated to maximal length of parameter */
            const int32_t          maxLength = ( 0 > config.Max ) ? 0 : config.Max;
            const crsf_MsgLength_t textSize  = ( static_cast<int32_t>( sizeof( text ) ) > maxLength ) ?
                                               static_cast<crsf_MsgLength_t>( maxLength + 1 ) : sizeof( text );

            reader.Get_String( text, textSize );

            value.Text = text;
            valueValid = ( false == reader.Is_Underflow() );
            break;
        }

        case CRSF_PARAM_COMMAND:
            value.Number = reader.Get_U8();
            valueValid   = ( false == reader.Is_Underflow() ) &&
                           ( ( CRSF_CMD_START   == value.Number ) ||
                             ( CRSF_CMD_CONFIRM == value.Number ) ||
                             ( CRSF_CMD_CANCEL  == value.Number )    );
            /* CRSF_CMD_POLL only requests actual entry */
            break;

        default:
            /* Folder, info and unsupported types cannot be written */
            valueValid = false;
            break;
    }

    if( ( true == valueValid ) && ( nullptr != config.Setter ) )
    {
        (void)config.Setter( &value );
    }
    else
    {
        /* Invalid value or read only parameter */
    }
}


/**
 * \brief Returns count of text selection options
 *
 * \param options [in]: Options separated by semicolon
 *
 * \return Count of options (at least 1)
 */
uint8_t Crsf::AppLayer::Params::Get_OptionsCount( const char * const options ) const
{
    uint8_t optionsCount = 1u;

    if( nullptr != options )
    {
        for( uint16_t charIndex = 0u; ( crsf_StringEnd != options[ charIndex ] ) && ( crsf_Uint8Max > optionsCount ); charIndex++ )
        {
            if( crsf_OptionsSeparator == options[ charIndex ] )
            {
                optionsCount++;
            }
            else
            {
                /* Option text */
            }
        }
    }
    else
    {
        /* No options */
    }

    return ( optionsCount );
}

/* =============================== TASKS ==================================== */
