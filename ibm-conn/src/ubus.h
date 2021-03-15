enum {
	TOTAL_MEMORY,
	FREE_MEMORY,
	SHARED_MEMORY,
	BUFFERED_MEMORY,
	__MEMORY_MAX,
};

enum  {
	MEMORY_DATA,
	__INFO_MAX,
};

void board_cb(struct ubus_request *req, int type, struct blob_attr *msg);
