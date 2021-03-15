#include <stdio.h>
#include <signal.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

/* for UBUS calls */
#include <libubox/blobmsg_json.h>
#include <libubus.h>

enum {
	TOTAL_MEMORY,
	FREE_MEMORY,
	SHARED_MEMORY,
	BUFFERED_MEMORY,
	__MEMORY_MAX,
};

enum {
	MEMORY_DATA,
	__INFO_MAX,
};


static const struct blobmsg_policy memory_policy[__MEMORY_MAX] = {
	[TOTAL_MEMORY] = { .name = "total", .type = BLOBMSG_TYPE_INT64 },
	[FREE_MEMORY] = { .name = "free", .type = BLOBMSG_TYPE_INT64 },
	[SHARED_MEMORY] = { .name = "shared", .type = BLOBMSG_TYPE_INT64 },
	[BUFFERED_MEMORY] = { .name = "buffered", .type = BLOBMSG_TYPE_INT64 },
};

static const struct blobmsg_policy info_policy[__INFO_MAX] = {
	[MEMORY_DATA] = { .name = "memory", .type = BLOBMSG_TYPE_TABLE },
};

static void board_cb(struct ubus_request *req, int type, struct blob_attr *msg) {
	struct blob_attr *buf = (struct blob_attr *)req->priv;
	struct blob_attr *tb[__INFO_MAX];

	blobmsg_parse(info_policy, __INFO_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[MEMORY_DATA]) {
		fprintf(stderr, "No memory data received\n");
		return;
	}

	blobmsg_parse(memory_policy, __MEMORY_MAX, buf, blobmsg_data(tb[MEMORY_DATA]), blobmsg_data_len(tb[MEMORY_DATA]));
}