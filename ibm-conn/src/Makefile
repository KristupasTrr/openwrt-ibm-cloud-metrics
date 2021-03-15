BIN:=ibm-conn
LDFLAGS:=-lblobmsg_json -lubox -lubus -liotp-as-device # cia nurodom package'o liba

.PHONY: all clean

all:$(BIN)

$(BIN): conn.c ubus.c
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BIN)