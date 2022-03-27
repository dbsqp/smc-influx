CC      = cc
CFLAGS  = -O2 -Wall
INC     = -framework IOKit
PREFIX  = /usr/local
EXEC    = smc-influxdb
SOURCES = smc-influxdb.c

build : $(EXEC)

clean : 
	rm $(EXEC)

install : $(EXEC)
	@install -v $(EXEC) $(PREFIX)/bin/$(EXEC)

$(EXEC) : smc-influxdb.c
	$(CC) $(CFLAGS) $(INC) -o $@ $?
