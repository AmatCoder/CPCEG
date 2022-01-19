APP = cpceg

CFLAGS = -DSDL2 -Wl,-export-dynamic `pkg-config --cflags sdl2 gtk+-3.0 gmodule-export-2.0`
LDFLAGS = `pkg-config --libs sdl2 gtk+-3.0 gmodule-export-2.0 `

SOURCE_FILES = cpceg.c cpcec.c file.c machine.c dialogs.c resources.c
OBJ_FILES = cpceg.o cpcec.o file.o machine.o dialogs.o resources.o

.PHONY: build

build: $(APP)

cpceg.o:	cpceg.c cpceg.h
cpcec.o:	cpcec.c cpcec-a8.h  cpcec-ay.h  cpcec-d7.h  cpcec-k7.h  cpcec-os.h  cpcec-ox.h  cpcec-rt.h  cpcec-z8.h
resources.o:	resources.c resources.h
file.o:	file.c
machine.o: machine.c
dialogs.o: dialogs.c dialogs.h

$(APP): $(OBJ_FILES)

.PHONY: clean

clean:
	$(RM) *.o $(APP) 
