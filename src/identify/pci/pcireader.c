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

/*
 * Read a word.
 */
static LONG read_word(BPTR fh) {
  UBYTE buffer[2];

  LONG len = Read(fh, &buffer, 2);
  if (len == 2) {
    return (buffer[0] << 8) + (buffer[1]);
  } else {
    return -1;
  }
}

/*
 * Read a file offset.
 */
static LONG read_offset(BPTR fh) {
  UBYTE buffer[4];

  LONG len = Read(fh, &buffer, 4);
  if (len == 4) {
    return (buffer[0] << 24) + (buffer[1] << 16) + (buffer[2] << 8) + (buffer[3]);
  } else {
    return -1;
  }
}

/*
 * Read a null-terminated string to the given target, taking care not to exceed maxlen.
 * The read string is guaranteed to be null-terminated, unless maxlen is 0.
 * If target is NULL, nothing will be read.
 */
static LONG read_string(BPTR fh, STRPTR target, UWORD maxlen) {
  LONG ch;
  if (maxlen == 0 || target == NULL) {
    return 0;
  }
  while ((--maxlen > 0) && ((ch = FGetC(fh)) > 0)) {
    *target = (BYTE) ch;
    target++;
  }
  *target = (BYTE) 0;
  if (ch > 0) {  // maxlen was reached, simulate OK
    ch = 0;
  }
  return ch;
}

/*
 * Seek to the given position, related to the beginning of the file.
 */
static LONG seek(BPTR fh, LONG pos) {
  Seek(fh, pos, OFFSET_BEGINNING);
  return IoErr();
}

/*
 * Locate the manufacturer by binary search.
 */
static LONG locate_manufacturer(BPTR fh, LONG manCount, UWORD manufId) {
  LONG min = 0;
  LONG max = manCount - 1;
  LONG current, chkManuf;

  while (min <= max) {
    current = min + (max - min) / 2;

    if (seek(fh, (current * BYTES_PER_MANUF) + OFFSET_MANUF_TABLE) != 0) {
      return -1;
    }

    if ((chkManuf = read_word(fh)) < 0) {
      return -1;
    }

    if (chkManuf == manufId) {
      return read_offset(fh);
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
 static LONG locate_product(BPTR fh, LONG manOffset, LONG prodCount, UWORD prodId) {
  LONG min = 0;
  LONG max = prodCount - 1;
  LONG current, chkProd;

  while (min <= max) {
    current = min + (max - min) / 2;

    if (seek(fh, (current * BYTES_PER_PROD) + manOffset + OFFSET_PROD_TABLE) != 0) {
      return -1;
    }

    if ((chkProd = read_word(fh)) < 0) {
      return -1;
    }

    if (chkProd == prodId) {
      return read_offset(fh);
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

  LONG rc = ERROR_BADFILE;
  do {
    LONG version = read_word(fh);
    if (version < 0) {
      break;
    }
    if (version != DATABASE_VERSION) {
      rc = ERROR_BADVERSION;
      break;
    }

    LONG manCount = read_word(fh);
    if (manCount <= 0) {
      break;
    }

    LONG manOffset = locate_manufacturer(fh, manCount, manufId);
    if (manOffset < 0) {
      break;
    }
    if (manOffset == 0) {
      rc = ERROR_MANUFNOTFOUND;
      break;
    }

    if (seek(fh, manOffset) < 0) {
      break;
    }

    LONG manufNameOffset = read_offset(fh);
    if (manufNameOffset < 0) {
      break;
    }

    LONG prodCount = read_word(fh);
    if (prodCount < 0) {
      break;
    }

    if (seek(fh, manufNameOffset) < 0) {
      break;
    }
    if (read_string(fh, manufName, maxLen) < 0) {
      break;
    }

    LONG prodNameOffset = locate_product(fh, manOffset, prodCount, prodId);
    if (prodNameOffset < 0) {
      break;
    }
    if (prodNameOffset == 0) {
      rc = ERROR_PRODNOTFOUND;
      break;
    }

    if (seek(fh, prodNameOffset) < 0) {
      break;
    }
    if (read_string(fh, prodName, maxLen) < 0) {
      break;
    }

    rc = SUCCESS;
  } while (FALSE);

  Close(fh);
  return rc;
}
