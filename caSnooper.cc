// CaSnooper: Server that logs broadcasts

#include "snoopServer.h"
#include "fdManager.h"

#define NCHECK_DEFAULT -1
#define NPRINT_DEFAULT -1
#define NSIGMA_DEFAULT -1
#define NLIMIT_DEFAULT 0.0

// Interval for rate statistics in seconds
#define RATE_STATS_INTERVAL 1u

#define VERSION "CaSnooper 1.1"

// Function prototypes
extern int main(int argc, const char **argv);
void usage(const char *name);

//
// main()
// (example single threaded ca server tool main loop)
//
extern int main(int argc, const char **argv)
{
    osiTime delay(0u,10000000u);     // (sec, nsec) (10 ms)
    int doStats = 0;
    snoopServer *pCAS;
    unsigned debugLevel = 0u;
    float executionTime;
    float waitTime = 0.0;
    int forever = 1;
    char *prefix = NULL;
    int nCheck = NCHECK_DEFAULT;
    int nPrint = NPRINT_DEFAULT;
    int nSigma = NSIGMA_DEFAULT;
    float nLimit = NLIMIT_DEFAULT;
    int i;
    
    for (i=1; i<argc; i++) {
	if(sscanf(argv[i],"-c %d", &nCheck)==1) {
	    continue;
	}
	if(sscanf(argv[i], "-d %d", &debugLevel)==1) {
	    continue;
	}
	if(!strcmp(argv[i], "-h")) {
	    usage(argv[0]);
	    return(0);
	}
	if(sscanf(argv[i],"-l %f", &nLimit)==1) {
	    continue;
	}
	if(sscanf(argv[i],"-p %d", &nPrint)==1) {
	    continue;
	}
	if(!strncmp(argv[i],"-n",2)) {
	    prefix=(char *)malloc(PREFIX_SIZE*sizeof(char));
	    if(prefix) {
		strncpy(prefix,&argv[i][2],PREFIX_SIZE);
		prefix[PREFIX_SIZE-1]='\0';
		doStats = 1;
	    }
	    continue;
	}
	if(sscanf(argv[i],"-s %d", &nSigma)==1) {
	    continue;
	}
	if(!strcmp(argv[i], "-r")) {
	    doStats = 1;
	    return(0);
	}
	if(sscanf(argv[i],"-t %f", &executionTime)==1) {
	    forever = aitFalse;
	    continue;
	}
	if(sscanf(argv[i],"-w %f", &waitTime)==1) {
	    continue;
	}
	printf("Unknown option: \"%s\"\n", argv[i]);
	usage(argv[0]);
	return(1);
    }
    
    pCAS = new snoopServer(prefix,nCheck,nPrint,nSigma,nLimit);
    if(!pCAS) {
	return (-1);
    }
    pCAS->setDebugLevel(debugLevel);
    pCAS->disable();
    
  // Main loop
    printf("Starting %s at %s\n",VERSION,timeStamp());
    if(doStats) printf("PV name prefix is %s\n",pCAS->getPrefix());

    osiTime begin(osiTime::getCurrent());
    
  // Loop here until the specified wait time
    osiTime delay0(osiTime::getCurrent() - begin);
    osiTime wait(waitTime);
    while(delay0 < wait) {
	fileDescriptorManager.process(delay0);
	delay0 = osiTime::getCurrent() - begin;
    }

  // Initialize stat counters
    if(doStats) {
	osiTime rateStatsDelay(RATE_STATS_INTERVAL,0u);
	snoopRateStatsTimer *statTimer =
	  new snoopRateStatsTimer(rateStatsDelay, pCAS);
      // Call the expire routine to initialize it
	statTimer->expire();
    }
    
  // Start the processing
    double processedTime = 0.0;
    pCAS->enable();
    osiTime start(osiTime::getCurrent());
    while (aitTrue) {
	fileDescriptorManager.process(delay);
	if(!forever) {
	    processedTime=(double)(osiTime::getCurrent() - start);
	    if(processedTime > executionTime) break;
	}
    }

  // Print timing
    double elapsedTime=processedTime+start-begin;
    pCAS->setProcessTime(processedTime);
    printf("CaSnopper terminating after %.2f seconds [%.2f minutes]\n",
      elapsedTime,elapsedTime/60.);
    printf("  Data collected for %.2f seconds [%.2f minutes]\n",
      processedTime,processedTime/60.);
#if 0
    printf("Server Statistics:\n");
    pCAS->show(2u);
#endif
    pCAS->report();
    delete pCAS;
    return(0);
}

void usage(const char *name)
{
    printf(
      "Usage: %s [options]\n"
      "  Options:\n"
      "    -c<integer>  Check validity of top n requests (0 means all)\n"
      "    -d<integer>  Set debug level to n\n"
      "    -h           Help (This message)\n"
      "    -l<decimal>  Print all requests over n Hz\n"
      "    -p<integer>  Print top n requests (0 means all)\n"
      "    -n[<string>] Use string as prefix for internal PVs (%d chars max)\n"
      "                   Default is: %s\n"
      "    -s<integer>  Print all requests over n sigma\n"
      "    -r           Implement rate stat PVs\n"
      "    -t<decimal>  Run n seconds, then print report\n"
      "    -w<decimal>  Wait n sec before collecting data\n"
      "\n", 
      name,PREFIX_SIZE-1,DEFAULT_PREFIX);
	
}
