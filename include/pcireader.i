*
* identify.library
*
* Copyright (C) 2025 Richard "Shred" Koerber
*	http://identify.shredzone.org
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
*

		IFND	PCIREADER_I
PCIREADER_I	SET	1

PCI_SUCCESS		EQU	0	; No error
PCI_MANUFNOTFOUND	EQU	-1	; Unknown manufacturer
PCI_PRODNOTFOUND	EQU	-2	; Known manufacturer, unknown product
PCI_BADVERSION		EQU	-3	; Bad version of S:pci.db
PCI_BADFILE		EQU	-4	; S:pci.db seems to be corrupted
PCI_NODATABASE		EQU	-5	; No S:pci.db found

		ENDC
