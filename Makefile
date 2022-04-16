CC      = cc
CFLAGS  = -O2 -Wall
INC     = -framework IOKit
EXEC    = influxdb-smc
SOURCES = smc-influxdb.c

build : $(EXEC)

clean : 
	rm $(EXEC)

install : $(EXEC)
	@install -v $(EXEC) $(HOME)/.bin/$(EXEC)

$(EXEC) : smc-influxdb.c
	$(CC) $(CFLAGS) $(INC) -o $@ $?
