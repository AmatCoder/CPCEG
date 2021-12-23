APP = cpceg

CFLAGS = -DSDL2 `pkg-config --cflags sdl2 gtk+-3.0 gmodule-export-2.0 glib-2.0`
LDFLAGS = `pkg-config --libs sdl2 gtk+-3.0 gmodule-export-2.0 glib-2.0`

SOURCE_FILES = cpceg.c cpcec.c
OBJ_FILES = cpceg.o cpcec.o

.PHONY: build

build: $(APP)

cpceg.o:	cpceg.c cpceg.h
cpcec.o:	cpcec.c cpcec-a8.h  cpcec-ay.h  cpcec-d7.h  cpcec-k7.h  cpcec-os.h  cpcec-ox.h  cpcec-rt.h  cpcec-z8.h

$(APP): $(OBJ_FILES)

.PHONY: clean

clean:
	$(RM) *.o $(APP) 
