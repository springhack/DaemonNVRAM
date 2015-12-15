/* add your code here */

#include "DaemonNVRAM.hpp"
#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <libkern/version.h>
#include <IOKit/IOUserClient.h>
#include <libkern/c++/OSUnserialize.h>

#include <IOKit/IONVRAM.h>
#include <IOKit/IOLib.h>

#define kIOPMPowerOff 0
#define POWER_STATE_OFF     0
#define POWER_STATE_ON      1
static IOPMPowerState sPowerStates[] = {
    {1, kIOPMPowerOff, kIOPMPowerOff, kIOPMPowerOff, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOPMPowerOn,  kIOPMPowerOn,  kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};

#define super IOService

OSDefineMetaClassAndStructors(DaemonNVRAM, IOService)

bool DaemonNVRAM::init(OSDictionary *dict)
{
    bool res = super::init(dict);
    IOLog("DaemonNVRAM::init SpringHack\n");
    return res;
}

bool DaemonNVRAM::start(IOService *provider)
{
    bool res = super::start(provider);
    IOLog("DaemonNVRAM::start\n");
    
    // Register Power modes
    PMinit();
    registerPowerDriver(this, sPowerStates, sizeof(sPowerStates)/sizeof(IOPMPowerState));
    provider->joinPMtree(this);
    
    // Create the command gate.
    mCommandGate = IOCommandGate::commandGate(this, dispatchCommand);
    getWorkLoop()->addEventSource(mCommandGate);
    
    // Create a timer task
    mTimer = IOTimerEventSource::timerEventSource(this, timeoutOccurred);
    
    if(mTimer)
    {
        getWorkLoop()->addEventSource(mTimer);
        mTimer->setTimeoutMS(50); // callback isn't being setup right, causes a panic
    }
    
    // We should be root right now... cache this for later.
    mCtx = vfs_context_current();
    
    registerService();
    
    return res;
}


void DaemonNVRAM::sync(void)
{
    mCommandGate->runCommand( ( void * ) kNVRAMSyncCommand, NULL, NULL, NULL );
}

void DaemonNVRAM::doSync(void)
{
    
    OSCollectionIterator *iter = NULL;
    OSDictionary *inputDict = NULL;
    
    //Find root
    IORegistryEntry* nvram = IORegistryEntry::fromPath("/chosen/nvram", gIODTPlane);
    if (!nvram)
    {
        IOLog("DaemonNVRAM: no /chosen/nvram, trying IODTNVRAM\n");
        if (OSDictionary* matching = serviceMatching("IODTNVRAM"))
        {
            nvram = waitForMatchingService(matching, 1000000000ULL * 15);
            matching->release();
        }
    }
    
    if (nvram)
    {
        if (OSSerialize* serial = OSSerialize::withCapacity(0))
        {
            nvram->serializeProperties(serial);
            if (inputDict = OSDynamicCast(OSDictionary, OSUnserializeXML(serial->text())))
            {
                iter = OSCollectionIterator::withCollection(inputDict);
            }
        }
    }
    
    //create the output Dictionary
    OSDictionary * outputDict = OSDictionary::withCapacity(1);
    
    if (iter == 0)
    {
        IOLog("DaemonNVRAM:FAILURE!. No iterator on input dictionary (myself)\n");
        return;
    }
    
    OSSymbol * key = NULL;
    OSObject * value = NULL;
    
    while( (key = OSDynamicCast(OSSymbol,iter->getNextObject())))
    {
        
        //just get the value now anyway
        value = inputDict->getObject(key);
        
        //if the key conmektains :, look to see if it's in the map already, cause we'll add a child pair to it
        //otherwise we just slam the key/val pair in
        
        const char * keyChar = key->getCStringNoCopy();
        const char * guidValueStr = NULL;
        if( ( guidValueStr = strstr(keyChar , NVRAM_SEPERATOR)) != NULL)
        {
            //we have a GUID child to deal with
            //now substring out the GUID cause thats going to be a DICT itself on the new outputDict
            //guidValueStr points to the :
            size_t guidCutOff = guidValueStr - keyChar;
            
            //allocate buffer
            //we ar ereally accounting for + sizeof('\0')
            //thats always 1. so 1.
            char guidStr[guidCutOff+1];
            strlcpy(guidStr, keyChar, guidCutOff+1);
            
            //in theory we have a guid and a value
            //LOG("sync() -> Located GUIDStr as %s\n",guidStr);
            
            //check for ?OSDictionary? from the dictionary
            OSDictionary * guidDict = OSDynamicCast(OSDictionary, outputDict->getObject(guidStr));
            if(!guidDict)
            {
                guidDict = OSDictionary::withCapacity(1);
                outputDict->setObject(guidStr,guidDict);
            }
            
            //now we have a dict for the guid no matter what (mapping GUID | DICT)
            guidDict->setObject(OSString::withCString(guidValueStr+strlen(NVRAM_SEPERATOR)), value);
        }
        else
        {
            //we are boring.
            outputDict->setObject(key,value);
        }
    }//end while
    
    //serialize and write this out
    OSSerialize *s = OSSerialize::withCapacity(10000);
    s->addString(NVRAM_FILE_HEADER);
    outputDict->serialize(s);
    s->addString(NVRAM_FILE_FOOTER);
    
    
    int error =	write_buffer("/Extra/NVRAM/nvram.plist", s->text(), s->getLength(), mCtx);
    if(error)
    {
        IOLog("DaemonNVRAM:Unable to write to %s, errno %d\n", "/Extra/NVRAM/nvram.plist", error);
    }
    
    //now free the dictionaries && iter
    iter->release();
    outputDict->release();
    s->release();
    
}

void DaemonNVRAM::free(void)
{
    IOLog("DaemonNVRAM::free\n");
    super::free();
}

IOService* DaemonNVRAM::probe(IOService* provider, SInt32* score)
{
    IOService *res = super::probe(provider, score);
    IOLog("DaemonNVRAM::probe\n");
    return res;
}

IOReturn DaemonNVRAM::dispatchCommand(OSObject* owner, void* arg0, void* arg1, void* arg2, void* arg3 )
{
    DaemonNVRAM* self = OSDynamicCast(DaemonNVRAM, owner);
    if(!self)
        return kIOReturnBadArgument;
    
    size_t command = (size_t) arg0;
    switch (command)
    {
        case kNVRAMSyncCommand:
            self->doSync();
            break;
            
        default:
            break;
    }
    
    return kIOReturnSuccess;
}

void DaemonNVRAM::timeoutOccurred(OSObject *target, IOTimerEventSource* timer)
{
    DaemonNVRAM* self = OSDynamicCast(DaemonNVRAM, target);
    if(!self)
        IOLog("DaemonNVRAM: Failed sync to file /Extra/NVRAM/nvram.plist!\n");
    else {
        IOLog("DaemonNVRAM: sync to file /Extra/NVRAM/nvram.plist\n");
        self->sync();
    }
    timer->setTimeoutMS(300*1000); //300s == 5 mins
}

void DaemonNVRAM::stop(IOService *provider)
{
    IOLog("DaemonNVRAM::stop\n");
    
    sync();

    if(mCommandGate)
    {
        getWorkLoop()->removeEventSource(mCommandGate);
    }
    
    if(mTimer)
    {
        mTimer->cancelTimeout();
        getWorkLoop()->removeEventSource(mTimer);
        OSSafeReleaseNULL(mTimer);
    }
    
    PMstop();
    
    super::stop(provider);
}