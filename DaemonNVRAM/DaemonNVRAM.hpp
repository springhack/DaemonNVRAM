/* add your code here */

#include <sys/proc.h>

#include <IOKit/IOService.h>
#include <IOKit/IONVRAM.h>
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/IOService.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOTimerEventSource.h>

#include "FileIO.h"

#define private public

#define kNVRAMSyncCommand   1
#define kNVRAMSetProperty   2
#define kNVRAMGetProperty   4

#define NVRAM_SEPERATOR         ":"
#define NVRAM_FILE_HEADER		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"\
"\t<plist version=\"1.0\">\n"
#define NVRAM_FILE_FOOTER       "</plist>\n"
#define NVRAM_MISS_KEY			"NVRAM_MISS"
#define NVRAM_MISS_HEADER       "\n<key>NVRAM_MISS</key>\n"

class DaemonNVRAM : public IOService {
    
    //一个宏定义，会自动生成该类的构造方法、析构方法和运行时
    OSDeclareDefaultStructors(DaemonNVRAM)
    
public:
    
    //该方法与Cocoa中的init和C++中的构造方法类似
    virtual bool init(OSDictionary* dictionary = NULL);
    //该方法与Cocoa中的dealloc和C++中的析构方法类似
    virtual void free(void);
    
    virtual IOService* probe(IOService* provider, SInt32* score);
    virtual bool start(IOService *provider);
    virtual void stop(IOService *provider);
    virtual void sync(void);
    virtual void doSync(void);
private:
    
    IOCommandGate* mCommandGate;
    vfs_context_t mCtx;
    IOTimerEventSource* mTimer;
    char* buffer;
    
    static IOReturn dispatchCommand( OSObject* owner, void* arg0, void* arg1, void* arg2, void* arg3);
    static void timeoutOccurred(OSObject *target, IOTimerEventSource* timer);
};

static inline const char * strstr(const char *s, const char *find)
{
    char c, sc;
    size_t len;
    
    if ((c = *find++) != 0) {
        len = strlen(find);
        do {
            do {
                if ((sc = *s++) == 0)
                    return (NULL);
            } while (sc != c);
        } while (strncmp(s, find, len) != 0);
        s--;
    }
    return s;
}
