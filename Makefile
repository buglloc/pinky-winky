.PHONY: all build debug push

BOARD := "xiao_ble"

all: push

build:
	west build -p always -b ${BOARD} .

debug:
	west debug -r blackmagicprobe

push: build
	west -v flash -r blackmagicprobe
