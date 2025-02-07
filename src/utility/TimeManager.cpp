////////////////////////////////////////////////////////////////////////////////
// /****************************************************************************
// **
// ** Copyright (C) 2015-2022 M-Way Solutions GmbH
// ** Contact: https://www.blureange.io/licensing
// **
// ** This file is part of the Bluerange/FruityMesh implementation
// **
// ** $BR_BEGIN_LICENSE:GPL-EXCEPT$
// ** Commercial License Usage
// ** Licensees holding valid commercial Bluerange licenses may use this file in
// ** accordance with the commercial license agreement provided with the
// ** Software or, alternatively, in accordance with the terms contained in
// ** a written agreement between them and M-Way Solutions GmbH. 
// ** For licensing terms and conditions see https://www.bluerange.io/terms-conditions. For further
// ** information use the contact form at https://www.bluerange.io/contact.
// **
// ** GNU General Public License Usage
// ** Alternatively, this file may be used under the terms of the GNU
// ** General Public License version 3 as published by the Free Software
// ** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
// ** included in the packaging of this file. Please review the following
// ** information to ensure the GNU General Public License requirements will
// ** be met: https://www.gnu.org/licenses/gpl-3.0.html.
// **
// ** $BR_END_LICENSE$
// **
// ****************************************************************************/
////////////////////////////////////////////////////////////////////////////////

#include "TimeManager.h"
#include "mini-printf.h"
#include "GlobalState.h"
#include "Node.h"
#include "Utility.h"

/*
Known Limitation: Starting a timesync on multiple nodes around the same time with different times will
not sync the same time to all nodes. Doing another sync on one node will however correct this.
The reason is that both nodes will generate the same counter value and therefore, there will be no winner.
*/

TimeManager::TimeManager()
{
    syncTime = 0;
    timeSinceSyncTime = 0;
}

u32 TimeManager::GetUtcTime()
{
    ProcessTicks();
    u32 unixTime = syncTime + timeSinceSyncTime;

    return unixTime;
}

u32 TimeManager::GetLocalTime()
{
    ProcessTicks();
    u32 unixTime = syncTime + timeSinceSyncTime;
    i32 offsetSeconds = offset * 60;
    if (offsetSeconds < 0 && unixTime < static_cast<u32>(-offsetSeconds))
    {
        //Edge case.
        return unixTime;
    }
    else
    {
        return static_cast<u32>(unixTime + offsetSeconds);
    }
}

i16 TimeManager::GetOffset()
{
    return offset;
}

bool TimeManager::IsTimeMaster()
{
    return isTimeMaster;
}

TimePoint TimeManager::GetLocalTimePoint()
{
    ProcessTicks();
    return TimePoint(GetLocalTime(), additionalTicks);
}

void TimeManager::SetMasterTime(u32 syncTimeDs, u32 timeSinceSyncTimeDs, i16 offset, u32 additionalTicks)
{
    this->syncTime = syncTimeDs;
    this->timeSinceSyncTime = timeSinceSyncTimeDs;
    GS->caltick =  additionalTicks; //new test
    GS->inittimeSinceSyncTime = timeSinceSyncTimeDs; //new test
    this->additionalTicks = additionalTicks;
    this->offset = offset;
    this->counter++;
    this->waitingForCorrection = false;
    this->timeCorrectionReceived = true;
    FruityHal::UpdateDelayTimer(); // new Sync DelayTimer
    this->isTimeMaster = true;
    //trace("master timeSinceSyncTime:%u",this->timeSinceSyncTime); //note test new
    //We inform the connection manager so that it resends the time sync messages.
    logt("TSYNC", "Received time by command! NodeId: %u", (u32)GS->node.configuration.nodeId);
    GS->cm.ResetTimeSync();
}

void TimeManager::SetTime(const TimeSyncInitial & timeSyncIntitialMessage)
{
    if (timeSyncIntitialMessage.counter > this->counter)
    {
        this->syncTime = timeSyncIntitialMessage.syncTimeStamp;
        this->timeSinceSyncTime = timeSyncIntitialMessage.timeSincSyncTimeStamp;
        GS->caltick = timeSyncIntitialMessage.additionalTicks; //new test
        GS->inittimeSinceSyncTime = timeSyncIntitialMessage.timeSincSyncTimeStamp; //new test
        this->additionalTicks = timeSyncIntitialMessage.additionalTicks;
        this->offset = timeSyncIntitialMessage.offset;
        this->counter = timeSyncIntitialMessage.counter; //THIS is the main difference to SetTime(u32,u32,u32)!
        this->waitingForCorrection = true;
        this->timeCorrectionReceived = false;

        this->isTimeMaster = false;
        GS->appTimerDs = timeSyncIntitialMessage.appTimerDs; //new 
        trace("syncTime : %u timeSinceSyncTime : %u addticks : %u offset : %u counter : %u " EOL,  timeSyncIntitialMessage.syncTimeStamp,timeSyncIntitialMessage.timeSincSyncTimeStamp,timeSyncIntitialMessage.additionalTicks,timeSyncIntitialMessage.offset,timeSyncIntitialMessage.counter);
        //We inform the connection manager so that it resends the time sync messages.
        //trace("timeSinceSyncTime:%u",this->timeSinceSyncTime); //note test new
        logt("TSYNC", "Received time by mesh! NodeId: %u, Partner: %u", (u32)GS->node.configuration.nodeId, (u32)timeSyncIntitialMessage.header.header.sender);
        FruityHal::UpdateDelayTimer(); // new Sync DelayTimer
        GS->cm.ResetTimeSync();
    }
}

void TimeManager::SetTime(const TimeSyncInterNetwork& timeSyncInterNetwork)
{
    if (this->counter == 0 || GET_DEVICE_TYPE() == DeviceType::ASSET)
    {
        this->syncTime = timeSyncInterNetwork.syncTimeStamp;
        this->timeSinceSyncTime = timeSyncInterNetwork.timeSincSyncTimeStamp;
        this->additionalTicks = timeSyncInterNetwork.additionalTicks;
        this->offset = timeSyncInterNetwork.offset;
        this->counter++;
        this->waitingForCorrection = false;
        this->timeCorrectionReceived = false;

        this->isTimeMaster = false;

        //We inform the connection manager so that it resends the time sync messages.
        logt("TSYNC", "Received time by inter mesh! NodeId: %u, Partner: %u", (u32)GS->node.configuration.nodeId, (u32)timeSyncInterNetwork.header.header.sender);
        GS->cm.ResetTimeSync();
    }
}

bool TimeManager::IsTimeSynced() const
{
    return syncTime != 0;
}

bool TimeManager::IsTimeCorrected() const
{
    return (timeCorrectionReceived || !waitingForCorrection) && IsTimeSynced();
}

void TimeManager::AddTicks(u32 ticks)
{
    additionalTicks += ticks;
}

void TimeManager::AddCorrection(u32 ticks)
{
    if (waitingForCorrection)
    {
        AddTicks(ticks);
        GS->appTimerDs += (int(ticks / 3276.8)); //new update appTimerDS ticksPerSecond 32768
        GS->caltick += ticks; //new
        this->waitingForCorrection = false;
        this->timeCorrectionReceived = true;

        if(timeSyncedListener)
        {
            timeSyncedListener->TimeSyncedHandler();
        }

        logt("TSYNC", "Time synced and corrected");
    }
}

void TimeManager::ProcessTicks()
{
    u32 seconds = additionalTicks / ticksPerSecond;
    timeSinceSyncTime += seconds;

    additionalTicks -= seconds * ticksPerSecond;
}



void TimeManager::HandleUpdateTimestampMessages(ConnPacketHeader const * packetHeader, MessageLength dataLength)
{
    if (packetHeader->messageType == MessageType::UPDATE_TIMESTAMP)
    {
        //Set our time to the received timestamp
        connPacketUpdateTimestamp const * packet = (connPacketUpdateTimestamp const *)packetHeader;
        if (dataLength >= offsetof(connPacketUpdateTimestamp, offset) + sizeof(packet->offset))
        {
            SetMasterTime(packet->timestampSec, 0, packet->offset);
        }
        else
        {
            SetMasterTime(packet->timestampSec, 0, 0);
        }
    }
}

void TimeManager::ConvertTimeToString(char* buffer, u16 bufferSize)
{
    ProcessTicks();
    TimeManager::ConvertTimeToString(GetUtcTime(), GetOffset(), additionalTicks, buffer, bufferSize);
}

void TimeManager::ConvertTimeToString(u32 unixTimestamp, i16 offset, u32 ticks, char* buffer, u16 bufferSize)
{
    u32 localTime = unixTimestamp;
    i32 offsetSeconds = offset * 60;
    if (offsetSeconds < 0 && localTime < static_cast<u32>(-offsetSeconds))
    {
        //Edge case.
        snprintf(buffer, bufferSize, "Negative Offset (%d) smaller than timestamp (%u)", offset, unixTimestamp);
        return;
    }
    else
    {
        localTime = unixTimestamp + offsetSeconds;
    }


    u32 remainingSeconds = localTime;

    u32 yearDivider = 60 * 60 * 24 * 365;
    u16 years = remainingSeconds / yearDivider + 1970;
    remainingSeconds = remainingSeconds % yearDivider;

    u32 gapDays = (years - 1970) / 4 - 1;
    u32 dayDivider = 60 * 60 * 24;
    u16 days = remainingSeconds / dayDivider;
    days -= gapDays;
    remainingSeconds = remainingSeconds % dayDivider;

    u32 hourDivider = 60 * 60;
    u16 hours = remainingSeconds / hourDivider;
    remainingSeconds = remainingSeconds % hourDivider;

    u32 minuteDivider = 60;
    u16 minutes = remainingSeconds / minuteDivider;
    remainingSeconds = remainingSeconds % minuteDivider;

    u32 seconds = remainingSeconds;

    snprintf(buffer, bufferSize, "approx. %u years, %u days, %02uh:%02um:%02us,%u ticks (offset %d)", years, days, hours, minutes, seconds, ticks, offset);
}

TimeSyncInitial TimeManager::GetTimeSyncIntialMessage(NodeId receiver) const
{
    TimeSyncInitial retVal;
    CheckedMemset(&retVal, 0, sizeof(retVal));
    
    retVal.header.header.messageType = MessageType::TIME_SYNC;
    retVal.header.header.receiver = receiver;
    retVal.header.header.sender = GS->node.configuration.nodeId;
    retVal.header.type = TimeSyncType::INITIAL;

    retVal.syncTimeStamp = syncTime;
    retVal.timeSincSyncTimeStamp = timeSinceSyncTime;
    retVal.additionalTicks = additionalTicks;
    retVal.offset = offset;
    retVal.counter = counter;
    retVal.appTimerDs = GS->appTimerDs;//new
    return retVal;
}

TimeSyncInterNetwork TimeManager::GetTimeSyncInterNetworkMessage(NodeId receiver) const
{
    TimeSyncInterNetwork retVal;
    CheckedMemset(&retVal, 0, sizeof(retVal));

    retVal.header.header.messageType = MessageType::TIME_SYNC;
    retVal.header.header.receiver = receiver;
    retVal.header.header.sender = GS->node.configuration.nodeId;
    retVal.header.type = TimeSyncType::INTER_NETWORK;

    retVal.syncTimeStamp = syncTime;
    retVal.timeSincSyncTimeStamp = timeSinceSyncTime;
    retVal.additionalTicks = additionalTicks;
    retVal.offset = offset;

    return retVal;
}

void TimeManager::AddTimeSyncedListener(TimeSyncedListener* listener)
{
    if (timeSyncedListener)
    {
        // The current implementation only allows a single Listener. If more
        // is needed, change timeSyncedListener to be an array instead.
        SIMEXCEPTION(IllegalStateException);
    }
    timeSyncedListener = listener;
}

TimePoint::TimePoint(u32 unixTime, u32 additionalTicks)
    :unixTime(unixTime), additionalTicks(additionalTicks)
{
}

TimePoint::TimePoint()
    : unixTime(0), additionalTicks(0)
{
}

i32 TimePoint::operator-(const TimePoint & other)
{
    const i32 secondDifference = this->unixTime - other.unixTime;
    const i32 ticksDifference = this->additionalTicks - other.additionalTicks;
    
    return ticksDifference + secondDifference * ticksPerSecond;
}

u32 TimePoint::GetAdditionalTicks() const
{
    return additionalTicks;
}
