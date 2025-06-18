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

/*
 * This file contains a few internal tests related to the PCI database. You
 * can use it as an example, but please bear in mind that this code is rather
 * meant for testing purposes only.
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

/*
 * This is a test for accessing the internal database file. Do not use in your code!
 */
static void testDatabaseAccess()
{
  static char manufacturerName[128] = {0};
  static char productName[128] = {0};

  LONG result = read_pci_database(0x144d, 0xaa00, (STRPTR) &manufacturerName, (STRPTR) &productName, 128);

  Printf("Manufacturer: %s\n", manufacturerName);
  Printf("Product     : %s\n", productName);
  Printf("Return Code : %ld\n", result);
}

/*
 * This is a test for accessing the database via identify API.
 */
static void testIdentify()
{
  static char buf_manuf[IDENTIFYBUFLEN] = {0};
  static char buf_product[IDENTIFYBUFLEN] = {0};
  static char buf_class[IDENTIFYBUFLEN] = {0};

  /* FIND A FIXED MANUFACTURER / PRODUCT ID */
/**/
  LONG result = IdPciExpansionTags(
    IDTAG_ManufID  , 0x0e11,
    IDTAG_ProdID   , 0x5678,
    IDTAG_ClassID  , 0x0a,
    IDTAG_ManufStr , buf_manuf,
    IDTAG_ProdStr  , buf_product,
    IDTAG_ClassStr , buf_class,
    TAG_DONE);
/**/

  /* FIND FROM A PCI_DEV STRUCTURE */
/*
  struct pci_dev pd;          // Fake pci_dev structure by filling it
  pd.vendor = 0x0e11;         // only with the necessary values.
  pd.device = 0xb060;
  pd.devclass = 0x000a0000;

  LONG result = IdPciExpansionTags(
    IDTAG_ManufStr , buf_manuf,
    IDTAG_ProdStr  , buf_product,
    IDTAG_ClassStr , buf_class,
    IDTAG_PciDev   , &pd,
    TAG_DONE);
*/

  /* READ ALL PRESENT PCI EXPANSIONS (REQUIRES A PCI BRIDGEBOARD) */
/*
  struct pci_dev *expans = NULL;

  LONG result = IdPciExpansionTags(
    IDTAG_ManufStr , buf_manuf,
    IDTAG_ProdStr  , buf_product,
    IDTAG_ClassStr , buf_class,
    IDTAG_Expansion, &expans,
    TAG_DONE);
*/

  Printf("-- RESULT --\n");
  Printf("Manuf: %s\n", buf_manuf);
  Printf("Prod:  %s\n", buf_product);
  Printf("Class: %s\n", buf_class);
  Printf("RC:    %ld\n", result);
}

int main(void) {
  testDatabaseAccess();

  if(IdentifyBase = OpenLibrary("identify.library", IDENTIFYVERSION))
  {
    testIdentify();
    CloseLibrary(IdentifyBase);
  }

  return 0;
}
