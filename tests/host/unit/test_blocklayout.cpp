/*************************** Block Layout Tests ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers reading what a block device carries from its first blocks: the
filesystem its signature names, the way linux blkid reads it, and the
partitions a dos partition table lists.

Author          : Suraj I.
created Date    : 21st Oct 2026
******************************************************************************/

#include <utility/BlockLayout.h>
#include <utility/DataTypeConversions.h>
#include <pditest.h>

#include <string.h>

namespace
{

const uint32_t DISK_BLOCKS = 16384;

struct Blocks
{
    uint8_t m_first[512];
    uint8_t m_third[512];

    Blocks()
    {
        memset(m_first, 0, sizeof(m_first));
        memset(m_third, 0, sizeof(m_third));
    }

    void signature()
    {
        m_first[510] = 0x55;
        m_first[511] = 0xAA;
    }

    void fatBootSector(bool fat32)
    {
        m_first[0] = 0xEB;
        m_first[1] = 0x3C;
        m_first[2] = 0x90;
        memcpy(m_first + 3, "MSDOS5.0", 8);
        m_first[11] = 0x00;
        m_first[12] = 0x02;
        m_first[13] = 4;
        m_first[14] = 1;
        m_first[16] = 2;
        m_first[21] = 0xF8;
        memcpy(m_first + (fat32 ? 82 : 54), fat32 ? "FAT32   " : "FAT16   ", 8);
        signature();
    }

    void partition(uint8_t slot, uint8_t status, uint8_t id, uint32_t start, uint32_t blocks)
    {
        uint8_t *entry = m_first + 446 + slot * 16;
        entry[0] = status;
        entry[4] = id;
        memcpy(entry + 8, &start, 4);
        memcpy(entry + 12, &blocks, 4);
        signature();
    }

    void ext(uint32_t compat, uint32_t incompat)
    {
        m_third[56] = 0x53;
        m_third[57] = 0xEF;
        memcpy(m_third + 92, &compat, 4);
        memcpy(m_third + 96, &incompat, 4);
    }
};

} // namespace

TEST(blocklayout, a_blank_disk_carries_nothing)
{
    Blocks b;
    block_partition_t parts[BLOCK_PARTITIONS_MAX];
    uint8_t count = 9;

    ASSERT_EQ((int)blockFsType(b.m_first, b.m_third), (int)BLOCK_FS_NONE);
    ASSERT_EQ((int)blockTable(b.m_first, DISK_BLOCKS, parts, BLOCK_PARTITIONS_MAX, count), (int)BLOCK_TABLE_NONE);
    ASSERT_EQ((int)count, 0);
}

TEST(blocklayout, a_fat_boot_sector_is_vfat_and_no_partition_table)
{
    Blocks fat16, fat32;
    fat16.fatBootSector(false);
    fat32.fatBootSector(true);
    block_partition_t parts[BLOCK_PARTITIONS_MAX];
    uint8_t count = 0;

    ASSERT_EQ((int)blockFsType(fat16.m_first, fat16.m_third), (int)BLOCK_FS_VFAT);
    ASSERT_EQ((int)blockFsType(fat32.m_first, fat32.m_third), (int)BLOCK_FS_VFAT);
    ASSERT_EQ((int)blockTable(fat16.m_first, DISK_BLOCKS, parts, BLOCK_PARTITIONS_MAX, count), (int)BLOCK_TABLE_NONE);
}

TEST(blocklayout, a_boot_sector_with_a_bad_sector_size_is_not_vfat)
{
    Blocks b;
    b.fatBootSector(false);
    b.m_first[11] = 0x10;
    b.m_first[12] = 0x01;

    ASSERT_EQ((int)blockFsType(b.m_first, b.m_third), (int)BLOCK_FS_NONE);
}

TEST(blocklayout, exfat_and_ntfs_are_named_by_their_oem_name)
{
    Blocks exfat, ntfs;
    memcpy(exfat.m_first + 3, "EXFAT   ", 8);
    exfat.signature();
    memcpy(ntfs.m_first + 3, "NTFS    ", 8);
    ntfs.signature();

    ASSERT_EQ((int)blockFsType(exfat.m_first, exfat.m_third), (int)BLOCK_FS_EXFAT);
    ASSERT_EQ((int)blockFsType(ntfs.m_first, ntfs.m_third), (int)BLOCK_FS_NTFS);
}

TEST(blocklayout, ext_is_told_apart_by_its_journal_and_extents)
{
    Blocks ext2, ext3, ext4;
    ext2.ext(0, 0);
    ext3.ext(0x4, 0);
    ext4.ext(0x4, 0x40);

    ASSERT_EQ((int)blockFsType(ext2.m_first, ext2.m_third), (int)BLOCK_FS_EXT2);
    ASSERT_EQ((int)blockFsType(ext3.m_first, ext3.m_third), (int)BLOCK_FS_EXT3);
    ASSERT_EQ((int)blockFsType(ext4.m_first, ext4.m_third), (int)BLOCK_FS_EXT4);
}

TEST(blocklayout, littlefs_is_named_by_its_magic)
{
    Blocks b;
    memcpy(b.m_first + 8, "littlefs", 8);

    ASSERT_EQ((int)blockFsType(b.m_first, b.m_third), (int)BLOCK_FS_LITTLEFS);
}

TEST(blocklayout, no_block_to_read_carries_nothing)
{
    Blocks b;
    b.ext(0, 0);
    block_partition_t parts[BLOCK_PARTITIONS_MAX];
    uint8_t count = 9;

    ASSERT_EQ((int)blockFsType(nullptr, b.m_third), (int)BLOCK_FS_NONE);
    ASSERT_EQ((int)blockFsType(b.m_first, nullptr), (int)BLOCK_FS_NONE);
    ASSERT_EQ((int)blockTable(nullptr, DISK_BLOCKS, parts, BLOCK_PARTITIONS_MAX, count), (int)BLOCK_TABLE_NONE);
    ASSERT_EQ((int)count, 0);
    b.partition(0, 0x00, 0x0C, 2048, 8192);
    ASSERT_EQ((int)blockTable(b.m_first, DISK_BLOCKS, nullptr, BLOCK_PARTITIONS_MAX, count), (int)BLOCK_TABLE_NONE);
}

TEST(blocklayout, a_boot_sector_is_vfat_only_with_every_field_fat_allows)
{
    struct Field
    {
        uint8_t at;
        uint8_t value;
        bool vfat;
    };
    const Field fields[] = {
        {0, 0xE9, true}, {2, 0x00, false}, {12, 0x04, true}, {12, 0x08, true}, {12, 0x10, true},
        {13, 0, false}, {13, 3, false}, {14, 0, false}, {16, 0, false}, {16, 3, false},
        {16, 1, true}, {21, 0xF0, true}, {21, 0xF5, false}, {21, 0xFF, true},
    };

    for (const Field &f : fields)
    {
        Blocks b;
        b.fatBootSector(false);
        b.m_first[f.at] = f.value;
        ASSERT_EQ((int)blockFsType(b.m_first, b.m_third), f.vfat ? (int)BLOCK_FS_VFAT : (int)BLOCK_FS_NONE);
    }
}

TEST(blocklayout, half_a_boot_signature_is_no_table)
{
    Blocks b;
    b.partition(0, 0x00, 0x0C, 2048, 8192);
    b.m_first[511] = 0x00;
    block_partition_t parts[BLOCK_PARTITIONS_MAX];
    uint8_t count = 0;

    ASSERT_EQ((int)blockTable(b.m_first, DISK_BLOCKS, parts, BLOCK_PARTITIONS_MAX, count), (int)BLOCK_TABLE_NONE);
    b.m_first[511] = 0xAA;
    b.m_first[510] = 0x00;
    ASSERT_EQ((int)blockTable(b.m_first, DISK_BLOCKS, parts, BLOCK_PARTITIONS_MAX, count), (int)BLOCK_TABLE_NONE);
}

TEST(blocklayout, an_entry_with_no_start_or_no_length_leaves_its_slot_empty_and_one_past_the_room_is_left_out)
{
    Blocks b;
    b.partition(0, 0x00, 0x0C, 0, 8192);
    b.partition(1, 0x00, 0x0C, 2048, 0);
    b.partition(2, 0x00, 0x0C, 2048, 1024);
    b.partition(3, 0x00, 0x83, 4096, 1024);
    block_partition_t parts[BLOCK_PARTITIONS_MAX];
    uint8_t count = 0;

    ASSERT_EQ((int)blockTable(b.m_first, DISK_BLOCKS, parts, BLOCK_PARTITIONS_MAX, count), (int)BLOCK_TABLE_DOS);
    ASSERT_EQ((int)count, 4);
    ASSERT_EQ(parts[0].m_blocks, 0u);
    ASSERT_EQ(parts[1].m_blocks, 0u);
    ASSERT_EQ((int)parts[2].m_start, 2048);
    ASSERT_EQ((int)parts[3].m_start, 4096);

    ASSERT_EQ((int)blockTable(b.m_first, DISK_BLOCKS, parts, 3, count), (int)BLOCK_TABLE_DOS);
    ASSERT_EQ((int)count, 3);
    ASSERT_EQ((int)parts[2].m_start, 2048);
}

TEST(blocklayout, a_dos_table_lists_its_partitions)
{
    Blocks b;
    b.partition(0, 0x00, 0x0C, 2048, 8192);
    b.partition(1, 0x80, 0x83, 10240, 4096);
    block_partition_t parts[BLOCK_PARTITIONS_MAX];
    uint8_t count = 0;

    ASSERT_EQ((int)blockTable(b.m_first, DISK_BLOCKS, parts, BLOCK_PARTITIONS_MAX, count), (int)BLOCK_TABLE_DOS);
    ASSERT_EQ((int)count, 2);
    ASSERT_EQ((int)parts[0].m_start, 2048);
    ASSERT_EQ((int)parts[0].m_blocks, 8192);
    ASSERT_EQ((int)parts[0].m_id, 0x0C);
    ASSERT_EQ((int)parts[1].m_start, 10240);
    ASSERT_EQ((int)parts[1].m_id, 0x83);
}

TEST(blocklayout, a_partition_past_the_end_of_the_disk_is_left_out)
{
    Blocks b;
    b.partition(0, 0x00, 0x0C, 2048, 8192);
    b.partition(1, 0x00, 0x0C, 12288, 8192);
    block_partition_t parts[BLOCK_PARTITIONS_MAX];
    uint8_t count = 0;

    ASSERT_EQ((int)blockTable(b.m_first, DISK_BLOCKS, parts, BLOCK_PARTITIONS_MAX, count), (int)BLOCK_TABLE_DOS);
    ASSERT_EQ((int)count, 1);
}

TEST(blocklayout, a_table_with_a_bad_status_byte_is_no_table)
{
    Blocks b;
    b.partition(0, 0x12, 0x0C, 2048, 8192);
    block_partition_t parts[BLOCK_PARTITIONS_MAX];
    uint8_t count = 0;

    ASSERT_EQ((int)blockTable(b.m_first, DISK_BLOCKS, parts, BLOCK_PARTITIONS_MAX, count), (int)BLOCK_TABLE_NONE);
    ASSERT_EQ((int)count, 0);
}

TEST(blocklayout, a_protective_entry_is_a_gpt_disk_with_no_dos_partitions)
{
    Blocks b;
    b.partition(0, 0x00, 0xEE, 1, DISK_BLOCKS - 1);
    block_partition_t parts[BLOCK_PARTITIONS_MAX];
    uint8_t count = 0;

    ASSERT_EQ((int)blockTable(b.m_first, DISK_BLOCKS, parts, BLOCK_PARTITIONS_MAX, count), (int)BLOCK_TABLE_GPT);
    ASSERT_EQ((int)count, 0);
}

/**
 * A dos label with every partition deleted is still a label, the way linux
 * blkid and fdisk report it.
 */
TEST(blocklayout, a_signature_with_no_entries_is_an_empty_dos_table)
{
    Blocks b;
    b.signature();
    block_partition_t parts[BLOCK_PARTITIONS_MAX];
    uint8_t count = 9;

    ASSERT_EQ((int)blockTable(b.m_first, DISK_BLOCKS, parts, BLOCK_PARTITIONS_MAX, count), (int)BLOCK_TABLE_DOS);
    ASSERT_EQ((int)count, 0);
}

/* ------------------------------------------------------- what wipefs finds */

TEST(blocklayout, a_fat_boot_sector_without_its_signature_is_not_fat)
{
    Blocks b;
    b.fatBootSector(true);
    b.m_first[510] = 0;
    b.m_first[511] = 0;
    ASSERT_EQ((int)blockFsType(b.m_first, b.m_third), (int)BLOCK_FS_NONE);
}

TEST(blocklayout, a_blank_block_carries_no_signature)
{
    Blocks b;
    block_signature_t found[BLOCK_SIGNATURES_MAX];
    ASSERT_EQ((int)blockSignatures(b.m_first, 0, found, BLOCK_SIGNATURES_MAX), 0);
}

TEST(blocklayout, fat32_is_named_by_its_type_and_its_boot_signature)
{
    Blocks b;
    b.fatBootSector(true);
    block_signature_t found[BLOCK_SIGNATURES_MAX];
    ASSERT_EQ((int)blockSignatures(b.m_first, 0, found, BLOCK_SIGNATURES_MAX), 2);
    ASSERT_EQ((int)found[0].m_offset, 0x52);
    ASSERT_EQ((int)found[0].m_size, 8);
    ASSERT_STREQ(found[0].m_name, "vfat");
    ASSERT_EQ((int)found[1].m_offset, 0x1FE);
    ASSERT_EQ((int)found[1].m_size, 2);
    ASSERT_STREQ(found[1].m_name, "vfat");
}

TEST(blocklayout, fat16_is_named_by_the_type_at_its_own_place)
{
    Blocks b;
    b.fatBootSector(false);
    block_signature_t found[BLOCK_SIGNATURES_MAX];
    ASSERT_EQ((int)blockSignatures(b.m_first, 0, found, BLOCK_SIGNATURES_MAX), 2);
    ASSERT_EQ((int)found[0].m_offset, 0x36);
}

TEST(blocklayout, a_dos_table_is_named_by_its_boot_signature)
{
    Blocks b;
    b.partition(0, 0x00, 0x0C, 2048, 4096);
    block_signature_t found[BLOCK_SIGNATURES_MAX];
    ASSERT_EQ((int)blockSignatures(b.m_first, 0, found, BLOCK_SIGNATURES_MAX), 1);
    ASSERT_EQ((int)found[0].m_offset, 0x1FE);
    ASSERT_STREQ(found[0].m_name, "dos");
}

TEST(blocklayout, ext_is_named_in_the_third_block_and_littlefs_in_any)
{
    Blocks b;
    b.ext(0x0004, 0);
    block_signature_t found[BLOCK_SIGNATURES_MAX];
    ASSERT_EQ((int)blockSignatures(b.m_third, 2, found, BLOCK_SIGNATURES_MAX), 1);
    ASSERT_EQ((int)found[0].m_block, 2);
    ASSERT_EQ((int)found[0].m_offset, 56);
    ASSERT_STREQ(found[0].m_name, "ext3");

    uint8_t copy[512];
    memset(copy, 0, sizeof(copy));
    memcpy(copy + 8, "littlefs", 8);
    ASSERT_EQ((int)blockSignatures(copy, 8, found, BLOCK_SIGNATURES_MAX), 1);
    ASSERT_EQ((int)found[0].m_block, 8);
    ASSERT_EQ((int)found[0].m_offset, 8);
    ASSERT_STREQ(found[0].m_name, "littlefs");
}

/* ------------------------------------------------- writing a dos table */

TEST(blocklayout, a_built_table_reads_back_and_keeps_the_boot_code)
{
    uint8_t first[512];
    for (int i = 0; i < 512; i++)
    {
        first[i] = (uint8_t)(i * 7);
    }
    block_partition_t parts[2];
    parts[0].m_start = 2048;
    parts[0].m_blocks = 100000;
    parts[0].m_id = 0x0C;
    parts[0].m_boot = true;
    parts[1].m_start = 102048;
    parts[1].m_blocks = 50000;
    parts[1].m_id = 0x83;

    blockTableBuild(first, parts, 2, 0x1234ABCDu);

    for (int i = 0; i < 440; i++)
    {
        ASSERT_EQ((int)first[i], (int)(uint8_t)(i * 7));
    }
    ASSERT_EQ((uint32_t)blockDiskId(first), 0x1234ABCDu);
    block_partition_t back[BLOCK_PARTITIONS_MAX];
    uint8_t count = 0;
    ASSERT_EQ((int)blockTable(first, 200000, back, BLOCK_PARTITIONS_MAX, count), (int)BLOCK_TABLE_DOS);
    ASSERT_EQ((int)count, 2);
    ASSERT_EQ(back[0].m_start, 2048u);
    ASSERT_EQ(back[0].m_blocks, 100000u);
    ASSERT_EQ((int)back[0].m_id, 0x0C);
    ASSERT_TRUE(back[0].m_boot);
    ASSERT_EQ(back[1].m_start, 102048u);
    ASSERT_FALSE(back[1].m_boot);
    ASSERT_EQ((int)first[446 + 1], 32);
    ASSERT_EQ((int)first[446 + 2], 33);
    ASSERT_EQ((int)first[446 + 3], 0);
}

TEST(blocklayout, an_entry_past_what_chs_can_hold_carries_its_most)
{
    uint8_t first[512];
    memset(first, 0, sizeof(first));
    block_partition_t part;
    part.m_start = 2048;
    part.m_blocks = 20000000;
    part.m_id = 0x0C;
    blockTableBuild(first, &part, 1, 1);
    ASSERT_EQ((int)first[446 + 5], 254);
    ASSERT_EQ((int)first[446 + 6], 0xFF);
    ASSERT_EQ((int)first[446 + 7], 0xFF);
}

/* --------------------------------------------------------- sfdisk lines */

TEST(blocklayout, an_empty_script_line_fills_the_disk_from_one_mib)
{
    block_partition_t p;
    ASSERT_EQ((int)blockScriptLine(",,c", 7864320, 1, p), (int)PDI_OK);
    ASSERT_EQ(p.m_start, 2048u);
    ASSERT_EQ(p.m_blocks, 7864320u - 2048u);
    ASSERT_EQ((int)p.m_id, 0x0C);
    ASSERT_FALSE(p.m_boot);
}

TEST(blocklayout, a_script_line_takes_sizes_types_and_the_bootable_mark)
{
    block_partition_t p;
    ASSERT_EQ((int)blockScriptLine(",512M,c,*", 7864320, 1, p), (int)PDI_OK);
    ASSERT_EQ(p.m_start, 2048u);
    ASSERT_EQ(p.m_blocks, 1048576u);
    ASSERT_TRUE(p.m_boot);

    ASSERT_EQ((int)blockScriptLine(", +, L", 7864320, 1050624, p), (int)PDI_OK);
    ASSERT_EQ(p.m_start, 1050624u);
    ASSERT_EQ(p.m_blocks, 7864320u - 1050624u);
    ASSERT_EQ((int)p.m_id, 0x83);

    ASSERT_EQ((int)blockScriptLine("4096 1MiB 83", 7864320, 1, p), (int)PDI_OK);
    ASSERT_EQ(p.m_start, 4096u);
    ASSERT_EQ(p.m_blocks, 2048u);

    ASSERT_EQ((int)blockScriptLine(",", 7864320, 1, p), (int)PDI_OK);
    ASSERT_EQ((int)p.m_id, 0x83);
}

TEST(blocklayout, a_script_line_that_does_not_fit_is_refused_with_its_reason)
{
    block_partition_t p;
    ASSERT_EQ((int)blockScriptLine("8000000,,c", 7864320, 1, p), (int)PDI_ERR_RANGE);
    ASSERT_EQ((int)blockScriptLine(",8G,c", 7864320, 1, p), (int)PDI_ERR_RANGE);
    ASSERT_EQ((int)blockScriptLine("0,,c", 7864320, 1, p), (int)PDI_ERR_RANGE);
    ASSERT_EQ((int)blockScriptLine("2048,,c", 7864320, 4096, p), (int)PDI_ERR_RANGE);
    ASSERT_EQ((int)blockScriptLine(",,zz", 7864320, 1, p), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ((int)blockScriptLine(",10Q,c", 7864320, 1, p), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ((int)blockScriptLine(",0,c", 7864320, 1, p), (int)PDI_ERR_INVALID_ARG);
    ASSERT_EQ((int)blockScriptLine(",,c,x", 7864320, 1, p), (int)PDI_ERR_INVALID_ARG);
}

/* ------------------------------------------------ names and sizes shown */

TEST(blocklayout, partition_types_have_the_names_fdisk_gives_them)
{
    ASSERT_STREQ(blockPartitionTypeName(0x0C), "W95 FAT32 (LBA)");
    ASSERT_STREQ(blockPartitionTypeName(0x83), "Linux");
    ASSERT_STREQ(blockPartitionTypeName(0x42), "Unknown");
}

TEST(blocklayout, sizes_read_the_way_fdisk_prints_them)
{
    char out[16];
    BytesToSizeString(4026531840ULL, true, out, sizeof(out));
    ASSERT_STREQ(out, "3.8G");
    BytesToSizeString(4026531840ULL, false, out, sizeof(out));
    ASSERT_STREQ(out, "3.75 GiB");
    BytesToSizeString(536870912ULL, true, out, sizeof(out));
    ASSERT_STREQ(out, "512M");
    BytesToSizeString(1073741824ULL, false, out, sizeof(out));
    ASSERT_STREQ(out, "1 GiB");
    BytesToSizeString(500, true, out, sizeof(out));
    ASSERT_STREQ(out, "500B");
}

TEST(blocklayout, a_partition_keeps_its_number_when_the_slot_before_it_is_empty)
{
    Blocks b;
    b.partition(1, 0x00, 0x83, 4096, 8192);
    block_partition_t parts[BLOCK_PARTITIONS_MAX];
    uint8_t count = 0;
    ASSERT_EQ((int)blockTable(b.m_first, DISK_BLOCKS, parts, BLOCK_PARTITIONS_MAX, count), (int)BLOCK_TABLE_DOS);
    ASSERT_EQ((int)count, 2);
    ASSERT_EQ(parts[0].m_blocks, 0u);
    ASSERT_EQ(parts[1].m_start, 4096u);
    ASSERT_EQ(parts[1].m_blocks, 8192u);
}
