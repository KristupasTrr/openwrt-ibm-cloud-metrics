// config ibm "ibm_sct"
//     option 'enable'     '0'
//     option 'org_id'     'Enter organization ID'
//     option 'dev_type'   'Enter device type ID'
//     option 'dev_id'     'Enter device ID'
//     option 'token'      'Enter authentication token'

#include <stdio.h>
#include <signal.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

/* Include header file of IBM Watson IoT platform C Client for devices */ 
#include <iotp_device.h>

/* for UBUS calls */
#include <libubox/blobmsg_json.h>
#include <libubus.h>

#include "ubus.h"

int rc = 0;

IoTPConfig *config = NULL;
IoTPDevice *device = NULL;

char *configFilePath = NULL;
volatile int interrupt = 0;
int useConfig = 0;
int useEnv = 0;

/* Signal handler - to support CTRL-C to quit */
void sigHandler(int signo) {
    signal(SIGINT, NULL);
    fprintf(stdout, "Received signal: %d\n", signo);
    interrupt = 1;
}

/* Get and process command line options */
void getopts(int argc, char** argv)
{
    int count = 1;

    while (count < argc)
    {
        if (strcmp(argv[count], "--config") == 0)
        {
            if (++count < argc) {
                useConfig = 1;
                configFilePath = argv[count];
            }
        }
        if (strcmp(argv[count], "--useEnv") == 0) {
            useEnv = 1;
        }
        count++;
    }
}

void logCallback (int level, char * message)
{
    if ( level > 0 )
        fprintf(stdout, "%s\n", message? message:"NULL");
    fflush(stdout);
}

void init() {

    /* Set signal handlers */
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    /* Set IoTP Client log handler */
    rc = IoTPConfig_setLogHandler(IoTPLog_FileDescriptor, stdout);
    if ( rc != 0 ) {
        fprintf(stderr, "ERROR: Failed to set IoTP Client log handler: rc=%d\n", rc);
        return;
    }

    /* Create IoTPConfig object using configuration options defined in the configuration file. */
    if ( useConfig == 1 ) {
        rc = IoTPConfig_create(&config, configFilePath);    // for conf file path
    } else {
        rc = IoTPConfig_create(&config, NULL);              // for ENV VARS
    }
    if ( rc != 0 ) {
        fprintf(stderr, "ERROR: Failed to initialize configuration: rc=%d\n", rc);
        return;
    } else {
        fprintf(stdout, "SUCCESS: Configuration initiliazed: rc=%d\n", rc);
    }
    

    /* read additional config from environment */
    if ( useEnv == 1 ) {
        IoTPConfig_readEnvironment(config);
    } 

    /* Create IoTPDevice object */
    rc = IoTPDevice_create(&device, config);
    if ( rc != 0 ) {
        fprintf(stderr, "ERROR: Failed to configure IoTP device: rc=%d\n", rc);
        return;
    } else {
        fprintf(stdout, "SUCCESS: IoTP device configured: rc=%d\n", rc);
    }

    /* Invoke connection API IoTPDevice_connect() to connect to WIoTP. */
    rc = IoTPDevice_connect(device);
    if ( rc != 0 ) {
        fprintf(stderr, "ERROR: Failed to connect to Watson IoT Platform: rc=%d\n", rc);
        fprintf(stderr, "ERROR: Returned error reason: %s\n", IOTPRC_toString(rc));
        return;
    }else {
        fprintf(stdout, "SUCCESS: Connected to Watson IoT Platform: rc=%d\n", rc);
    }

}

/* Main program */
int main(int argc, char *argv[])
{
    struct blob_attr *memory[__MEMORY_MAX];

    /* check for args */
    if ( argc < 2 ) {
        fprintf(stderr, "ERROR: Argument number should be >1\n");
        exit(1);
    }

    /* get argument options */
    getopts(argc, argv);

    /* Initiliaze IoTP config and device */
    init();

    if (rc != 0) {
        fprintf(stderr, "ERROR: Initiliazion failed!\n");
        goto error;
    }

    struct ubus_context *ctx;
	uint32_t id;
    ctx = ubus_connect(NULL);
	if (!ctx) {
        fprintf(stderr, "ERROR: Failed to connect to ubus: rc=%d\n", rc);
		goto error;
	}
    
    char data[200];

    while(!interrupt)
    {
        if (ubus_lookup_id(ctx, "system", &id) || ubus_invoke(ctx, id, "info", NULL, board_cb, memory, 3000)) {
            fprintf(stderr, "cannot request memory info from procd\n");
            rc=-1;
        } else {
            memset(data, 0, 200);
            int cx = snprintf(data, 200, "{\"data\" : {\"Total memory\": %lld, \"Free memory\": %lld, \"Shared memory\": %lld, \"Buffered memory\": %lld}}", 
                                        blobmsg_get_u64(memory[TOTAL_MEMORY]), 
                                        blobmsg_get_u64(memory[FREE_MEMORY]), 
                                        blobmsg_get_u64(memory[SHARED_MEMORY]), 
                                        blobmsg_get_u64(memory[BUFFERED_MEMORY]));

            rc = IoTPDevice_sendEvent(device, "status", data, "json", QoS0, NULL);
        }

        sleep(10);
    }

    ubus_free(ctx);

    fprintf(stdout, "Event cycle is complete.\n");

    /* Disconnect device */
    rc = IoTPDevice_disconnect(device);
    if ( rc != 0 ) {
        fprintf(stderr, "ERROR: Failed to disconnect from  Watson IoT Platform: rc=%d\n", rc);
        goto error;
    } else {
        fprintf(stdout, "SUCCESS: Disconnected from  Watson IoT Platform: rc=%d\n", rc);
    }

error:
    /* Destroy client */
    IoTPDevice_destroy(device);

    /* Clear configuration */
    IoTPConfig_clear(config);

    return 0;

}