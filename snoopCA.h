/* snoopClient.h: Modifications to CA */

#ifdef __cplusplus
#define UNREFERENCED(x) (x)
#else
#define UNREFERENCED(x)
#endif

#ifdef ALLOCATE_STORAGE
#define EXTERN
#else
#define EXTERN extern
#endif

#include "bsdSocketResource.h"
#include "caProto.h"

#ifdef __cplusplus
extern "C" {
#endif

EXTERN unsigned long udpCount;
EXTERN void (*udpMsgHook)(const struct sockaddr_in *net_addr,
  const struct sockaddr_in *in_addr, ca_uint16_t cmd);
    
#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif
