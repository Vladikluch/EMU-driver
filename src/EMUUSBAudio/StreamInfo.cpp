//
//  StreamInfo.cpp
//  EMUUSBAudio
//
//  Created by Wouter Pasman on 09/01/15.
//  Copyright (c) 2015 com.emu. All rights reserved.
//

#include "StreamInfo.h"
#include "EMUUSBLogging.h"
#include "EMUUSBAudioCommon.h"

IOReturn StreamInfo::init() {
    
    return kIOReturnSuccess;
    
}

IOReturn StreamInfo::start(UInt64 startUsbFrame) {
    ReturnIf(startUsbFrame < streamInterface->getDevice1()->getFrameNumber() + 10, kIOReturnTimeout);
    
    nextUsableUsbFrameNr = startUsbFrame;
    
    return kIOReturnSuccess;
}

IOReturn StreamInfo::reset() {
    ReturnIf(!pipe, kIOReturnNotOpen);
    
    UInt16 pollInterval  = 1 << (pipe->GetEndpointDescriptor()->bInterval - 1);
    frameNumberIncreasePerCycle  = (NUMBER_FRAMES / 8) * pollInterval; // 1 per frame
    
    return kIOReturnSuccess;
}

UInt64 StreamInfo::getNextFrameNr() {
    UInt64 current = nextUsableUsbFrameNr;
    nextUsableUsbFrameNr += frameNumberIncreasePerCycle;
    
    return current;
}