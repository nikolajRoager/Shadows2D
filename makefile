#Gnu makefile, recognizes system and makes accordingly, Linux version assumes that the libraries have already been installed by the system package manager, Windows does not have a package manager, so I include the libraries explicitly, I don't own a mac so I can't work with that

ifeq ($(OS),Windows_NT)
	OS_NAME = Windows
else
	OS_NAME := $(shell uname -s)
endif



default:
ifeq ($(OS_NAME),Windows)
	mingw32-make -f makefile_windows_mingw
else
ifeq ($(OS_NAME),Linux)
	make -f makefile_linux
else
	echo 'Only available for Linux adn Windows NT based systems'
endif
endif


hide_and_shoot:
ifeq ($(OS_NAME),Windows)
	mingw32-make -f makefile_windows_mingw hide_and_shoot
else
ifeq ($(OS_NAME),Linux)
	make -f makefile_linux hide_and_shoot
else
	echo 'Only available for Linux adn Windows NT based systems'
endif
endif

.PHONY: clean

clean:
ifeq ($(OS_NAME),Windows)
	mingw32-make -f makefile_windows_mingw clean
else
ifeq ($(OS_NAME),Linux)
	make -f makefile_linux clean
else
	echo 'Only available for Linux adn Windows NT based systems'
endif
endif
