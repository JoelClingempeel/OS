#include "disk.h"
#include "interrupts.h"

// ATA PIO primary bus ports.
#define ATA_DATA         0x1F0
#define ATA_SECTOR_COUNT 0x1F2
#define ATA_LBA_LOW      0x1F3
#define ATA_LBA_MID      0x1F4
#define ATA_LBA_HIGH     0x1F5
#define ATA_DRIVE_HEAD   0x1F6
#define ATA_STATUS       0x1F7
#define ATA_COMMAND      0x1F7

#define ATA_STATUS_BSY   0x80
#define ATA_STATUS_DRQ   0x08

#define ATA_CMD_READ     0x20
#define ATA_CMD_WRITE    0x30
#define ATA_CMD_FLUSH    0xE7

// 0 = primary master, 1 = primary slave.
// disk.img is attached as the second drive, so use slave.
#define DISK_DRIVE       1

static void ata_wait_ready() {
    while (inb(ATA_STATUS) & ATA_STATUS_BSY);
}

static void ata_wait_drq() {
    while (!(inb(ATA_STATUS) & ATA_STATUS_DRQ));
}

void disk_read(uint32_t lba, uint8_t* buf) {
    ata_wait_ready();
    outb(ATA_DRIVE_HEAD, 0xE0 | (DISK_DRIVE << 4) | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_LBA_LOW,  (uint8_t)(lba));
    outb(ATA_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, ATA_CMD_READ);
    ata_wait_drq();
    for (int i = 0; i < 256; i++) {
        uint16_t word = inw(ATA_DATA);
        buf[2 * i]     = (uint8_t)(word & 0xFF);
        buf[2 * i + 1] = (uint8_t)(word >> 8);
    }
}

void disk_write(uint32_t lba, const uint8_t* buf) {
    ata_wait_ready();
    outb(ATA_DRIVE_HEAD, 0xE0 | (DISK_DRIVE << 4) | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_LBA_LOW,  (uint8_t)(lba));
    outb(ATA_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, ATA_CMD_WRITE);
    ata_wait_drq();
    for (int i = 0; i < 256; i++) {
        uint16_t word = (uint16_t)buf[2 * i] | ((uint16_t)buf[2 * i + 1] << 8);
        outw(ATA_DATA, word);
    }
    outb(ATA_COMMAND, ATA_CMD_FLUSH);
    ata_wait_ready();
}
