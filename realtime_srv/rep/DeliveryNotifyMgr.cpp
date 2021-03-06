#include "realtime_srv/common/RealtimeSrvShared.h"

using namespace realtime_srv;

namespace
{
const float kDelayBeforeAckTimeout = 0.6f;
}

DeliveryNotifyMgr::DeliveryNotifyMgr( bool inShouldSendAcks, bool inShouldProcessAcks ) :
	mNextOutgoingSequenceNumber( 0 ),
	mNextExpectedSequenceNumber( 0 ),
	mShouldSendAcks( inShouldSendAcks ),
	mShouldProcessAcks( inShouldProcessAcks ),
	mDeliveredPacketCount( 0 ),
	mDroppedPacketCount( 0 ),
	mDispatchedPacketCount( 0 )
{
	if ( mShouldSendAcks )
	{
		mAckBitField = new AckBitField();
	}
}

DeliveryNotifyMgr::~DeliveryNotifyMgr()
{
	//LOG( "DNM destructor. Delivery rate %d%%, Drop rate %d%%",
	//	( 100 * mDeliveredPacketCount ) / mDispatchedPacketCount,
	//	( 100 * mDroppedPacketCount ) / mDispatchedPacketCount );

	if ( mShouldSendAcks && mAckBitField )
	{
		delete mAckBitField;
	}
}

InFlightPacket* DeliveryNotifyMgr::WriteSequenceNumber( OutputBitStream& inOutputStream,
	ClientProxy* inClientProxy )
{

	PacketSN sequenceNumber = mNextOutgoingSequenceNumber++;
	inOutputStream.Write( sequenceNumber );

	++mDispatchedPacketCount;

	if ( mShouldProcessAcks )
	{
		mInFlightPackets.emplace_back( sequenceNumber, inClientProxy );

		return &mInFlightPackets.back();
	}
	else
	{
		return nullptr;
	}
}

bool DeliveryNotifyMgr::ProcessSequenceNumber( InputBitStream& inInputStream )
{
	PacketSN	sequenceNumber;

	inInputStream.Read( sequenceNumber );
	if ( RealtimeSrvHelper::SequenceGreaterThanOrEqual( sequenceNumber, mNextExpectedSequenceNumber ) )
		//if ( sequenceNumber >= mNextExpectedSequenceNumber )
	{
		PacketSN lastSN = mNextExpectedSequenceNumber - 1;
		mNextExpectedSequenceNumber = sequenceNumber + 1;

		if ( mShouldSendAcks )
		{
			mAckBitField->AddToAckBitField( sequenceNumber, lastSN );
		}

		return true;
	}
	else
	{
		return false;
	}

	return false;
}

void DeliveryNotifyMgr::ProcessTimedOutPackets()
{
	float timeoutTime = RealtimeSrvTiming::sInst.GetCurrentGameTime() - kDelayBeforeAckTimeout;

	while ( !mInFlightPackets.empty() )
	{
		const auto& nextInFlightPacket = mInFlightPackets.front();

		if ( nextInFlightPacket.GetTimeDispatched() < timeoutTime )
		{
			HandlePacketDeliveryFailure( nextInFlightPacket );
			mInFlightPackets.pop_front();
		}
		else
		{
			break;
		}
	}
}

void DeliveryNotifyMgr::HandlePacketDeliveryFailure( const InFlightPacket& inFlightPacket )
{
	++mDroppedPacketCount;
	inFlightPacket.HandleDeliveryFailure();
}

void DeliveryNotifyMgr::HandlePacketDeliverySuccess( const InFlightPacket& inFlightPacket )
{
	++mDeliveredPacketCount;
	inFlightPacket.HandleDeliverySuccess();
}

void DeliveryNotifyMgr::ProcessAckBitField( InputBitStream& inInputStream )
{
	AckBitField recvedAckBitField;
	recvedAckBitField.Read( inInputStream );

	PacketSN LastAckedSN = recvedAckBitField.GetLatestAckSN();
	PacketSN nextAckedSN =
		LastAckedSN - ( ACK_BIT_FIELD_BYTE_LEN << 3 );


	while (
		RealtimeSrvHelper::SequenceGreaterThanOrEqual( LastAckedSN, nextAckedSN )
		//LastAckedSN >= nextAckdSequenceNumber
		&& !mInFlightPackets.empty() )
	{
		const auto& nextInFlightPacket = mInFlightPackets.front();
		PacketSN nextInFlightPacketSN = nextInFlightPacket.GetSequenceNumber();

		if ( RealtimeSrvHelper::SequenceGreaterThan( nextAckedSN, nextInFlightPacketSN ) )
			//if ( nextAckedSN > nextInFlightPacketSN )
		{
			auto copyOfInFlightPacket = nextInFlightPacket;
			mInFlightPackets.pop_front();
			HandlePacketDeliveryFailure( copyOfInFlightPacket );
		}
		else if ( nextAckedSN == nextInFlightPacketSN )
		{
			if ( nextAckedSN == LastAckedSN
				|| recvedAckBitField.IsSetCorrespondingAckBit( nextAckedSN ) )
			{
				HandlePacketDeliverySuccess( nextInFlightPacket );
				mInFlightPackets.pop_front();
				++nextAckedSN;
			}
			else
			{
				auto copyOfInFlightPacket = nextInFlightPacket;
				mInFlightPackets.pop_front();
				HandlePacketDeliveryFailure( copyOfInFlightPacket );
				++nextAckedSN;
			}
		}
		else if ( RealtimeSrvHelper::SequenceGreaterThan( nextInFlightPacketSN, nextAckedSN ) )
	 //else if ( nextAckedSN < nextInFlightPacketSN )
		{
			nextAckedSN = nextInFlightPacketSN;
		}
	}
}