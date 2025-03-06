ASM = nasm

SRC = src
BUILD = build

$(BUILD)/floppy.img: $(BUILD)/boot.bin
	cp $(BUILD)/boot.bin $(BUILD)/floppy.img
	truncate -s 1440k $(BUILD)/floppy.img

$(BUILD)/boot.bin: $(SRC)/boot.asm
	$(ASM) $(SRC)/boot.asm -f bin -o $(BUILD)/boot.bin
