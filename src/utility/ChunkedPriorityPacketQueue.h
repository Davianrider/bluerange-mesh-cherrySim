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

#pragma once

#include <array>
#include "FmTypes.h"
#include "ChunkedPacketQueue.h"

struct QueuePriorityPair
{
    ChunkedPacketQueue* queue;
    DeliveryPriority priority;
};

struct QueuePriorityPairConst
{
    const ChunkedPacketQueue* queue;
    DeliveryPriority priority;
};

constexpr u32 AMOUNT_OF_PRIORITY_DROPLETS_UNTIL_OVERFLOW = 2;
static_assert(AMOUNT_OF_PRIORITY_DROPLETS_UNTIL_OVERFLOW > 0, "Must be at least 1, else we always overflow and never send.");

class ChunkedPriorityPacketQueue
{
    //See Quality of Service documentation.
private:
    std::array<ChunkedPacketQueue, AMOUNT_OF_SEND_QUEUE_PRIORITIES> queues = {};
    std::array<u32,                AMOUNT_OF_SEND_QUEUE_PRIORITIES> priorityDroplets = {};

    QueuePriorityPair GetSplitQueue();
    QueuePriorityPairConst GetSplitQueue() const;

public:
    ChunkedPriorityPacketQueue();

    bool SplitAndAddMessage(DeliveryPriority prio, u8* data, u16 size, u16 payloadSizePerSplit, u32* messageHandle);
    u32 GetAmountOfPackets() const;
    bool IsCurrentlySendingSplitMessage() const;
    QueuePriorityPair GetSendQueue();
    ChunkedPacketQueue* GetQueueByPriority(DeliveryPriority prio);
    void RollbackLookAhead();
};


