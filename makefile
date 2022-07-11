#Gnu makefile, recognizes system and makes accordingly, Linux version assumes that the libraries have already been installed by the system package manager, Windows does not have a package manager, so I include the libraries explicitly, I don't own a mac so I can't work with that

ifeq ($(OS),Windows_NT)
	OS_NAME = Windows
else
	OS_NAME := $(shell uname -s)
endif



default:
ifeq ($(OS),Windows_NT)
	mingw32-make -f makefile_windows_mingw
else
ifeq ($(OS_NAME),Linux)
	make -f makefile_linux
endif
ifeq ($(OS_NAME),Darwin)
	echo 'Not available for Mac'
endif
endif

.PHONY: clean

clean:
ifeq ($(OS),Windows_NT)
	mingw32-make -f makefile_windows_mingw clean
else
ifeq ($(OS_NAME),Linux)
	make -f makefile_linux clean
endif
ifeq ($(OS_NAME),Darwin)
	echo 'Not available for Mac'
endif
endif
