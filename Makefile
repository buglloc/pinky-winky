.PHONY: all build debug push

BOARD := "holyiot_21014"

all: push

build:
	west build -p always -b ${BOARD} .

debug:
	west debug -r blackmagicprobe

push: build
	west -v flash -r blackmagicprobe
