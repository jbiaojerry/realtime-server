// Fill out your copyright notice in the Description page of Project Settings.


#include "RealTimeSrvHelper.h"
#include "RealTimeSrvTiming.h"


//ScreenMsg
void RealTimeSrvHelper::ScreenMsg( const FString& Msg )
{
	if (!RTS_SHOW_DEBUG_SCREEN_MSG) return;
	GEngine->AddOnScreenDebugMessage( -1, 55.f, FColor::Red, *Msg );
}

void RealTimeSrvHelper::ScreenMsg( const FString& Msg, const FString& Msg2 )
{
	if (!RTS_SHOW_DEBUG_SCREEN_MSG) return;
	GEngine->AddOnScreenDebugMessage( -1, 55.f, FColor::Red, FString::Printf( TEXT( "%s %s" ), *Msg, *Msg2 ) );
}

void RealTimeSrvHelper::ScreenMsg( const FString& Msg, const float Value )
{
	if (!RTS_SHOW_DEBUG_SCREEN_MSG) return;
	GEngine->AddOnScreenDebugMessage( -1, 55.f, FColor::Red, FString::Printf( TEXT( "%s %f" ), *Msg, Value ) );
}

