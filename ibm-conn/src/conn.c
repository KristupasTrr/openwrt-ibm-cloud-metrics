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
#include "iotp_device.h"

/* for UBUS calls */
#include <libubox/blobmsg_json.h>
#include <libubus.h>

#include "ubus.c"

int rc = 0;

struct blob_attr *memory[__MEMORY_MAX];

char *configFilePath = NULL;
volatile int interrupt = 0;
char *progname = "conn.c";
int useConfig = 0;
int useEnv = 0;

/* Usage text */
void usage(void) {
    fprintf(stderr, "Usage: %s --config config_file_path\n", progname);
    exit(1);
}

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
            else
                usage();
        }
        if (strcmp(argv[count], "--useEnv") == 0) {
            useEnv = 1;
        }
        count++;
    }
}

/* 
 * Device command callback function
 * Device developers can customize this function based on their use case
 * to handle device commands sent by WIoTP.
 * Set this callback function using API setCommandHandler().
 */
void  deviceCommandCallback (char* type, char* id, char* commandName, char *format, void* payload, size_t payloadSize)
{
    fprintf(stdout, "Received device command:\n");
    fprintf(stdout, "Type=%s ID=%s CommandName=%s Format=%s Len=%d\n", 
        type?type:"", id?id:"", commandName?commandName:"", format?format:"", (int)payloadSize);
    fprintf(stdout, "Payload: %s\n", payload?(char *)payload:"");

    /* Device developers - add your custom code to process device commands */
}

void logCallback (int level, char * message)
{
    if ( level > 0 )
        fprintf(stdout, "%s\n", message? message:"NULL");
    fflush(stdout);
}

void MQTTTraceCallback (int level, char * message)
{
    if ( level > 0 )
        fprintf(stdout, "%s\n", message? message:"NULL");
    fflush(stdout);
}

/* Main program */
int main(int argc, char *argv[])
{
    int rc = 0;
    int cycle = 0;

    /* 
        * DEV_NOTES:
        * Specifiy variable for WIoT client object 
    */
    IoTPConfig *config = NULL;
    IoTPDevice *device = NULL;

    
    /* check for args */
    if ( argc < 2 )
        usage();

    /* Set signal handlers */
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    /* get argument options */
    getopts(argc, argv);

    /* Set IoTP Client log handler */
    rc = IoTPConfig_setLogHandler(IoTPLog_FileDescriptor, stdout);
    if ( rc != 0 ) {
        fprintf(stderr, "WARN: Failed to set IoTP Client log handler: rc=%d\n", rc);
        exit(1);
    }

    /* Create IoTPConfig object using configuration options defined in the configuration file. */
    if ( useConfig == 1 ) {
        rc = IoTPConfig_create(&config, configFilePath);    // for conf file path
    } else {
        rc = IoTPConfig_create(&config, NULL);              // for ENV VARS
    }
    if ( rc != 0 ) {
        fprintf(stderr, "ERROR: Failed to initialize configuration: rc=%d\n", rc);
        exit(1);
    }

    /* read additional config from environment */
    if ( useEnv == 1 ) {
        IoTPConfig_readEnvironment(config);
    } 

    /* Create IoTPDevice object */
    rc = IoTPDevice_create(&device, config);
    if ( rc != 0 ) {
        fprintf(stderr, "ERROR: Failed to configure IoTP device: rc=%d\n", rc);
        exit(1);
    }

    /* Set MQTT Trace handler */
    rc = IoTPDevice_setMQTTLogHandler(device, &MQTTTraceCallback);
    if ( rc != 0 ) {
        fprintf(stderr, "WARN: Failed to set MQTT Trace handler: rc=%d\n", rc);
    }

    /* Invoke connection API IoTPDevice_connect() to connect to WIoTP. */
    rc = IoTPDevice_connect(device);
    if ( rc != 0 ) {
        fprintf(stderr, "ERROR: Failed to connect to Watson IoT Platform: rc=%d\n", rc);
        fprintf(stderr, "ERROR: Returned error reason: %s\n", IOTPRC_toString(rc));
        exit(1);
    }

    /*
     * Set device command callback using API IoTPDevice_setCommandsHandler().
     * Refer to deviceCommandCallback() function DEV_NOTES for details on
     * how to process device commands received from WIoTP.
     */
    IoTPDevice_setCommandsHandler(device, deviceCommandCallback);


    /* Use IoTPDevice_sendEvent() API to send device events to Watson IoT Platform. */

    struct ubus_context *ctx;
	uint32_t id;
    ctx = ubus_connect(NULL);
	if (!ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		return -1;
	}
    
    char data[200];

    while(!interrupt)
    {

        if (ubus_lookup_id(ctx, "system", &id) || ubus_invoke(ctx, id, "info", NULL, board_cb, memory, 3000)) {
            fprintf(stderr, "cannot request memory info from procd\n");
            rc=-1;
        } else {
            
            int cx = snprintf(data, 200, "{\"data\" : {\"Total memory\": %lld, \"Free memory\": %lld, \"Shared memory\": %lld, \"Buffered memory\": %lld}}", 
                                        blobmsg_get_u64(memory[TOTAL_MEMORY]), 
                                        blobmsg_get_u64(memory[FREE_MEMORY]), 
                                        blobmsg_get_u64(memory[SHARED_MEMORY]), 
                                        blobmsg_get_u64(memory[BUFFERED_MEMORY]));

            rc = IoTPDevice_sendEvent(device,"status", data, "json", QoS0, NULL);
        }

        sleep(10);
    }
    ubus_free(ctx);

    fprintf(stdout, "Publish event cycle is complete.\n");

    /* Disconnect device */
    rc = IoTPDevice_disconnect(device);
    if ( rc != IOTPRC_SUCCESS ) {
        fprintf(stderr, "ERROR: Failed to disconnect from  Watson IoT Platform: rc=%d\n", rc);
        exit(1);
    }

    /* Destroy client */
    IoTPDevice_destroy(device);

    /* Clear configuration */
    IoTPConfig_clear(config);

    return 0;

}