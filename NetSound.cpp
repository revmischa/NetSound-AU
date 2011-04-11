/*

TESTING
*	File:		NetSound.cpp
*	
*	Version:	1.0
* 
*	Created:	4/10/11
*	
*	Copyright:  Copyright © 2011 int80, All Rights Reserved
* 
*	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc. ("Apple") in 
*				consideration of your agreement to the following terms, and your use, installation, modification 
*				or redistribution of this Apple software constitutes acceptance of these terms.  If you do 
*				not agree with these terms, please do not use, install, modify or redistribute this Apple 
*				software.
*
*				In consideration of your agreement to abide by the following terms, and subject to these terms, 
*				Apple grants you a personal, non-exclusive license, under Apple's copyrights in this 
*				original Apple software (the "Apple Software"), to use, reproduce, modify and redistribute the 
*				Apple Software, with or without modifications, in source and/or binary forms; provided that if you 
*				redistribute the Apple Software in its entirety and without modifications, you must retain this 
*				notice and the following text and disclaimers in all such redistributions of the Apple Software. 
*				Neither the name, trademarks, service marks or logos of Apple Computer, Inc. may be used to 
*				endorse or promote products derived from the Apple Software without specific prior written 
*				permission from Apple.  Except as expressly stated in this notice, no other rights or 
*				licenses, express or implied, are granted by Apple herein, including but not limited to any 
*				patent rights that may be infringed by your derivative works or by other works in which the 
*				Apple Software may be incorporated.
*
*				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO WARRANTIES, EXPRESS OR 
*				IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY 
*				AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE 
*				OR IN COMBINATION WITH YOUR PRODUCTS.
*
*				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL 
*				DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
*				OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, 
*				REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER 
*				UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN 
*				IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#include "NetSound.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

COMPONENT_ENTRY(NetSound)


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	NetSound::NetSound
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NetSound::NetSound(AudioUnit component)
	: AUEffectBase(component)
{
	CreateElements();
	Globals()->UseIndexedParameters(kNumberOfParameters);
	SetParameter(kParam_One, kDefaultValue_ParamOne );
        
#if AU_DEBUG_DISPATCHER
	mDebugDispatcher = new AUDebugDispatcher (this);
#endif
    
    CreateBuffer();
	OpenSocket();
}

void NetSound::CreateBuffer() {
    inputBuffer = (u_int8_t *)malloc(GetBufferSize());
    audioBuffer = new RingBuffer(inputBuffer, GetBufferSize());
}

void NetSound::OpenSocket() {
    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        // error creating socket
        perror("NetSound: error creating socket");
        return;
    }
    
    memset(&listenAddr, 0, sizeof(listenAddr));
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_port = htons(1488);
    listenAddr.sin_addr.s_addr = INADDR_ANY;
    
    fcntl(sock, F_SETFL, O_NONBLOCK);
    
    if (bind(sock, (struct sockaddr *)&listenAddr, sizeof(listenAddr)) < 0) {
        perror("NetSound: error binding");
        close(sock);
    }    
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	NetSound::GetParameterValueStrings
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus			NetSound::GetParameterValueStrings(AudioUnitScope		inScope,
                                                                AudioUnitParameterID	inParameterID,
                                                                CFArrayRef *		outStrings)
{
        
    return kAudioUnitErr_InvalidProperty;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	NetSound::GetParameterInfo
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus			NetSound::GetParameterInfo(AudioUnitScope		inScope,
                                                        AudioUnitParameterID	inParameterID,
                                                        AudioUnitParameterInfo	&outParameterInfo )
{
	OSStatus result = noErr;

	outParameterInfo.flags = 	kAudioUnitParameterFlag_IsWritable
						|		kAudioUnitParameterFlag_IsReadable;
    
    if (inScope == kAudioUnitScope_Global) {
        switch(inParameterID)
        {
            case kParam_One:
                AUBase::FillInParameterName (outParameterInfo, kParameterOneName, false);
                outParameterInfo.unit = kAudioUnitParameterUnit_LinearGain;
                outParameterInfo.minValue = 0.0;
                outParameterInfo.maxValue = 1;
                outParameterInfo.defaultValue = kDefaultValue_ParamOne;
                break;
            default:
                result = kAudioUnitErr_InvalidParameter;
                break;
            }
	} else {
        result = kAudioUnitErr_InvalidParameter;
    }
    


	return result;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	NetSound::GetPropertyInfo
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus			NetSound::GetPropertyInfo (AudioUnitPropertyID	inID,
                                                        AudioUnitScope		inScope,
                                                        AudioUnitElement	inElement,
                                                        UInt32 &		outDataSize,
                                                        Boolean &		outWritable)
{
	if (inScope == kAudioUnitScope_Global) 
	{
		switch (inID) 
		{
			case kAudioUnitProperty_CocoaUI:
				outWritable = false;
				outDataSize = sizeof (AudioUnitCocoaViewInfo);
				return noErr;
					
		}
	}

	return AUEffectBase::GetPropertyInfo (inID, inScope, inElement, outDataSize, outWritable);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	NetSound::GetProperty
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus			NetSound::GetProperty(	AudioUnitPropertyID inID,
															AudioUnitScope 		inScope,
															AudioUnitElement 	inElement,
															void *				outData )
{
	if (inScope == kAudioUnitScope_Global) 
	{
		switch (inID) 
		{
			case kAudioUnitProperty_CocoaUI:
			{
				// Look for a resource in the main bundle by name and type.
				CFBundleRef bundle = CFBundleGetBundleWithIdentifier( CFSTR("com.audiounit.NetSound") );
				
				if (bundle == NULL) return fnfErr;
                
				CFURLRef bundleURL = CFBundleCopyResourceURL( bundle, 
                    CFSTR("NetSound_CocoaViewFactory"), 
                    CFSTR("bundle"), 
                    NULL);
                
                if (bundleURL == NULL) return fnfErr;

				AudioUnitCocoaViewInfo cocoaInfo;
				cocoaInfo.mCocoaAUViewBundleLocation = bundleURL;
				cocoaInfo.mCocoaAUViewClass[0] = CFStringCreateWithCString(NULL, "NetSound_CocoaViewFactory", kCFStringEncodingUTF8);
				
				*((AudioUnitCocoaViewInfo *)outData) = cocoaInfo;
				
				return noErr;
			}
		}
	}

	return AUEffectBase::GetProperty (inID, inScope, inElement, outData);
}

UInt32 NetSound::GetBufferSize() {
    return GetSampleSize() * 44100 * 1; // 1 second
}

UInt32 NetSound::GetSampleSize() {
    return 2; // 16-bit
}



#pragma mark ____NetSoundEffectKernel


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	NetSound::NetSoundKernel::Reset()
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void		NetSound::NetSoundKernel::Reset()
{
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	NetSound::NetSoundKernel::Process
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void NetSound::NetSoundKernel::Process(	const Float32 	*inSourceP,
                                                    Float32		 	*inDestP,
                                                    UInt32 			inFramesToProcess,
                                                    UInt32			inNumChannels, // for version 2 AudioUnits inNumChannels is always 1
                                       bool			&ioSilence ) {
    UInt32 nSampleFrames = inFramesToProcess;
    const Float32 *sourceP = inSourceP;
    Float32 *destP = inDestP;
    Float32 gain = GetParameter( kParam_One );
    ssize_t read;
    socklen_t readAddressSize = sizeof(au->listenAddr);
    sockaddr_in listenAddr = au->listenAddr;
    unsigned int toReadFromNetwork, audioBufReadSize, availDataSize, bufferAdditionSize;
    SInt8 readBuf[50000];
    SInt16 sample;
    UInt8 sampleSize = au->GetSampleSize();
    
    if (au->audioBuffer->BufferedBytes() < 44100*sampleSize*2 &&
        au->sock > 0 && au->audioBuffer->BufferedBytes() < au->GetBufferSize()) {
        
        // read at most either nSampleFrames worth of samples or the max size of our buffer
        toReadFromNetwork = MIN(nSampleFrames * sampleSize, sizeof(readBuf));
        
        // buffer getting low, do a read from the network
        read = recvfrom(au->sock, readBuf, toReadFromNetwork, 0, (sockaddr *)&listenAddr, &readAddressSize);
        if (read > 0) {
            //printf("Read %d bytes! inNumChannels=%d\n", (int)read, inNumChannels);
            
            // got sound
            bufferAdditionSize = read;
            au->audioBuffer->AddData(readBuf, bufferAdditionSize);
            
            if (bufferAdditionSize < (unsigned int)read) {
                // failed to add some data
                //fprintf(stderr, "NetSound: bufferAdditionSize < read\n");
            }
        } else {
            if (errno != EAGAIN) {
                // got error
                perror("NetSound: error reading from socket");
            }
        }
    }
    
    while (nSampleFrames-- > 0) {
        Float32 inputSample = *sourceP;

        //The current (version 2) AudioUnit specification *requires* 
        //non-interleaved format for all inputs and outputs. Therefore inNumChannels is always 1
        
        sourceP += inNumChannels;  // advance to next frame (e.g. if stereo, we're advancing 2 samples);
                                   // we're only processing one of an arbitrary number of interleaved channels
        
        Float32 outputSample = inputSample * gain;
            
        // see if we have received data
        availDataSize = au->audioBuffer->BufferedBytes();
        if (availDataSize > 0) {
            //printf("Got %u bytes to process for %u sample frames\n", availDataSize, nSampleFrames);
            
            // get nSampleFrames or buffer max of samples from the buffer
            // how many sample frames do we have left to read?
            audioBufReadSize = MIN(nSampleFrames * sampleSize, sizeof(readBuf));
            
            // replace them with data we have received from the network
            au->audioBuffer->GetData(readBuf, audioBufReadSize);
            
            // actual number of samples we have is in audioBufReadSize
            for (SInt8 *networkSamplePtr = readBuf; networkSamplePtr < (readBuf + audioBufReadSize); networkSamplePtr += sampleSize) {
                sample = ntohs(*networkSamplePtr);
                //printf("%.4X ", sample);
                *destP = (Float32)(sample) / (Float32)32767;
                //printf("network sample @ %X: %d, destP: %f\n", networkSamplePtr, sample, *destP);
                
                destP += inNumChannels; // ??
                nSampleFrames--;
                //printf("nSampleFrames W: %d\n", nSampleFrames);
            }
            
            // TODO: read rest of data from ring buffer if audioBufReadSize < availDataSize
            // (happens if we reach the end of the buffer and need to read from the beginning)
        } else {
            *destP = outputSample;
            destP += inNumChannels;
            
            //printf("nSampleFrames X: %d\n", nSampleFrames);
        }
    }
}

