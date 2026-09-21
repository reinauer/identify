/*
 * identify.library
 *
 * Copyright (C) 2025 Richard "Shred" Koerber
 *        http://identify.shredzone.org
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <clib/dos_protos.h>
#include <pragmas/dos_pragmas.h>
#include <exec/types.h>
#include <dos/dos.h>


#define SUCCESS             0
#define ERROR_MANUFNOTFOUND -1
#define ERROR_PRODNOTFOUND  -2
#define ERROR_BADVERSION    -3
#define ERROR_BADFILE       -4
#define ERROR_NODATABASE    -5


#define DATABASE_VERSION   1

#define OFFSET_VERSION     0
#define OFFSET_NUM_MANUF   1
#define OFFSET_MANUF_TABLE 4
#define OFFSET_PROD_TABLE  6
#define BYTES_PER_MANUF    6
#define BYTES_PER_PROD     6

/* A per-lookup cache keeps stack use small and avoids DOS packets per byte.
 * All positions are ours: never mix DOS buffered I/O with Read/Seek. Aligning
 * refills also lets nearby binary-search probes reuse the same data. */
#define READ_BUFFER_SIZE 256
struct reader {
  BPTR fh;
  LONG position, start, length;
  UBYTE buffer[READ_BUFFER_SIZE];
};

static LONG read_byte(struct reader *r) {
  LONG offset = r->position - r->start;
  if (offset < 0 || offset >= r->length) {
    r->start = r->position & ~(READ_BUFFER_SIZE - 1);
    r->length = 0;
    if (Seek(r->fh, r->start, OFFSET_BEGINNING) == -1) return -1;
    r->length = Read(r->fh, r->buffer, READ_BUFFER_SIZE);
    offset = r->position - r->start;
    if (offset >= r->length) return -1;
  }
  ++r->position;
  return r->buffer[offset];
}

static LONG read_word(struct reader *r) {
  LONG hi = read_byte(r), lo = read_byte(r);
  return hi < 0 || lo < 0 ? -1 : (hi << 8) | lo;
}

static LONG read_offset(struct reader *r) {
  LONG hi = read_word(r), lo = read_word(r);
  /* File offsets must fit DOS's signed LONG. */
  return hi < 0 || hi > 32767 || lo < 0 ? -1 : (hi << 16) | lo;
}

/* Always terminate a nonempty destination, including a one-byte buffer. */
static LONG read_string(struct reader *r, STRPTR target, UWORD maxlen) {
  LONG ch = 0;
  if (maxlen == 0 || target == NULL) return 0;
  while (--maxlen > 0) {
    ch = read_byte(r);
    if (ch <= 0) break;
    *target++ = (BYTE)ch;
  }
  *target = 0;
  return ch < 0 ? -1 : 0;
}

static LONG seek(struct reader *r, LONG pos) {
  if (pos < 0) return -1;
  r->position = pos;
  return 0;
}

/*
 * Locate the manufacturer by binary search.
 */
static LONG locate_manufacturer(struct reader *r, LONG manCount, UWORD manufId) {
  LONG min = 0;
  LONG max = manCount - 1;
  LONG current, chkManuf;

  while (min <= max) {
    current = min + ((ULONG)(max - min) >> 1);

    if (seek(r, (current * BYTES_PER_MANUF) + OFFSET_MANUF_TABLE) != 0) {
      return -1;
    }

    if ((chkManuf = read_word(r)) < 0) {
      return -1;
    }

    if (chkManuf == manufId) {
      return read_offset(r);
    }

    if (chkManuf < manufId) {
      min = current + 1;
    } else {
      max = current - 1;
    }
  }

  return 0;
}

/*
 * Locate the product by binary search.
 */
 static LONG locate_product(struct reader *r, LONG manOffset, LONG prodCount, UWORD prodId) {
  LONG min = 0;
  LONG max = prodCount - 1;
  LONG current, chkProd;

  while (min <= max) {
    current = min + ((ULONG)(max - min) >> 1);

    if (seek(r, (current * BYTES_PER_PROD) + manOffset + OFFSET_PROD_TABLE) != 0) {
      return -1;
    }

    if ((chkProd = read_word(r)) < 0) {
      return -1;
    }

    if (chkProd == prodId) {
      return read_offset(r);
    }

    if (chkProd < prodId) {
      min = current + 1;
    } else {
      max = current - 1;
    }
  }

  return 0;
}

/*
 * Read from the PCI database file.
 *
 * @param manufId          Manufacturer ID to read
 * @param prodId           Product ID to read
 * @param manufName        Buffer to write the manufacturer name to, can be null
 * @param prodName         Buffer to write the product name to, can be null
 * @param maxLen           Length of the buffers, including terminator
 * @return Success code, see constants above
 */
__saveds LONG read_pci_database(
  __reg("d0") UWORD manufId,
  __reg("d1") UWORD prodId,
  __reg("a0") STRPTR manufName,
  __reg("a1") STRPTR prodName,
  __reg("d2") UWORD maxLen
)
{
  BPTR fh = Open("S:pci.db", MODE_OLDFILE);
  if (fh == 0) {
    return ERROR_NODATABASE;
  }

  struct reader state, *r = &state;
  r->fh = fh;
  r->position = r->start = r->length = 0;
  LONG rc = ERROR_BADFILE;
  do {
    LONG version = read_word(r);
    if (version < 0) {
      break;
    }
    if (version != DATABASE_VERSION) {
      rc = ERROR_BADVERSION;
      break;
    }

    LONG manCount = read_word(r);
    if (manCount <= 0) {
      break;
    }

    LONG manOffset = locate_manufacturer(r, manCount, manufId);
    if (manOffset < 0) {
      break;
    }
    if (manOffset == 0) {
      rc = ERROR_MANUFNOTFOUND;
      break;
    }

    if (seek(r, manOffset) < 0) {
      break;
    }

    LONG manufNameOffset = read_offset(r);
    if (manufNameOffset < 0) {
      break;
    }

    LONG prodCount = read_word(r);
    if (prodCount < 0) {
      break;
    }

    if (seek(r, manufNameOffset) < 0) {
      break;
    }
    if (read_string(r, manufName, maxLen) < 0) {
      break;
    }

    LONG prodNameOffset = locate_product(r, manOffset, prodCount, prodId);
    if (prodNameOffset < 0) {
      break;
    }
    if (prodNameOffset == 0) {
      rc = ERROR_PRODNOTFOUND;
      break;
    }

    if (seek(r, prodNameOffset) < 0) {
      break;
    }
    if (read_string(r, prodName, maxLen) < 0) {
      break;
    }

    rc = SUCCESS;
  } while (FALSE);

  Close(fh);
  return rc;
}
