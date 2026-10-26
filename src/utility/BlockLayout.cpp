/******************************** Block Layout *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 21st Oct 2026
******************************************************************************/

#include "BlockLayout.h"
#include "DataTypeConversions.h"

namespace {

const uint16_t s_boot_signature_at = 510;
const uint16_t s_table_at = 446;
const uint8_t s_entry_size = 16;
const uint8_t s_oem_at = 3;
const uint8_t s_oem_size = 8;
const uint8_t s_littlefs_at = 8;
const uint8_t s_ext_magic_at = 56;
const uint8_t s_ext_compat_at = 92;
const uint8_t s_ext_incompat_at = 96;
const uint32_t s_ext_has_journal = 0x0004;
const uint32_t s_ext4_incompat = 0x0040 | 0x0080 | 0x0200;
const uint8_t s_gpt_protective = 0xEE;
const uint16_t s_disk_id_at = 440;
const uint16_t s_fat32_type_at = 82;
const uint16_t s_fat_type_at = 54;
const uint8_t s_littlefs_size = 8;
const uint8_t s_chs_heads = 255;
const uint8_t s_chs_sectors = 63;
const uint16_t s_chs_cylinders_max = 1023;
const uint8_t s_type_linux = 0x83;
const uint8_t s_status_boot = 0x80;

/**
 * Whether the block ends with the boot signature.
 */
bool signed55AA(const uint8_t *first)
{
  return 0x55 == first[s_boot_signature_at] && 0xAA == first[s_boot_signature_at + 1];
}

/**
 * The cylinder, head and sector a dos entry carries for a block, the most it
 * can hold for one past it.
 */
void putChs(uint8_t *at, uint32_t block)
{
  uint32_t cylinder = block / ((uint32_t)s_chs_heads * s_chs_sectors);
  uint8_t head = (uint8_t)((block / s_chs_sectors) % s_chs_heads);
  uint8_t sector = (uint8_t)(block % s_chs_sectors + 1);
  if( cylinder > s_chs_cylinders_max ){
    cylinder = s_chs_cylinders_max;
    head = s_chs_heads - 1;
    sector = s_chs_sectors;
  }
  at[0] = head;
  at[1] = (uint8_t)((sector & 0x3F) | ((cylinder >> 2) & 0xC0));
  at[2] = (uint8_t)cylinder;
}

/**
 * A whole number from the text, read up to its first non digit; false when there is
 * none or it does not fit.
 */
bool readNumber(const char *&at, const char *end, uint64_t &value)
{
  const char *start = at;
  value = 0;
  while( at < end && *at >= '0' && *at <= '9' ){
    uint64_t next = value * 10 + (uint64_t)(*at - '0');
    if( next / 10 != value ){
      return false;
    }
    value = next;
    at++;
  }
  return at != start;
}

/**
 * An sfdisk field as blocks: a plain number, or bytes with a K, M, G or T suffix;
 * false when it is neither.
 */
bool readBlocksField(const char *at, const char *end, uint64_t &blocks)
{
  uint64_t value = 0;
  if( !readNumber(at, end, value) ){
    return false;
  }
  if( at == end ){
    blocks = value;
    return true;
  }
  uint8_t shift = 0;
  switch( *at ){
    case 'K': case 'k': shift = 10; break;
    case 'M': case 'm': shift = 20; break;
    case 'G': case 'g': shift = 30; break;
    case 'T': case 't': shift = 40; break;
    default: return false;
  }
  at++;
  if( end - at == 2 && 'i' == at[0] && 'B' == at[1] ){
    at += 2;
  }
  if( at != end || value > (UINT64_MAX >> shift) ){
    return false;
  }
  blocks = (value << shift) / SD_BLOCK_SIZE;
  return true;
}

/**
 * Trims the spaces around a field.
 */
void trimField(const char *&start, const char *&end)
{
  while( start < end && (' ' == *start || '\t' == *start) ) start++;
  while( end > start && (' ' == end[-1] || '\t' == end[-1] || '\r' == end[-1]) ) end--;
}

/**
 * Whether the first block is a FAT boot sector, judged by its signature, jump,
 * sector, cluster and FAT fields.
 */
bool fatBootSector(const uint8_t *first)
{
  bool jump = (0xEB == first[0] && 0x90 == first[2]) || 0xE9 == first[0];
  uint16_t sector = BytesToUint16LE(first + 11);
  uint8_t cluster = first[13];
  uint16_t reserved = BytesToUint16LE(first + 14);
  uint8_t fats = first[16];
  uint8_t media = first[21];
  return signed55AA(first) && jump && (512 == sector || 1024 == sector || 2048 == sector || 4096 == sector) &&
         0 != cluster && 0 == (cluster & (cluster - 1)) && 0 != reserved &&
         (1 == fats || 2 == fats) && (0xF0 == media || media >= 0xF8);
}

}

/**
 * The filesystem the first and third 512 byte blocks of a device name.
 */
uint8_t blockFsType(const uint8_t *first, const uint8_t *third)
{
  if( nullptr == first ){
    return BLOCK_FS_NONE;
  }

  if( 0 == memcmp(first + s_oem_at, "EXFAT   ", s_oem_size) ){
    return BLOCK_FS_EXFAT;
  }
  if( 0 == memcmp(first + s_oem_at, "NTFS    ", s_oem_size) ){
    return BLOCK_FS_NTFS;
  }
  if( fatBootSector(first) ){
    return BLOCK_FS_VFAT;
  }
  if( 0 == memcmp(first + s_littlefs_at, "littlefs", 8) ){
    return BLOCK_FS_LITTLEFS;
  }

  if( nullptr != third && 0x53 == third[s_ext_magic_at] && 0xEF == third[s_ext_magic_at + 1] ){
    if( 0 != (BytesToUint32LE(third + s_ext_incompat_at) & s_ext4_incompat) ){
      return BLOCK_FS_EXT4;
    }
    return (0 != (BytesToUint32LE(third + s_ext_compat_at) & s_ext_has_journal)) ? BLOCK_FS_EXT3 : BLOCK_FS_EXT2;
  }

  return BLOCK_FS_NONE;
}

/**
 * The partition table the first block holds and its partitions that fit the device,
 * each at its linux slot.
 */
uint8_t blockTable(const uint8_t *first, uint32_t blocks, block_partition_t *out, uint8_t max, uint8_t &count)
{
  count = 0;
  if( nullptr == first || nullptr == out || !signed55AA(first) || BLOCK_FS_NONE != blockFsType(first, nullptr) ){
    return BLOCK_TABLE_NONE;
  }

  for( uint8_t i = 0; i < BLOCK_PARTITIONS_MAX; i++ ){
    const uint8_t *entry = first + s_table_at + i * s_entry_size;
    if( 0x00 != entry[0] && 0x80 != entry[0] ){
      count = 0;
      return BLOCK_TABLE_NONE;
    }
    if( 0 == entry[4] ){
      continue;
    }
    if( s_gpt_protective == entry[4] ){
      count = 0;
      return BLOCK_TABLE_GPT;
    }
    uint32_t start = BytesToUint32LE(entry + 8);
    uint32_t length = BytesToUint32LE(entry + 12);
    if( 0 == start || 0 == length || start >= blocks || length > blocks - start || i >= max ){
      continue;
    }
    for( uint8_t gap = count; gap < i; gap++ ){
      out[gap] = block_partition_t();
    }
    out[i].m_start = start;
    out[i].m_blocks = length;
    out[i].m_id = entry[4];
    out[i].m_fs = BLOCK_FS_NONE;
    out[i].m_boot = (s_status_boot == entry[0]);
    count = (uint8_t)(i + 1);
  }

  return BLOCK_TABLE_DOS;
}

/**
 * The signatures blkid would name in one 512 byte block, as wipefs finds them; how
 * many were put in out.
 */
uint8_t blockSignatures(const uint8_t *block, uint32_t index, block_signature_t *out, uint8_t max)
{
  uint8_t count = 0;
  if( nullptr == block || nullptr == out ){
    return 0;
  }

  uint16_t offsets[BLOCK_SIGNATURES_MAX];
  uint8_t sizes[BLOCK_SIGNATURES_MAX];
  const char *names[BLOCK_SIGNATURES_MAX];

  if( 0 == index ){
    uint8_t fs = blockFsType(block, nullptr);
    if( BLOCK_FS_VFAT == fs ){
      bool fat32 = (0 == memcmp(block + s_fat32_type_at, "FAT32   ", s_oem_size));
      offsets[count] = fat32 ? s_fat32_type_at : s_fat_type_at;
      sizes[count] = s_oem_size;
      names[count++] = BLOCK_FS_NAME_VFAT;
    }else if( BLOCK_FS_EXFAT == fs || BLOCK_FS_NTFS == fs ){
      offsets[count] = s_oem_at;
      sizes[count] = s_oem_size;
      names[count++] = (BLOCK_FS_EXFAT == fs) ? BLOCK_FS_NAME_EXFAT : BLOCK_FS_NAME_NTFS;
    }
    if( count > 0 ){
      offsets[count] = s_boot_signature_at;
      sizes[count] = 2;
      names[count] = names[count - 1];
      count++;
    }else{
      block_partition_t parts[BLOCK_PARTITIONS_MAX];
      uint8_t listed = 0;
      uint8_t table = blockTable(block, UINT32_MAX, parts, BLOCK_PARTITIONS_MAX, listed);
      if( BLOCK_TABLE_NONE != table ){
        offsets[count] = s_boot_signature_at;
        sizes[count] = 2;
        names[count++] = (BLOCK_TABLE_GPT == table) ? BLOCK_TABLE_NAME_GPT : BLOCK_TABLE_NAME_DOS;
      }
    }
  }

  if( 2 == index && 0x53 == block[s_ext_magic_at] && 0xEF == block[s_ext_magic_at + 1] ){
    offsets[count] = s_ext_magic_at;
    sizes[count] = 2;
    if( 0 != (BytesToUint32LE(block + s_ext_incompat_at) & s_ext4_incompat) ){
      names[count++] = BLOCK_FS_NAME_EXT4;
    }else{
      names[count++] = (0 != (BytesToUint32LE(block + s_ext_compat_at) & s_ext_has_journal)) ? BLOCK_FS_NAME_EXT3 : BLOCK_FS_NAME_EXT2;
    }
  }

  if( 0 == memcmp(block + s_littlefs_at, "littlefs", s_littlefs_size) ){
    offsets[count] = s_littlefs_at;
    sizes[count] = s_littlefs_size;
    names[count++] = BLOCK_FS_NAME_LITTLEFS;
  }

  uint8_t given = 0;
  for( uint8_t i = 0; i < count && given < max; i++, given++ ){
    out[given].m_block = index;
    out[given].m_offset = offsets[i];
    out[given].m_size = sizes[i];
    out[given].m_name = names[i];
  }
  return given;
}

/**
 * Writes a dos partition table into the first block, keeping the boot code ahead of
 * it.
 */
void blockTableBuild(uint8_t *first, const block_partition_t *parts, uint8_t count, uint32_t diskid)
{
  if( nullptr == first ){
    return;
  }
  memset(first + s_disk_id_at, 0, SD_BLOCK_SIZE - s_disk_id_at);
  Uint32ToBytesLE(diskid, first + s_disk_id_at);
  for( uint8_t i = 0; nullptr != parts && i < count && i < BLOCK_PARTITIONS_MAX; i++ ){
    if( 0 == parts[i].m_blocks ){
      continue;
    }
    uint8_t *entry = first + s_table_at + i * s_entry_size;
    entry[0] = parts[i].m_boot ? s_status_boot : 0x00;
    putChs(entry + 1, parts[i].m_start);
    entry[4] = parts[i].m_id;
    putChs(entry + 5, parts[i].m_start + parts[i].m_blocks - 1);
    Uint32ToBytesLE(parts[i].m_start, entry + 8);
    Uint32ToBytesLE(parts[i].m_blocks, entry + 12);
  }
  first[s_boot_signature_at] = 0x55;
  first[s_boot_signature_at + 1] = 0xAA;
}

/**
 * The identifier a dos partition table carries.
 */
uint32_t blockDiskId(const uint8_t *first)
{
  return (nullptr == first) ? 0 : BytesToUint32LE(first + s_disk_id_at);
}

/**
 * Reads one sfdisk script line, "start, size, type, bootable", empty fields taking
 * defaults; PDI_OK or the error.
 */
pdi_err_t blockScriptLine(const char *line, uint32_t blocks, uint32_t from, block_partition_t &out)
{
  if( nullptr == line ){
    return PDI_ERR_NULL_PTR;
  }

  const char *fields[4] = {nullptr, nullptr, nullptr, nullptr};
  const char *ends[4] = {nullptr, nullptr, nullptr, nullptr};
  const char *end = line + strlen(line);
  bool commas = (nullptr != memchr(line, ',', (size_t)(end - line)));
  const char *at = line;
  uint8_t n = 0;
  while( n < 4 ){
    const char *stop = at;
    if( commas ){
      while( stop < end && ',' != *stop ) stop++;
    }else{
      while( at < end && (' ' == *at || '\t' == *at) ) at++;
      stop = at;
      while( stop < end && ' ' != *stop && '\t' != *stop ) stop++;
    }
    fields[n] = at;
    ends[n] = stop;
    trimField(fields[n], ends[n]);
    n++;
    if( stop >= end ){
      break;
    }
    at = stop + (commas ? 1 : 0);
  }
  if( commas && at < end && n == 4 && nullptr != memchr(at, ',', (size_t)(end - at)) ){
    return PDI_ERR_INVALID_ARG;
  }

  uint64_t start = 0;
  if( nullptr == fields[0] || fields[0] == ends[0] ){
    uint64_t base = (from > 0) ? from : 1;
    start = ((base + BLOCK_ALIGN_SECTORS - 1) / BLOCK_ALIGN_SECTORS) * BLOCK_ALIGN_SECTORS;
  }else if( !readBlocksField(fields[0], ends[0], start) ){
    return PDI_ERR_INVALID_ARG;
  }
  if( 0 == start || start < from || start >= blocks ){
    return PDI_ERR_RANGE;
  }

  uint64_t size = 0;
  const char *sizeat = fields[1];
  if( nullptr != sizeat && sizeat < ends[1] && '+' == *sizeat ){
    sizeat++;
  }
  if( nullptr == sizeat || sizeat == ends[1] ){
    size = (uint64_t)blocks - start;
  }else if( !readBlocksField(sizeat, ends[1], size) ){
    return PDI_ERR_INVALID_ARG;
  }
  if( 0 == size ){
    return PDI_ERR_INVALID_ARG;
  }
  if( size > (uint64_t)blocks - start ){
    return PDI_ERR_RANGE;
  }

  uint8_t id = s_type_linux;
  if( nullptr != fields[2] && fields[2] != ends[2] ){
    uint32_t typelen = (uint32_t)(ends[2] - fields[2]);
    char letter = fields[2][0];
    if( 1 == typelen && ('L' == letter || 'S' == letter || 'E' == letter || 'U' == letter) ){
      id = ('L' == letter) ? 0x83 : ('S' == letter) ? 0x82 : ('E' == letter) ? 0x05 : 0xEF;
    }else{
      if( typelen > 2 ){
        return PDI_ERR_INVALID_ARG;
      }
      uint16_t value = 0;
      for( const char *c = fields[2]; c < ends[2]; c++ ){
        uint8_t digit = 0;
        if( *c >= '0' && *c <= '9' ) digit = (uint8_t)(*c - '0');
        else if( *c >= 'a' && *c <= 'f' ) digit = (uint8_t)(*c - 'a' + 10);
        else if( *c >= 'A' && *c <= 'F' ) digit = (uint8_t)(*c - 'A' + 10);
        else return PDI_ERR_INVALID_ARG;
        value = (uint16_t)(value * 16 + digit);
      }
      if( 0 == value ){
        return PDI_ERR_INVALID_ARG;
      }
      id = (uint8_t)value;
    }
  }

  bool boot = false;
  if( nullptr != fields[3] && fields[3] != ends[3] ){
    if( 1 != ends[3] - fields[3] || ('*' != fields[3][0] && '-' != fields[3][0]) ){
      return PDI_ERR_INVALID_ARG;
    }
    boot = ('*' == fields[3][0]);
  }

  out.m_start = (uint32_t)start;
  out.m_blocks = (uint32_t)size;
  out.m_id = id;
  out.m_fs = BLOCK_FS_NONE;
  out.m_boot = boot;
  return PDI_OK;
}

/**
 * The name of a disk's partition, e.g. mmcblk0p1 for partition 1 of mmcblk0.
 */
pdiutil::string blockPartitionName(const char *disk, uint8_t number)
{
  char digits[4] = {0};
  Uint32ToString(number, digits, sizeof(digits));
  pdiutil::string name = (nullptr == disk) ? "" : disk;
  name += 'p';
  name += digits;
  return name;
}

/**
 * The name fdisk gives a partition type id, a read only string.
 */
const char *blockPartitionTypeName(uint8_t id)
{
  switch( id ){
    case 0x01: return RODT_ATTR("FAT12");
    case 0x04: return RODT_ATTR("FAT16 <32M");
    case 0x05: return RODT_ATTR("Extended");
    case 0x06: return RODT_ATTR("FAT16");
    case 0x07: return RODT_ATTR("HPFS/NTFS/exFAT");
    case 0x0B: return RODT_ATTR("W95 FAT32");
    case 0x0C: return RODT_ATTR("W95 FAT32 (LBA)");
    case 0x0E: return RODT_ATTR("W95 FAT16 (LBA)");
    case 0x0F: return RODT_ATTR("W95 Ext'd (LBA)");
    case 0x82: return RODT_ATTR("Linux swap / Solaris");
    case 0x83: return RODT_ATTR("Linux");
    case 0xEE: return RODT_ATTR("GPT");
    case 0xEF: return RODT_ATTR("EFI (FAT-12/16/32)");
    default:   return RODT_ATTR("Unknown");
  }
}

