//
//  EMUUSBOutputStream.h
//  EMUUSBAudio
//
//  Created by Wouter Pasman on 31/12/14.
//  Copyright (c) 2014 com.emu. All rights reserved.
//

#ifndef __EMUUSBAudio__EMUUSBOutputStream__
#define __EMUUSBAudio__EMUUSBOutputStream__

#include "StreamInfo.h"
#include "RingBufferDefault.h"
#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/IOSubMemoryDescriptor.h>
#include <IOKit/IOMultiMemoryDescriptor.h>

/*! The USB Output stream handler. It pushes the data into the USB pipe.
 
 Life cycle:
 
 ( init ( start stop )* free )*
 
 See discussion in EMUUSBAudioEngine for global explanation of input and output.
 */
class EMUUSBOutputStream: public StreamInfo {
    
    
public:
    
    /*! initializes the output stream. Must be called before use.
     @param frameQueue a initialized FrameSizeQueue.
     */
    virtual IOReturn                init();
    
    /*!
     * @param frameQueue
     * @param startUsbFrame the usb frame number on which to start writing. used to sync with input stream
     * @param franeSamples the normal number of samples per frame. The max samples per frame is frameSamples+1
     */
    IOReturn                        start(FrameSizeQueue *frameQueue, UInt64 startUsbFrame,UInt32 frameSamples);
    
    /*! Stop the output stream */
    IOReturn stop();
    
    /*! frees the stream. Only to be called after stop() FINISHED (which is notified
     through the notifyClosed() call */
    virtual void free();
    
    /*!
     This is called when this EMUUSBOutputStream has closed all streams.
     */
    virtual void                    notifyClosed() = 0  ;
    
    
    UInt32							averageSampleRate;
    
    /*! Place where we were left filling a USB frame. first byte to send from mOutput.usbBufferDescriptor */
	UInt32							previouslyPreparedBufferOffset;
    
    /*! the framelist that was written last.
     Basically runs from 0 to numUSBFrameListsToQueue-1 and then
     restarts at 0. Updated after readHandler handled the block. */
    volatile UInt32					currentFrameList;
    
private:
    
    /*! Called when a queue was terminated. Counts number of terminated queues and calls handler
     when completely closed. */
    void                            queueTerminated();
    
    /*!  Write frame list (typ. 64 frames) to USB. called from writeHandler.
     Every time the frames in the list have to point to sub-memory blocks in the buffer
     */
    IOReturn                        writeFrameList (UInt32 frameListNum);
    
    /*!
     Write-completion handler. Queues another a write. This is called from clipOutputSamples
     and from the callback in PrepareWriteFrameList.
     @param target pointer to EMUUSBAudioEngine
     @param parameter
     * low 16 bits: byteoffset.
     * high 16 bits; framenumber.
     */
    static void writeCompletedStatic (void * object, void * parameter, IOReturn result, LowLatencyIsocFrame * pFrames);
    
    /*! Write-completion handler. Queues another a write. This is called from writeCompletedStatic.
     @param parameter
     * low 16 bits: byteoffset.
     * high 16 bits; framenumber.
     */
    void writeCompleted(void * parameter, IOReturn result, LowLatencyIsocFrame * pFrames);
    
    
    /*! Set up the given framelist for writing.  called from writeFrameList.
     Copies all data from audioStream into the framelists for output.
     Assumes that frameSizeQueue contains at least mOutput.numUSBFramesPerList values
     because the size of each frame must now be set.
     @param listNr the list number to be prepared.
     @return kIOReturnUnderrun if frameSizeQueue does not contain enough values.
     kIOReturnNoDevice if mOutput.audioStream==0.
     kIOReturnNoMemory if sampleBufferSize == 0*/
	IOReturn	PrepareWriteFrameList (UInt32 listNr);
    
    
    /*! an array of size [frameListnum] holding usbFrameToQueueAt for each frame when it was requested for read or write. read does not use this anymore. */
    //UInt64 *					frameQueuedForList;
    
    /*! the next USB MBus Frame number that can be used for read/write. Initially this is at frameOffset from the current frame number. Must be incremented with steps of size numUSBTimeFrames. Currently we just set it to kAppleUSBSSIsocContinuousFrame after
     the initial startup cycle. */
    //UInt64						usbFrameToQueueAt;
    
    
    /*! set to true after succesful start() */
    bool started;
    
    /*! if !=0 then we are busy stopping. Counts up till we reach RECORD_NUM_FRAMELISTS.
     If stop complete, notifyStop is called and stopped=true */
    volatile UInt32			shouldStop;
    
    
    /*! When we wrap around in the output buffer, this connects the ends for the output usb data */
	IOMultiMemoryDescriptor *			theWrapRangeDescriptor;
    /*! the two parts of a datablock that contains a wrap */
	IOSubMemoryDescriptor *				theWrapDescriptors[2];
    
    
    /*! frame size queue, holding sizes of incoming frames in the read stream */
    FrameSizeQueue *        frameSizeQueue;
    
    /*! guess: flag that is iff while we are inside the writeHandler. */
	Boolean								inWriteCompletion;
    
    /*! Variable that is set TRUE every time a wrap occurs in the writeHandler and
     that theWrapDescriptors are used.*/
	Boolean								needTimeStamps;
    
    
    
    bool                                initialized=false;
    
    /*! number of samples normally in a frame. The maximum number is one more. */
    UInt32                              stockSamplesInFrame;
    
};

#endif /* defined(__EMUUSBAudio__EMUUSBOutputStream__) */
