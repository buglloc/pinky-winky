.PHONY: all build debug push

BOARD := "holyiot_21014"

all: flash

build:
	west build -p always -b ${BOARD} .

debug:
	west debug -r blackmagicprobe

flash: build
	west -v flash -r blackmagicprobe
