/**
 * \author Mr.Nobody
 * \file Crsf_App_RcChannels.cpp
 * \ingroup Crsf
 * \brief CRSF 0x16 RC channels packed - receiver with failsafe detection.
 */

/* ============================= INCLUDES =================================== */
#include "Crsf_App_RcChannels.hpp"          /* Self include                   */
#include "Crsf_App_Codec.hpp"               /* Payload decoding               */
/* ============================= TYPEDEFS =================================== */

/* ======================= FORWARD DECLARATIONS ============================= */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* ============================== MACROS ==================================== */

/* ========================= EXPORTED VARIABLES ============================= */

/* ========================== LOCAL VARIABLES =============================== */

/* ======================== EXPORTED FUNCTIONS ============================== */

/**
 * \brief Constructor of RC channels receiver
 *
 * \param failsafeTimeoutMs [in]: Time without RC channels frame after which
 *                                the link is reported as lost [ms]
 */
Crsf::AppLayer::RcChannels::RcChannels( crsf_Time_t failsafeTimeoutMs ) :
    failsafeTimeout( failsafeTimeoutMs )
{
    for( uint8_t channelIndex = 0u; CRSF_RC_CHANNELS_CNT > channelIndex; channelIndex++ )
    {
        channels.Channels[ channelIndex ] = CRSF_RC_CHANNEL_VALUE_CENTER;
    }
}


/** \brief Handler processes only \ref CRSF_FRAMETYPE_RC_CHANNELS_PACKED */
crsf_FrameTypeStatus_t Crsf::AppLayer::RcChannels::Get_FrameTypeStatus( crsf_FrameType_t frameType ) const
{
    return ( ( CRSF_FRAMETYPE_RC_CHANNELS_PACKED == frameType ) ? CRSF_FRAMETYPE_SUPPORTED : CRSF_FRAMETYPE_NOT_SUPPORTED );
}


/**
 * \brief Decodes received channels, restarts failsafe timeout and calls callbacks
 *
 * \param frame  [in]: Received frame
 * \param sender [in]: Not used
 */
void Crsf::AppLayer::RcChannels::Process_Frame( const crsf_Frame_t &frame, FrameSender &sender )
{
    (void)sender;

    crsf_RcChannels_t newChannels = {};

    if( CRSF_REQUEST_OK == Codec::Decode_RcChannels( frame.Payload, frame.PayloadLen, newChannels ) )
    {
        channels       = newChannels;
        timeSinceFrame = 0u;
        framesCnt++;

        Change_LinkState( CRSF_LINK_ACTIVE );

        if( nullptr != channelsCallback )
        {
            channelsCallback( &channels );
        }
        else
        {
            /* Callback not assigned */
        }
    }
    else
    {
        /* Invalid payload length */
    }
}


/**
 * \brief Checks failsafe timeout
 *
 * \param taskTime [in]: Period of task calling in ms [0.001s]
 * \param sender   [in]: Not used
 */
void Crsf::AppLayer::RcChannels::Task( crsf_Time_t taskTime, FrameSender &sender )
{
    (void)sender;

    if( CRSF_LINK_ACTIVE == linkState )
    {
        timeSinceFrame += taskTime;

        if( failsafeTimeout <= timeSinceFrame )
        {
            Change_LinkState( CRSF_LINK_LOST );
        }
        else
        {
            /* Link is active */
        }
    }
    else
    {
        /* Link already lost */
    }
}


/** \brief Sets callback called with every received RC channels frame (null disables) */
void Crsf::AppLayer::RcChannels::Set_ChannelsCallback( crsf_RcChannelsCallback_t *callback )
{
    channelsCallback = callback;
}


/** \brief Sets callback called when the link state changes (null disables) */
void Crsf::AppLayer::RcChannels::Set_LinkStateCallback( crsf_LinkStateCallback_t *callback )
{
    linkStateCallback = callback;
}


/** \brief Returns actual link state */
crsf_LinkState_t Crsf::AppLayer::RcChannels::Get_LinkState( void ) const
{
    return ( linkState );
}


/** \brief Returns last received channels (raw values) */
const crsf_RcChannels_t &Crsf::AppLayer::RcChannels::Get_Channels( void ) const
{
    return ( channels );
}


/**
 * \brief Returns raw value of one channel
 *
 * \param channelIndex [in]: Channel index 0 - 15 (channel 1 - 16)
 *
 * \return Raw value, center value for invalid index
 */
uint16_t Crsf::AppLayer::RcChannels::Get_Channel( uint8_t channelIndex ) const
{
    return ( ( CRSF_RC_CHANNELS_CNT > channelIndex ) ? channels.Channels[ channelIndex ] : CRSF_RC_CHANNEL_VALUE_CENTER );
}


/**
 * \brief Returns pulse width of one channel
 *
 * \param channelIndex [in]: Channel index 0 - 15 (channel 1 - 16)
 *
 * \return Pulse width in us, 1500 us for invalid index
 */
int32_t Crsf::AppLayer::RcChannels::Get_ChannelUs( uint8_t channelIndex ) const
{
    return ( Codec::Get_ChannelUs( Get_Channel( channelIndex ) ) );
}


/** \brief Returns count of received valid RC channels frames */
crsf_Counter_t Crsf::AppLayer::RcChannels::Get_FramesCnt( void ) const
{
    return ( framesCnt );
}

/* ========================== LOCAL FUNCTIONS =============================== */

/**
 * \brief Changes link state and calls callback on change
 *
 * \param newState [in]: New link state
 */
void Crsf::AppLayer::RcChannels::Change_LinkState( crsf_LinkState_t newState )
{
    if( newState != linkState )
    {
        linkState = newState;

        if( nullptr != linkStateCallback )
        {
            linkStateCallback( linkState );
        }
        else
        {
            /* Callback not assigned */
        }
    }
    else
    {
        /* No change */
    }
}

/* =============================== TASKS ==================================== */
