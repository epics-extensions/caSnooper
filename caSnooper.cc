// CaSnooper: Server that logs broadcasts

#include "snoopServer.h"
#include "fdManager.h"

#define NCHECK_DEFAULT -1
#define NPRINT_DEFAULT -1
#define NSIGMA_DEFAULT -1
#define NLIMIT_DEFAULT 0.0

// Interval for rate statistics in seconds
#define RATE_STATS_INTERVAL 10u

// Function prototypes
extern int main(int argc, const char **argv);
void usage(const char *name);

//
// main()
// (example single threaded ca server tool main loop)
//
extern int main(int argc, const char **argv)
{
    snoopServer *pCAS;
    unsigned debugLevel = 0u;
    float executionTime;
    float waitTime = 0.0;
    int forever = 1;
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
	if(sscanf(argv[i],"-s %d", &nSigma)==1) {
	    continue;
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
    
    pCAS = new snoopServer(nCheck,nPrint,nSigma,nLimit);
    if(!pCAS) {
	return (-1);
    }
    
    pCAS->setDebugLevel(debugLevel);
    
  // Main loop
    printf("Starting CaSnooper\n");
    osiTime begin(osiTime::getCurrent());
    pCAS->disable();

  // Loop here until the specified wait time
    osiTime delay0(osiTime::getCurrent() - begin);
    osiTime wait(waitTime);
    while(delay0 < wait) {
	fileDescriptorManager.process(delay0);
	delay0 = osiTime::getCurrent() - begin;
    }

  // Start the processing
    pCAS->enable();
    osiTime start(osiTime::getCurrent());
    if(forever) {
	osiTime	delay(1000u,0u);    // (sec,nsec)
	while (aitTrue) {
	    fileDescriptorManager.process(delay);
	}
    } else {
      // Loop here until the specified execution time
	osiTime total(executionTime);
	osiTime delay(osiTime::getCurrent() - start);
	while(delay < total) {
	    fileDescriptorManager.process(delay);
	    delay = osiTime::getCurrent() - start;
	}
    }

  // Print timing
    double elapsedTime=(double)(osiTime::getCurrent() - begin);
    double processedTime=elapsedTime+begin-start;
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
      "    -s<integer>  Print all requests over n sigma\n"
      "    -t<decimal>  Run n seconds, then print report\n"
      "    -w<decimal>  Wait n sec before collecting data\n"
      "\n", 
      name);
	
}
