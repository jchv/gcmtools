#
# GCM Tools Makefile
#

include nall/Makefile
include phoenix/Makefile

application := gcm-tool
flags := -std=gnu++0x -I. -fomit-frame-pointer -O2
link := -O2
prefix := /usr/local

# windows-specific code
ifeq ($(platform),win)
  winrc := winrc.o
  flags := -m32 $(flags)
  link := -m32 -mwindows $(link)
endif

all: gcm-tool gcm-info

# gcm-tool
gcm-tool: gcm-tool.o
ifeq ($(platform),win)
	windres --target=pe-i386 phoenix/windows/phoenix.rc $(winrc)
endif
	$(cpp) -o $(application) phoenix.o gcm-tool.o $(winrc) $(link) $(phoenixlink)

gcm-tool.o: phoenix.o gcm-tool.cpp
	$(cpp) -c -o gcm-tool.o gcm-tool.cpp $(flags) $(phoenixflags)

# gcm-info
gcm-info: gcm-info.o
ifeq ($(platform),win)
	windres --target=pe-i386 phoenix/windows/phoenix.rc $(winrc)
endif
	$(cpp) -o gcm-info phoenix.o gcm-info.cpp $(winrc) $(flags) $(link) $(phoenixlink)

gcm-info.o: phoenix.o gcm-info.cpp
	$(cpp) -c -o gcm-info.o gcm-info.cpp $(flags) $(phoenixflags)

# phoenix
phoenix.o:
	$(cpp) -c -o phoenix.o phoenix/phoenix.cpp $(flags) $(phoenixflags)

# management
install:
	sudo cp $(application) $(prefix)/bin/$(application)

uninstall:
	sudo rm $(prefix)/bin/$(application)

clean:
	-@$(call delete,*.o gcm-tool gcm-info)

# pseudo targets
force:
