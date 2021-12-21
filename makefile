APP = cpceg

CFLAGS = -DSDL2
LDFLAGS = -lSDL2

SOURCE_FILES = cpcec.c
OBJ_FILES = cpcec.o

.PHONY: build

build: $(APP)

cpcec.o:	cpcec.c cpcec-a8.h  cpcec-ay.h  cpcec-d7.h  cpcec-k7.h  cpcec-os.h  cpcec-ox.h  cpcec-rt.h  cpcec-z8.h

$(APP): $(OBJ_FILES)
	$(CC) -o $@ $(SOURCE_FILES) $(CFLAGS) $(LDFLAGS)

.PHONY: clean

clean:
	$(RM) *.o $(APP) 
