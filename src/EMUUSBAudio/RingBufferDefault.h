//
//  RingBufferDefault.h
//  EMUUSBAudio
//
//  Created by Wouter Pasman on 28/11/14.
//  Copyright (c) 2014 Wouter Pasman. All rights reserved.
//

#ifndef EMUUSBAudio_RingBufferDefault_h
#define EMUUSBAudio_RingBufferDefault_h


#include "RingBufferT.h"
#include "EMUUSBLogging.h"

/*! Default implementation for RingBufferT.
 This is still a template because of the TYPE but actually this is a complete
 implementation.
 
 * Partially thread safe:
 *  read and write can be called in parallel from different threads
 *  read should not be called from multiple threads at same time
 *  write idem.
 
 * We do not need full thread safety because there is only 1 producer (GatherInputSamples)
 * and one consumer (IOAudioEngine).
 */
template <typename TYPE>

class RingBufferDefault: public RingBufferT<TYPE> {
public:
	TYPE *buffer=0; //
    char * typeName;
    UInt32 size=0; // number of elements in buffer.
    UInt32 readhead; // index of next read. range [0,SIZE>
    UInt32 writehead; // index of next write. range [0,SIZE>
    // true if someone recently called pop. if false, suppresses overrun warnings.
    Boolean isPopped=false;
    
public:
    
    IOReturn init(UInt32 newSize, char* name) override {
        typeName = name;
        debugIOLogR("ringbuffer<%s> allocate %d",typeName, newSize);
        if (newSize<=0) {
            return kIOReturnBadArgument;
        }

        free(); // just in case free was not done of old buffer
        
        size=newSize;
		readhead=0;
        writehead=0;
    
        // allocate buffer as last step as this is flag that ring is ready for use.
        buffer=(TYPE *)IOMalloc(size * sizeof(TYPE));
        if (buffer==0) {
            size=0;
            return kIOReturnNoResources;
        }
        return kIOReturnSuccess;
	}
    
    void free() {
        if (buffer){
            debugIOLogR("ringbuffer<%s> freed %d",typeName,size);
            IOFree(buffer,size * sizeof(TYPE));
            buffer=0;
            size=0;
        }
    }
    
    
    IOReturn push(TYPE object, UInt64 time) override{
        if (!buffer) {
            return kIOReturnNotReady;
        }
        UInt32 newwritehead = writehead+1;
        if (newwritehead== size) newwritehead=0;
        if (newwritehead == readhead) {
            return kIOReturnOverrun ;
        }
        buffer[writehead]= object;
        writehead = newwritehead;
        if (writehead == 0) notifyWrap(time);
        return kIOReturnSuccess;
	}
    
    IOReturn push(TYPE *objects, UInt32 num, UInt64 time, UInt32 time_per_obj) override{
        if (!buffer) {
            return kIOReturnNotReady;
        }
        
        if (num > vacant() && isPopped) {
            doLog("RingBufferDefault<%s>::push warning. Ignoring overrun",typeName);
            isPopped=false;
        }
        for ( UInt32 n = 0; n<num; n++) {
            buffer[writehead++] = objects[n];
            if (writehead == size) { writehead = 0; notifyWrap(time + n * time_per_obj); }
        }
        return kIOReturnSuccess;
    }
    
    IOReturn pop(TYPE * data) override{
        if (!buffer) {
            return kIOReturnNotReady;
        }
        isPopped=true;
        if (readhead == writehead) {
            return kIOReturnUnderrun;
        }
        *data = buffer[readhead];
        readhead = readhead+1;
        if (readhead == size) readhead=0;
        return kIOReturnSuccess;
    }

    IOReturn pop(TYPE *objects, UInt32 num) override{
        if (!buffer) {
            return kIOReturnNotReady;
        }
        isPopped=true;
        if (num > available()) { return kIOReturnUnderrun; }
        
        for (UInt32 n = 0; n < num ; n++) {
            objects[n] = buffer[readhead++];
            if (readhead==size) { readhead = 0; }
        }
        return kIOReturnSuccess;
    }
    
    void notifyWrap(AbsoluteTime time) override {
        // default: do nothing
    }
    
    UInt32 available() override{
        if (!buffer) {
            return 0;
        }

        // +SIZE because % does not properly handle negative
        UInt32 avail = (size + writehead - readhead ) % size;
        return avail;
    }
    
    UInt32 vacant() override{
        if (!buffer) {
            return 0;
        }

        // +2*SIZE because % does not properly handle negative
        UInt32 vacant =  (2*size + readhead - writehead - 1 ) % size;
        return vacant;
        
    }

    IOReturn seek(UInt32 position) override {
        if (position>= size) {
            return kIOReturnBadArgument;
        }
        if (readhead != position) {
            readhead=position;
            return kIOReturnUnderrun;
        }
        return kIOReturnSuccess;
    }

    UInt32 currentWritePosition() override {
        return writehead;
    }

};


// HACK move to better place?
/*! Ring to store recent frame sizes, to sync write to read speed */
typedef RingBufferDefault<UInt32> FrameSizeQueue;


#endif
