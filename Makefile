CONTIKI=../..
CONTIKI_PROJECT = cister_collector
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

CONTIKI_WITH_RIME = 1
MODULES += core/net/mac/tsch

all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include
