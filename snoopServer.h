// CaSnooper: Server that logs broadcasts

#ifndef NELEMENTS
#  define NELEMENTS(A) (sizeof(A)/sizeof(A[0]))
#endif

#define NAMESIZE 80
#define UNREFERENCED(x) (x)

#define SS_OK 0
#define SS_ERROR -1

#define DELIMITER '\t'
#define CA_PEND_IO_TIME 30.

#define DEFAULT_PREFIX "CaSnoop"
#define PREFIX_SIZE 11

#define statRequestRate  0
#define statCount 1     // Number of statistics PVs

#include <string.h>
#include <stdio.h>

#include "epicsAssert.h"
#include "casdef.h"
#include "gddAppFuncTable.h"
#include "osiTimer.h"
#include "resourceLib.h"
#include "snoopStat.h"

struct snoopServerStats
{
    char* name;
    char* pvName;
    snoopStat* pv;
    unsigned long* initValue;
    char *units;
    short precision;
};
typedef struct snoopServerStats;

// Function prototypes
int errMsg(const char *fmt, ...);
void hsort(double array[], unsigned long  indx[], unsigned long n);
char *timeStamp(void);
struct timespec *timeSpec(void);

class snoopRateStatsTimer;
class snoopData;
class dataNode;
class snoopServer;

class snoopRateStatsTimer : public osiTimer
{
  public:
    snoopRateStatsTimer(const osiTime &delay, snoopServer *m) : 
      startTime(osiTime::getCurrent()), osiTimer(delay), interval(delay), 
      serv(m) {}
    virtual void expire();
    virtual const osiTime delay() const { return interval; }
    virtual osiBool again() const { return osiTrue; }
    virtual const char *name() const { return "snoopRateStatsTimer"; }
  private:
    osiTime interval;
    osiTime startTime;
    snoopServer* serv;
};

class snoopData
{
  public:
    snoopData();
    snoopData(const char *nameIn);
    snoopData::snoopData(const snoopData &snoopDataIn);
    snoopData &operator=(const snoopData &snoopDataIn);
    const char *getName(void) const { return name; }
    void incrCount(void) { count++; }
    unsigned long getCount(void) const { return count; }
    
  private:
    unsigned long count;
    char name[NAMESIZE];
};

class dataNode : public stringId, public tsSLNode<dataNode>
{
  public:
    dataNode(const char *name, snoopData& data,
      resTable<dataNode,stringId> &listIn) :
      stringId(name),
      pData(&data),
      list(listIn) { };
    ~dataNode(void) { };
    
    snoopData* getData(void) { return pData; }
    void destroy(void)
      {
	  list.remove(*this);
	  if(pData) delete pData;
	  delete this;
      }
    
    static void setNodeCount(unsigned long nodeCountIn) { nodeCount=nodeCountIn; }
    static unsigned long getNodeCount(void) { return nodeCount; }
    void addToNodeCount(void) { nodeCount++; }

    static void setDataArray(snoopData *dataArrayIn) { dataArray=dataArrayIn; }
    static snoopData *getDataArray(void) { return dataArray; }
    void addToDataArray(void) { dataArray[nodeCount++]=*getData(); }

  private:
    dataNode(void);
    snoopData* pData;
    resTable<dataNode,stringId> &list;
    static unsigned long nodeCount;
    static snoopData *dataArray;
};

class snoopServer : public caServer
{
  public:
    snoopServer(char *prefix, int nCheckIn, int nPrintIn, int nSigmaIn,
      double nLimitIn);
    ~snoopServer(void);
    
    void enable(void) { enabled=1; }
    void disable(void) { enabled=0; }
    void report(void);
    void show(unsigned level) const;
    void setProcessTime(double processTimeIn) { processTime = processTimeIn; }
    double getprocessTime(void) const {return processTime; };
    pvExistReturn pvExistTest(const casCtx &ctx, const char *pPvName);
    pvCreateReturn createPV(const casCtx &ctx, const char *pPvName);
    
    resTable<dataNode,stringId> *getPvList(void) { return &pvList; }

    casEventMask getSelectMask(void) const { return selectMask; }
    
    snoopServerStats *getStatTable(int type) { return &statTable[type]; }
    void setStat(int type,double val);
    void setStat(int type,unsigned long val);
    void clearStat(int type);
    void initStats(char *prefix);
    unsigned long getRequestCount() const { return requestCount; }
    char *getPrefix(void) const { return statPrefix; }

  private:
    int makeArray(unsigned long *nVals);
    int sortArray(unsigned long **index, unsigned long nVals);
    
    double processTime;
    int enabled;
    int nCheck;
    int nPrint;
    int nSigma;
    double nLimit;
    snoopData *dataArray;
    resTable<dataNode,stringId> pvList;

    casEventMask selectMask;

    snoopServerStats statTable[statCount];
    int doStats;
    char *statPrefix;
    int statPrefixLength;	
    unsigned long requestCount;
};
