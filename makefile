APP = cpceg

CFLAGS = -DSDL2 -Wl,-export-dynamic `pkg-config --cflags sdl2 gtk+-3.0 gmodule-export-2.0`
LDFLAGS = `pkg-config --libs sdl2 gtk+-3.0 gmodule-export-2.0`

SOURCE_FILES = cpceg.c cpcec.c file.c machine.c settings.c video.c audio.c dialogs.c resources.c
OBJ_FILES = cpceg.o cpcec.o file.o machine.o settings.o video.o audio.o dialogs.o resources.o

.PHONY: build

build: $(APP)

cpceg.o:	cpceg.c cpceg.h
cpcec.o:	cpcec.c cpcec-a8.h  cpcec-ay.h  cpcec-d7.h  cpcec-k7.h  cpcec-os.h  cpcec-ox.h  cpcec-rt.h  cpcec-z8.h
resources.o:	resources.c resources.h
file.o:	file.c
machine.o: machine.c
settings.o: settings.c
video.o: video.c
audio.o: audio.c
dialogs.o: dialogs.c dialogs.h

$(APP): $(OBJ_FILES)

.PHONY: clean

clean:
	$(RM) *.o $(APP) 
