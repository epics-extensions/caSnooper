// CaSnooper Statistics

// Based on gateStat class for the Gateway

#ifndef SNOOP_STAT_H
#define SNOOP_STAT_H

#include "aitTypes.h"

class gdd;
class snoopServer;

class snoopStat : public casPV
{
  public:
    snoopStat(snoopServer* serv,const char* n, int t);
    virtual ~snoopStat(void);
    
  // CA server interface functions
    virtual caStatus interestRegister(void);
    virtual void interestDelete(void);
    virtual aitEnum bestExternalType(void) const;
    virtual caStatus read(const casCtx &ctx, gdd &prototype);
    virtual caStatus write(const casCtx &ctx, gdd &value);
    virtual unsigned maxSimultAsyncOps(void) const;
    virtual const char *getName() const;
    
    void postData(long val);
    void postData(unsigned long val);
    void postData(double val);
    
  private:
    gdd* value;
    gdd *attr;
    int post_data;
    int type;
    snoopServer* serv;
    char* name;
};

#endif
