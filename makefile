APP = cpceg

CFLAGS = -DSDL2
LDFLAGS = -lSDL2

SOURCE_FILES = cpcec.c
OBJ_FILES = cpcec.o

.PHONY: build

build: $(APP)

$(APP): $(OBJ_FILES)
	$(CC) -o $@ $(SOURCE_FILES) $(CFLAGS) $(LDFLAGS)

.PHONY: clean

clean:
	$(RM) *.o $(APP) 
