#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>
#include <memory>

#include "realtime_srv/math/Vector3.h"
#include "realtime_srv/math/Quaternion.h"
#include "realtime_srv/math/Matrix3x3.h"
#include "realtime_srv/math/Vector2.h"
#include "realtime_srv/common/RealtimeSrvMacro.h"


namespace realtime_srv
{

inline uint32_t ConvertToFixed( float inNumber, float inMin, float inPrecision )
{
	return static_cast< int > ( ( inNumber - inMin ) / inPrecision );
}

inline float ConvertFromFixed( uint32_t inNumber, float inMin, float inPrecision )
{
	return static_cast< float >( inNumber ) * inPrecision + inMin;
}


class OutputBitStream
{
public:

	OutputBitStream() :
		mBuffer( nullptr ),
		mSlicePoint( 0 ),
		mBitHead( 0 )
	{
		ReallocBuffer( MAX_PACKET_BYTE_LENGTH * 8 );
	}

	~OutputBitStream() { std::free( mBuffer ); }

	bool SliceTo( OutputBitStream& refOutputBitStream );
	void SliceTo( OutputBitStream& refOutputBitStream, uint8_t inData, uint32_t inBitCount );

	void		WriteBits( uint8_t inData, uint32_t inBitCount );
	void		WriteBits( const void* inData, uint32_t inBitCount );

	const 	char*	GetBufferPtr()		const { return mBuffer; }
	uint32_t		GetBitLength()		const { return mBitHead; }
	uint32_t		GetByteLength()		const { return ( mBitHead + 7 ) >> 3; }

	void WriteBytes( const void* inData, uint32_t inByteCount ) { WriteBits( inData, inByteCount << 3 ); }

	/*
	void Write( uint32_t inData, uint32_t inBitCount = 32 )	{ WriteBits( &inData, inBitCount ); }
	void Write( int inData, uint32_t inBitCount = 32 )		{ WriteBits( &inData, inBitCount ); }
	void Write( float inData )								{ WriteBits( &inData, 32 ); }

	void Write( uint16_t inData, uint32_t inBitCount = 16 )	{ WriteBits( &inData, inBitCount ); }
	void Write( int16_t inData, uint32_t inBitCount = 16 )	{ WriteBits( &inData, inBitCount ); }

	void Write( uint8_t inData, uint32_t inBitCount = 8 )	{ WriteBits( &inData, inBitCount ); }
	*/

	template< typename T >
	void Write( T inData, uint32_t inBitCount = sizeof( T ) * 8 )
	{
		static_assert( std::is_arithmetic< T >::value ||
			std::is_enum< T >::value,
			"Generic Write only supports primitive data types" );
		WriteBits( &inData, inBitCount );
	}

	void 		Write( bool inData ) { WriteBits( &inData, 1 ); }

	void		Write( const Vector3& inVector );
	void		Write( const Quaternion& inQuat );

	void Write( const std::string& inString )
	{
		uint32_t elementCount = static_cast< uint32_t >( inString.size() );
		Write( elementCount );
		for ( const auto& element : inString )
		{
			Write( element );
		}
	}

	//void Write( const PacketSN& inPacketSN )
	//{
	//	inPacketSN.Write( *this );
	//}

	void		ReallocBuffer( uint32_t inNewBitCapacity );

private:

	char*		mBuffer;
	uint32_t	mSlicePoint;
	uint32_t	mBitHead;
	uint32_t	mBitCapacity;
};
typedef std::shared_ptr<OutputBitStream> OutputBitStreamPtr;


class InputBitStream
{
public:

	InputBitStream( const char* inBuffer = nullptr, const uint32_t inBitCount = 0 ) :
		mBitHead( 0 ),
		mBitCapacity( inBitCount ),
		mRecombinePoint( 0 )
	{
		if ( inBuffer )
		{
			int byteCount = mBitCapacity / 8;
			mBuffer = static_cast< char* >( malloc( inBitCount ) );
			memcpy( mBuffer, inBuffer, byteCount );
			mIsBufferOwner = true;
		}
		else
		{
			mBuffer = nullptr;
			mIsBufferOwner = false;
		}
	}

	InputBitStream( const InputBitStream& inOther ) :
		mBitHead( inOther.mBitHead ),
		mBitCapacity( inOther.mBitCapacity ),
		mRecombinePoint( inOther.mRecombinePoint ),
		mIsBufferOwner( true )
	{
		int byteCount = mBitCapacity / 8;
		mBuffer = static_cast< char* >( malloc( byteCount ) );
		memcpy( mBuffer, inOther.mBuffer, byteCount );
	}

	InputBitStream& operator=( const InputBitStream& inOther )
	{
		if ( this == &inOther )
		{
			return *this;
		}

		mBitCapacity = inOther.mBitCapacity;
		mBitHead = inOther.mBitHead;
		mRecombinePoint = inOther.mRecombinePoint;
		if ( mIsBufferOwner && mBuffer )
		{
			free( mBuffer );
		}
		int byteCount = mBitCapacity / 8;
		mBuffer = static_cast< char* >( malloc( byteCount ) );
		memcpy( mBuffer, inOther.mBuffer, byteCount );
		mIsBufferOwner = true;
		return *this;
	}

	~InputBitStream()
	{
		if ( mIsBufferOwner && mBuffer )
		{
			free( mBuffer );
			mBuffer = nullptr;
		}
	}
	const 	char*	GetBufferPtr()		const { return mBuffer; }
	uint32_t	GetRemainingBitCount() 	const { return mBitCapacity - mBitHead; }

	void		ReadBits( uint8_t& outData, uint32_t inBitCount );
	void		ReadBits( void* outData, uint32_t inBitCount );

	void		ReadBytes( void* outData, uint32_t inByteCount ) { ReadBits( outData, inByteCount << 3 ); }

	template< typename T >
	void Read( T& inData, uint32_t inBitCount = sizeof( T ) * 8 )
	{
		static_assert( std::is_arithmetic< T >::value ||
			std::is_enum< T >::value,
			"Generic Read only supports primitive data types" );
		ReadBits( &inData, inBitCount );
	}

	void		Read( uint32_t& outData, uint32_t inBitCount = 32 ) { ReadBits( &outData, inBitCount ); }
	void		Read( int& outData, uint32_t inBitCount = 32 ) { ReadBits( &outData, inBitCount ); }
	void		Read( float& outData ) { ReadBits( &outData, 32 ); }

	void		Read( uint16_t& outData, uint32_t inBitCount = 16 ) { ReadBits( &outData, inBitCount ); }
	void		Read( int16_t& outData, uint32_t inBitCount = 16 ) { ReadBits( &outData, inBitCount ); }

	void		Read( uint8_t& outData, uint32_t inBitCount = 8 ) { ReadBits( &outData, inBitCount ); }
	void		Read( bool& outData ) { ReadBits( &outData, 1 ); }

	void		Read( Quaternion& outQuat );

	void		ResetToCapacity( uint32_t inByteCapacity ) { mBitCapacity = inByteCapacity << 3; mBitHead = 0; }
	void		ResetToCapacityFromBit( uint32_t inBitCapacity ) { mBitCapacity = inBitCapacity; mBitHead = 0; }

	void		RecombineTo( InputBitStream& refInputBitStream );

	void Reinit( uint32_t inBitCount )
	{
		if ( mIsBufferOwner && mBuffer )
		{
			free( mBuffer );
		}
		int byteCount = inBitCount / 8;
		mBuffer = static_cast< char* >( malloc( byteCount ) );
		mBitCapacity = inBitCount;
		mBitHead = 0;
		mRecombinePoint = 0;
		mIsBufferOwner = true;
	}

	uint32_t	GetRecombinePoint() const { return mRecombinePoint; }


	void Read( std::string& inString )
	{
		uint32_t elementCount;
		Read( elementCount );
		inString.resize( elementCount );
		for ( auto& element : inString )
		{
			Read( element );
		}
	}

	void Read( Vector3& inVector );

	//void Read( PacketSN& inPacketSN )
	//{
	//	inPacketSN.Read( *this );
	//}

private:
	char*		mBuffer;
	uint32_t	mBitHead;
	uint32_t	mBitCapacity;
	uint32_t	mRecombinePoint;
	bool		mIsBufferOwner;

};
typedef std::shared_ptr<InputBitStream> InputBitStreamPtr;

}