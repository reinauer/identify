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
#include <exec/types.h>
#include <libraries/identify.h>
#include <libraries/openpci.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/identify.h>


struct Library *IdentifyBase;

LONG read_pci_database(
  __reg("d0") UWORD manufId,
  __reg("d1") UWORD prodId,
  __reg("a0") STRPTR manufName,
  __reg("a1") STRPTR prodName,
  __reg("d2") UWORD maxLen
);


int main(void) {
  UBYTE manufacturerName[128] = {0};
  UBYTE productName[128] = {0};

  LONG result = read_pci_database(0x144d, 0xaa00, (STRPTR) &manufacturerName, (STRPTR) &productName, 128);

  Printf("Manufacturer: %s\n", manufacturerName);
  Printf("Product     : %s\n", productName);
  Printf("Return Code : %ld\n", result);



  if(IdentifyBase = OpenLibrary("identify.library", IDENTIFYVERSION))
  {
    char manuf[IDENTIFYBUFLEN];
    char prod[IDENTIFYBUFLEN];
    char pclass[IDENTIFYBUFLEN];

    manuf[0] = 0;
    prod[0] = 0;
    pclass[0] = 0;

    struct pci_dev *expans = NULL;

    struct pci_dev pd;
    pd.vendor = 0x0e11;
    pd.device = 0xb060;
    pd.devclass = 0x000a0000;

    LONG result = IdPciExpansionTags(
      IDTAG_ManufID  , 0x0e11,
      IDTAG_ProdID   , 0x5678,
      IDTAG_ClassID  , 0x0a,
      IDTAG_ManufStr , &manuf,
      IDTAG_ProdStr  , &prod,
      IDTAG_ClassStr , &pclass,
  //    IDTAG_PciDev   , &pd,
  //    IDTAG_Expansion, &expans,
      TAG_DONE);

    Printf("-- RESULT --\n");
    Printf("Manuf: %s\n", manuf);
    Printf("Prod:  %s\n", prod);
    Printf("Class: %s\n", pclass);
    Printf("RC:    %ld\n", result);

    CloseLibrary(IdentifyBase);
  }

  return 0;
}
