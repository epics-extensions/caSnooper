// CaSnooper: Server that logs broadcasts

#include "snoopServer.h"
#include "gddApps.h"
#include <math.h>
#include <cadef.h>

#define DEBUG_SORT 0
#define DEBUG_HASH 0

// server.h is private and not in base/include.  We are including it
// so we can access casCtx, which is not normally possible (by intent).
#  define HOST_NAMESIZE 256
#  include "server.h"
inline casCoreClient * casCtx::getClient() const 
{
    return this->pClient;
}

// Static initializations
unsigned long dataNode::nodeCount=0;
snoopData *dataNode::dataArray=(snoopData *)0;

char connTable[][10]={
    "Never",
    "Previous",
    "Connected",
    "Closed",
};

////////////////////////////////////////////////////////////////////////
//                   snoopServer
////////////////////////////////////////////////////////////////////////

// snoopServer::snoopServer() //////////////////////////////////////////
snoopServer::snoopServer(int nCheckIn, int nPrintIn, int nSigmaIn,
  double nLimitIn) :
    enabled(0),
    processTime(0.0),
    nCheck(nCheckIn),
    nPrint(nPrintIn),
    nSigma(nSigmaIn),
    nLimit(nLimitIn),
    dataArray((snoopData *)0)
{
    pvList.init(2048u);
}

// snoopServer::~snoopServer ///////////////////////////////////////////
snoopServer::~snoopServer(void)
{
  // Clear the pvList
    pvList.destroyAllEntries();
}

// snoopServer::pvExistTest ////////////////////////////////////////////
pvExistReturn snoopServer::pvExistTest(const casCtx& ctxIn, const char *pPvName)
{
    casClient *pClient;
    pClient=(casClient *)ctxIn.getClient();
    char hostName[HOST_NAMESIZE]="";
    char name[NAMESIZE]="";
    dataNode *node=(dataNode *)0;
    int i=0,len;

  // Return if data taking is not enabled
    if(!enabled) return pverDoesNotExistHere;

  // Make the hash name
    pClient->clientHostName(hostName,HOST_NAMESIZE);
    strcpy(name,pPvName);
    len=strlen(name);
    name[len++]=DELIMITER;
    strncpy(name+len,hostName,NAMESIZE-len);
    name[NAMESIZE-1]='\0';
    stringId id(name,stringId::refString);

  // See if we have it
    node=pvList.lookup(id);
    if(node) {
	node->getData()->incrCount();
    } else {
	snoopData *sData = new snoopData(name);
	if(sData) {
	    node = new dataNode(name,*sData,pvList);
	    if(node) {
		node->getData()->incrCount();
		pvList.add(*node);
	    } else {
		delete sData;
		errMsg("Unable to create node for %s\n",name);
	    }
	}
    }

    return pverDoesNotExistHere;
}

// snoopServer::createPV ///////////////////////////////////////////////
pvCreateReturn snoopServer::createPV(const casCtx &ctx, const char *pPvName)
{
    UNREFERENCED(ctx);
    UNREFERENCED(pPvName);

    return S_casApp_pvNotFound;
}

// snoopServer::makeArray //////////////////////////////////////////////
// Make an array of the hash table.
int snoopServer::makeArray(unsigned long *nVals)
{
  // Return if already allocated
    if(dataArray) return SS_ERROR;

  // Find out how many nodes
    dataNode::setNodeCount(0u);
    pvList.traverse(dataNode::addToNodeCount);
    *nVals=dataNode::getNodeCount();
#if DEBUG_HASH
    printf("nVals=%lu\n",*nVals);
#endif

    dataArray=new snoopData[*nVals];
    if(!dataArray) {
	*nVals=0;
	return SS_ERROR;
    }

  // Fill the dataArray
    dataNode::setNodeCount(0u);
    dataNode::setDataArray(dataArray);
    pvList.traverse(dataNode::addToDataArray);
    *nVals=dataNode::getNodeCount();

#if DEBUG_HASH
    printf("nVals=%lu\n",*nVals);
    for(i=0; i < *nVals; i++) {
	printf("%3d %s\n",i,dataArray[i].getName());
    }
#endif

    return SS_OK;
}

// snoopServer::report /////////////////////////////////////////////////
void snoopServer::report(void)
{
    chid *pChid=(chid *)0;
    unsigned long nNodes=0,nNodes1,nRequestsTotal,nRequests;
    unsigned long *index=0,ii;
    double x;
    double max=0.0;
    double sum=0.0;
    double sumsq=0.0;
    double sigma, nSigmaVal, avg;
    unsigned long i,j;
    int status;
    char name[NAMESIZE];
    char *ptr;

  // Check time
    if(processTime <= 0.0) {
	printf("\nThe time interval is zero\n");
	return;
    }

  // Make an array out of the hash table
    status=makeArray(&nNodes);
    if(status != SS_OK) {
	errMsg("snoopServer::report: Cannot make data array");
	return;
    }
    if(status != SS_OK) {
	printf("\nThere is no data to report\n");
	return;
    }

  // Loop over the nodes to get data for statistics
    nRequests=0;
    for(i=0; i < nNodes; i++) {
	nRequests=dataArray[i].getCount();
	nRequestsTotal+=nRequests;
	x=(double)nRequests;
	sum+=x;
	sumsq+=(x*x);
	if(x > max) max=x;
    }

  // Calculate statistics
    if(nNodes == 0) {
	avg=0;
	sigma=0;
    } else if (nNodes == 1) {
	avg=sum;
	sigma=0;
    } else {
	avg=sum/(double)nNodes;
	sigma=sqrt((sumsq-avg*avg)/(nNodes-1));
    }
    printf("\nThere were %ld requests to check for PV existence "
      "for %ld different PVs.\n",
      nRequestsTotal,nNodes);
    printf("  Max(Hz):   %.2f\n",max/processTime);
    printf("  Mean(Hz):  %.2f\n",avg/processTime);
    printf("  StDev(Hz): %.2f\n",sigma/processTime);


  // Print the n sigma values
    if(nSigma > 0) {
	nSigmaVal=nSigma*sigma;
	printf("\nThe following were above the %d-sigma value (%.2f):\n",
	  nSigma,nSigmaVal);
	max=sum=sumsq=0.0;
	nRequestsTotal=nNodes1=0;
	for(i=j=0; i < nNodes; i++) {
	    nRequests=dataArray[i].getCount();
	    if((double)nRequests > nSigmaVal) {
		strcpy(name,dataArray[i].getName());
		ptr=strchr(name,DELIMITER);
		if(ptr) *ptr='\0';
		else ptr=name;
		printf("%4ld %-20s %-33s %.2f\n",++j,ptr+1,name,
		  dataArray[i].getCount()/processTime);
	    } else {
		nNodes1++;
		nRequestsTotal+=nRequests;
		x=(double)nRequests;
		sum+=x;
		sumsq+=(x*x);
		if(x > max) max=x;
	    }
	}
	
      // Calculate statistics without outliers
	if(nNodes1 == 0) {
	    avg=0;
	    sigma=0;
	} else if (nNodes1 == 1) {
	    avg=sum;
	    sigma=0;
	} else {
	    avg=sum/(double)nNodes1;
	    sigma=sqrt((sumsq-avg*avg)/(nNodes1-1));
	}
	printf("\nThere were %ld requests for %ld PVs excluding these.\n",
	  nRequestsTotal,nNodes1);
	printf("  Max(Hz):   %.2f\n",max/processTime);
	printf("  Mean(Hz):  %.2f\n",avg/processTime);
	printf("  StDev(Hz): %.2f\n",sigma/processTime);
    }

  // Print the top PVs
    int nPrint0;
    if(nPrint == 0) nPrint0 = nNodes;
    else nPrint0=nPrint;
    if(nPrint0 > nNodes) nPrint = nNodes;
    if(nPrint0 > 0) {
	if(!index) {
	    status=sortArray(&index,nNodes);
	    if(status != SS_OK || !index) {
		if(index) {
		    delete [] index;
		    index=0;
		}
		errMsg("snoopServer::report: Cannot sort data");
		goto CLEANUP;
	    }
	}
	printf("\nPVs with top %d requests:\n",nPrint0);
	for(i=0; i < nPrint0; i++) {
	    ii=index[i];
	    strcpy(name,dataArray[ii].getName());
	    ptr=strchr(name,DELIMITER);
	    if(ptr) *ptr='\0';
	    else ptr=name;
	    printf("%4ld %-20s %-33s %.2f\n",i+1,ptr+1,name,
	      dataArray[ii].getCount()/processTime);
	}
    }

  // Print the PVs over the limit
    if(nLimit > 0.0) {
	if(!index) {
	    status=sortArray(&index,nNodes);
	    if(status != SS_OK || !index) {
		if(index) {
		    delete [] index;
		    index=0;
		}
		errMsg("snoopServer::report: Cannot sort data");
		goto CLEANUP;
	    }
	}
	printf("\nPVs over %.2f Hz\n",nLimit);
	for(i=0; i < nNodes; i++) {
	    ii=index[i];
	    if(dataArray[ii].getCount()/processTime < nLimit) break;
	    strcpy(name,dataArray[ii].getName());
	    ptr=strchr(name,DELIMITER);
	    if(ptr) *ptr='\0';
	    else ptr=name;
	    printf("%4ld %-20s %-33s %.2f\n",i+1,ptr+1,name,
	      dataArray[ii].getCount()/processTime);
	}
    }

  // Check PV existence
    int nCheck0;
    if(nCheck == 0) nCheck0 = nNodes;
    else nCheck0=nCheck;
    if(nCheck0 > nNodes) nCheck = nNodes;
    if(nCheck0 > 0) {
	if(!index) {
	    status=sortArray(&index,nNodes);
	    if(status != SS_OK || !index) {
		if(index) {
		    delete [] index;
		    index=0;
		}
		errMsg("snoopServer::report: Cannot sort data");
		goto CLEANUP;
	    }
	}
	printf("\nConnection status for top %d PVs after %.2f sec:\n",
	  nCheck0,CA_PEND_IO_TIME);

      // Allocate chids
	pChid = new chid[nCheck0];
	if(!pChid) {
	    errMsg("snoopServer::report: Cannot allocate space for CA");
	    goto CLEANUP;
	}

      // Initialize CA
	status=ca_task_initialize();
	if(status != ECA_NORMAL) {
	    errMsg("snoopServer::report: "
	      "ca_task_initialize failed:\n%s",ca_message(status));
	    goto CLEANUP;
	}

      // Search
	for(i=0; i < nCheck0; i++) {
	    ii=index[i];
	    strcpy(name,dataArray[ii].getName());
	    ptr=strchr(name,DELIMITER);
	    if(ptr) *ptr='\0';
	    else ptr=name;
	    status=ca_search(name,&pChid[i]);
	    if(status != ECA_NORMAL) {
		errMsg("snoopServer::report: [%d %s] "
		  " ca_search failed:\n%s",ii,dataArray[ii].getName(),
		  ca_message(status));
	    }
	}

      // Wait
	ca_pend_io(CA_PEND_IO_TIME);
	
      // Close CA
	status=ca_task_exit();
	if(status != ECA_NORMAL) {
	    errMsg("snoopServer::report: "
	      "ca_task_exit failed:\n%s",ca_message(status));
	}

      // Print results
	for(i=0; i < nCheck0; i++) {
	    ii=index[i];
	    strcpy(name,dataArray[ii].getName());
	    ptr=strchr(name,DELIMITER);
	    if(ptr) *ptr='\0';
	    else ptr=name;
	    printf("%4ld %-20s %-33s %-9s %.2f\n",i+1,ptr+1,name,
	      connTable[ca_state(pChid[i])],
	      dataArray[ii].getCount()/processTime);
	}
    }

  CLEANUP:
    if(dataArray) {
	delete [] dataArray;
	dataArray=(snoopData *)0;
	dataNode::setDataArray((snoopData *)0);
    }
    if(index) {
	delete [] index;
    }
    if(pChid) {
	delete [] pChid;
    }
}

// snoopServer::show ///////////////////////////////////////////////////
void snoopServer::show(unsigned level) const
{
  //
  // server tool specific show code goes here
  //
    
  //
  // print information about ca server libarary
  // internals
  //
  //    printf("caServer:\n");
    this->caServer::show(level);
}

// snoopServer::sortArray //////////////////////////////////////////////
int snoopServer::sortArray(unsigned long **index, unsigned long nVals)
{
    int rc=SS_OK;
    unsigned long i;

  // Check if the index has already been made
    if(*index) return SS_OK;
    if(nVals <= 0) return SS_ERROR;
    if(!dataArray) return SS_ERROR;

  // Allocate space
    *index = new unsigned long[nVals];
    double *vals = new double[nVals];
    if(!*index) {
	errMsg("snoopServer::sortArray: Cannot allocate space for index");
	rc=SS_ERROR;
	goto ERROR;
    }
    if(!vals) {
	errMsg("snoopServer::sortArray: Cannot allocate space for values");
	rc=SS_ERROR;
	goto ERROR;
    }

  // Fill the vals array
    for(i=0; i < nVals; i++) {
	vals[i]=dataArray[i].getCount();
    }

  // Sort
    hsort(vals,*index,nVals);

#if DEBUG_SORT
    printf("\nDebug sort\n");
    for(i=0; i < nVals; i++) {
	printf("%3lu %.2f -> %3lu %.2f\n",
	  i,vals[i],(*index)[i],vals[(*index)[i]]);
    } 
#endif

    goto CLEAN;

  ERROR:
    if(*index) {
	delete *index;
	*index=0;
    }

  CLEAN:
    if(vals) delete [] vals;

    return rc;
}



////////////////////////////////////////////////////////////////////////
//                   snoopData
////////////////////////////////////////////////////////////////////////

// snoopData::snoopData ////////////////////////////////////////////////
snoopData::snoopData() :
    count(0)
{
    *name='\0';
}

snoopData::snoopData(const char *nameIn) :
    count(0)
{
    strcpy(name,nameIn);
}

// Copy constructor
snoopData::snoopData(const snoopData &snoopDataIn)
{
    count=snoopDataIn.getCount();
    strcpy(name,snoopDataIn.getName());
}

snoopData &snoopData::operator=(const snoopData &snoopDataIn)
{
    if(this != &snoopDataIn) {
	count=snoopDataIn.getCount();
	strcpy(name,snoopDataIn.getName());
    }
    return *this;
}
