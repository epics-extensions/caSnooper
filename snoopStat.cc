// CaSnooper Statistics

// Based on gateStat class for the Gateway

// snoopStat: Contains data and CAS interface for one bit of snoop
// status info.  Update is done via a snoop server's method (setStat)
// calling snoopStat::post_data.

// serv       points to the parent snoop server of this status bit
// post_data  is a flag that switches posting updates on/off

#define DEBUG_UMR 0

#include "gdd.h"
#include "gddApps.h"
#include "snoopServer.h"
#include "snoopStat.h"

snoopStat::snoopStat(snoopServer* s,const char* n,int t) :
    casPV(*s),type(t),serv(s),post_data(0),name(strDup(n))
{
    gddApplicationTypeTable& tt = gddApplicationTypeTable::AppTable();
    
  // Define the value gdd;
    int appValue=tt.getApplicationType("value");
    value=new gdd(appValue,aitEnumFloat64);
    if(value)
      value->put((aitFloat64)*serv->getStatTable(type)->initValue);
    value->setTimeStamp(timeSpec());
#if DEBUG_UMR
    fflush(stderr);
    printf("snoopStat::snoopStat: name=%s\n",name);
    fflush(stdout);
    value->dump();
    fflush(stderr);
#endif
    
  // Define the attributes gdd
    attr=new gdd(appValue,aitEnumFloat64);
    attr = gddApplicationTypeTable::AppTable().getDD(gddAppType_attributes);
    if(attr) {
	attr[gddAppTypeIndex_attributes_units].
	  put(serv->getStatTable(type)->units);
	attr[gddAppTypeIndex_attributes_maxElements]=1;
	attr[gddAppTypeIndex_attributes_precision]=
	  serv->getStatTable(type)->precision;
	attr[gddAppTypeIndex_attributes_graphicLow]=0.0;
	attr[gddAppTypeIndex_attributes_graphicHigh]=0.0;
	attr[gddAppTypeIndex_attributes_controlLow]=0.0;
	attr[gddAppTypeIndex_attributes_controlHigh]=0.0;
	attr[gddAppTypeIndex_attributes_alarmLow]=0.0;
	attr[gddAppTypeIndex_attributes_alarmHigh]=0.0;
	attr[gddAppTypeIndex_attributes_alarmLowWarning]=0.0;
	attr[gddAppTypeIndex_attributes_alarmHighWarning]=0.0;
	attr->setTimeStamp(timeSpec());
    }
}

snoopStat::~snoopStat(void)
{
    serv->clearStat(type);
    if(value) value->unreference();
    if(attr) attr->unreference();
    if(name) delete [] name;
}

const char* snoopStat::getName() const
{
    return name; 
}

caStatus snoopStat::interestRegister(void)
{
    post_data=1;
    return S_casApp_success;
}

void snoopStat::interestDelete(void) { post_data=0; }
unsigned snoopStat::maxSimultAsyncOps(void) const { return 5000u; }

aitEnum snoopStat::bestExternalType(void) const
{
    return aitEnumFloat64;
}

caStatus snoopStat::write(const casCtx & /*ctx*/, gdd & /*dd*/)
{
    return S_casApp_noSupport;
}

caStatus snoopStat::read(const casCtx & /*ctx*/, gdd &dd)
{
    static const aitString str = "Snoopway Statistics PV";
    gddApplicationTypeTable& table=gddApplicationTypeTable::AppTable();
    
  // Branch on application type
    unsigned at=dd.applicationType();
    switch(at) {
#if 0
      // KE: These are not implemented in the released version of base
    case gddAppType_ackt:
    case gddAppType_acks:
    case gddAppType_dbr_stsack_string:
	fprintf(stderr,"%s snoopStat::read: "
	  "Got unsupported app type %d for %s\n",
	  timeStamp(),
	  at,name?name:"Unknown Stat PV");
	fflush(stderr);
	return S_casApp_noSupport;
	break;
    case gddAppType_className:
	dd.put(str);
	return S_casApp_success;
	break;
#endif
    default:
      // Copy the current state
	if(attr) table.smartCopy(&dd,attr);
	if(value) table.smartCopy(&dd,value);
	return S_casApp_success;
    }
#if DEBUG_UMR
    fflush(stderr);
    printf("snoopStat::read: name=%s\n",name);
    fflush(stdout);
    dd.dump();
    fflush(stderr);
#endif
}

void snoopStat::postData(long val)
{
    value->put((aitFloat64)val);
    value->setTimeStamp(timeSpec());
    if(post_data) postEvent(serv->getSelectMask(),*value);
#if DEBUG_UMR
    fflush(stderr);
    printf("snoopStat::postData(long): name=%s\n",name);
    fflush(stdout);
    value->dump();
    fflush(stderr);
#endif
}

// KE: Could have these next two just call postData((aitFloat64)val)

void snoopStat::postData(unsigned long val)
{
    value->put((aitFloat64)val);
    value->setTimeStamp(timeSpec());
    if(post_data) postEvent(serv->getSelectMask(),*value);
#if DEBUG_UMR
    fflush(stderr);
    printf("snoopStat::postData(unsigned long): name=%s\n",name);
    fflush(stdout);
    value->dump();
    fflush(stderr);
#endif
}

void snoopStat::postData(double val)
{
    value->put(val);
    value->setTimeStamp(timeSpec());
    if(post_data) postEvent(serv->getSelectMask(),*value);
#if DEBUG_UMR
    fflush(stderr);
    printf("snoopStat::postData(double): name=%s\n",name);
    fflush(stdout);
    value->dump();
    fflush(stderr);
#endif
}
