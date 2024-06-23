.PHONY: all build push

BOARD := "xiao_ble"

all: push

build:
	west build -p always -b ${BOARD} .

push: build
	west -v flash -r blackmagicprobe
