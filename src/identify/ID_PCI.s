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

		INCLUDE lvo/exec.i
		INCLUDE lvo/utility.i
		INCLUDE lvo/openpci.i

		INCLUDE libraries/identify.i
		INCLUDE libraries/openpci.i
		INCLUDE	pcireader.i

		INCLUDE ID_PCI.i
		INCLUDE ID_Locale.i

		IFD	_MAKE_68020
		  MACHINE 68020
		ENDC

		SECTION text,CODE

**
* Initializes the PCI module.
*
		public	InitPCI
InitPCI		movem.l d0-d1/a0,-(SP)
		movem.l (SP)+,d0-d1/a0
		rts

**
* Exits the PCI module.
*
		public	ExitPCI
ExitPCI		rts

**
* Fetches the name of an expansion from the database.
*
*	-> A0.l	^Tags
*	<- D0.l	Error code
*
		clrfo
ipci_TagItem	fo.l	1	; ^^TagItem for NextTagItem()
ipci_ManufStr	fo.l	1	; ^Manufacturer string
ipci_ProdStr	fo.l	1	; ^Product string
ipci_ClassStr	fo.l	1	; ^Class string
ipci_CurrentPci	fo.l	1	; ^Current PCIDev
ipci_StrLength	fo.w	1	; String length, including the terminator
ipci_ManufID	fo.w	1	; Manufacturer ID
ipci_ProdID	fo.w	1	; Product ID
ipci_ClassID	fo.w	1	; Class ID
ipci_GotID	fo.b	1	; Is it a valid ID for searching?
ipci_EmptyFlag	fo.b	1	; Empty strings for unknown manuf/board
ipci_SIZEOF	fo.w	0

		public	IdPciExpansion
IdPciExpansion	movem.l d1-d7/a0-a3/a5-a6,-(sp)
		link	a4,#ipci_SIZEOF
	;-- initialize local variables
		moveq	#(-ipci_SIZEOF/2)-1,d0
		move.l	a4,a1
.clear		clr	-(a1)
		dbra	d0,.clear
		move.l	a0,(ipci_TagItem,a4)
		move	#IDENTIFYBUFLEN,(ipci_StrLength,a4)
	;-- collect search parameters
.tagloop	lea	(ipci_TagItem,a4),a0
		utils	NextTagItem
		tst.l	d0
		beq	.tagdone
		move.l	d0,a0
		move.l	(a0)+,d0		; tag key
		move.l	(a0),d1			; tav value
		sub.l	#IDTAG_ManufID,d0			; IDTAG_ManufID ?
		beq	.manufid
		subq.l	#IDTAG_ProdID-IDTAG_ManufID,d0		; IDTAG_ProdID ?
		beq	.prodid
		subq.l	#IDTAG_StrLength-IDTAG_ProdID,d0	; IDTAG_StrLength ?
		beq	.strlength
		subq.l	#IDTAG_ManufStr-IDTAG_StrLength,d0	; IDTAG_ManufStr ?
		beq	.manufstr
		subq.l	#IDTAG_ProdStr-IDTAG_ManufStr,d0	; IDTAG_ProdStr ?
		beq	.prodstr
		subq.l	#IDTAG_ClassStr-IDTAG_ProdStr,d0	; IDTAG_ClassStr ?
		beq	.classstr
		subq.l	#IDTAG_Expansion-IDTAG_ClassStr,d0	; IDTAG_Expansion ?
		beq	.expansion
		subq.l	#IDTAG_ClassID-IDTAG_Expansion,d0	; IDTAG_ClassID?
		beq	.classid
		subq.l	#IDTAG_EmptyIfUnknown-IDTAG_ClassID,d0	; IDTAG_EmptyIfUnknown ?
		beq	.emptyflag
		subq.l	#IDTAG_PciDev-IDTAG_EmptyIfUnknown,d0	; IDTAG_PciDev?
		beq	.pcidev
		bra	.tagloop		; unknown tag, ignore it
	;-- set tags
.manufid	move	d1,(ipci_ManufID,a4)
		st	(ipci_GotID,a4)
		bra	.tagloop
.prodid		move	d1,(ipci_ProdID,a4)	; PCI: Product ID is UWORD!
		; intentionally no st (..)
		bra	.tagloop
.classid	move.b	d1,(ipci_ClassID+1,a4)
		bra	.tagloop
.strlength	tst	d1
		beq	.err_nolength
		move	d1,(ipci_StrLength,a4)
		bra	.tagloop
.manufstr	move.l	d1,(ipci_ManufStr,a4)
		bra	.tagloop
.prodstr	move.l	d1,(ipci_ProdStr,a4)
		bra	.tagloop
.classstr	move.l	d1,(ipci_ClassStr,a4)
		bra	.tagloop
.pcidev		tst.l	d1
		beq	.tagloop
		move.l	d1,a5
		move	(pci_vendor,a5),(ipci_ManufID,a4)
		move	(pci_device,a5),(ipci_ProdID,a4)
		move.b	(pci_devclass+1,a5),(ipci_ClassID+1,a4)
		st	(ipci_GotID,a4)
		bra	.tagloop
.emptyflag	tst.l	d1
		sne	(ipci_EmptyFlag,a4)
		bra	.tagloop
	;-- find next expansion
.expansion	tst.l	d1
		beq	.tagloop
		move.l	(openpcibase,PC),d0
		beq	.err_nopcilib
		move.l	d1,a2
		move.l	(a2),a0
		lea	(.pcitags,PC),a1
		movem.l	a2-a4,-(sp)
		pci	FindBoardTagList
		movem.l	(sp)+,a2-a4
		move.l	d0,(ipci_CurrentPci,a4)
		move.l	d0,(a2)
		beq	.err_done
		move.l	d0,a5
		move	(pci_vendor,a5),(ipci_ManufID,a4)
		move	(pci_device,a5),(ipci_ProdID,a4)
		move.b	(pci_devclass+1,a5),(ipci_ClassID+1,a4)
		st	(ipci_GotID,a4)
		bra	.tagloop
	;-- prepare search
.tagdone	tst.b	(ipci_GotID,a4)		; is there anything to search?
		beq	.err_badid
	;-- start search
		move	(ipci_ManufID,a4),d0
		move	(ipci_ProdID,a4),d1
		move.l	(ipci_ManufStr,a4),a0
		move	(ipci_StrLength,a4),d2
		move.l	(ipci_ProdStr,a4),a1
		bsr	_read_pci_database
		cmp.l	#PCI_BADVERSION,d0
		beq	.err_baddb
		cmp.l	#PCI_BADFILE,d0
		beq	.err_baddb
		cmp.l	#PCI_NODATABASE,d0
		beq	.err_nodb
		cmp.l	#PCI_PRODNOTFOUND,d0
		bne	.prodfound
		move.l	(ipci_ProdStr,a4),a0
		move	(ipci_ProdID,a4),d1
		move	(ipci_StrLength,a4),d2
		bsr	toHex
.prodfound	cmp.l	#PCI_MANUFNOTFOUND,d0
		bne	.manuffound
		move.l	(ipci_ManufStr,a4),a0
		move	(ipci_ManufID,a4),d1
		move	(ipci_StrLength,a4),d2
		bsr	toHex
	;-- pci class
.manuffound	move.l	(ipci_ClassStr,a4),d0
		beq	.noclass
		move.l	d0,a0
		move	(ipci_ClassID,a4),d0
		move	(ipci_StrLength,a4),d1
		bsr	copyClass
.noclass
	;-- done
.done		moveq	#IDERR_OKAY,d0
.exit		unlk	a4
		movem.l (sp)+,d1-d7/a0-a3/a5-a6
		rts

	;-- error
.err_nolength	moveq	#IDERR_NOLENGTH,d0	; buffer length is zero
		bra	.exit
.err_badid	moveq	#IDERR_BADID,d0		; bad or missing ID
		bra	.exit
.err_nodb	moveq	#IDERR_NOPCIDB,d0	; no pci.db file found
		bra	.exit
.err_baddb	moveq	#IDERR_BADPCIDB,d0	; bad or outdated pci.db file
		bra	.exit
.err_done	moveq	#IDERR_DONE,d0		; done
		bra	.exit
.err_nopcilib	moveq	#IDERR_NOPCILIB,d0	; no openpci.library found
		bra	.exit

	;-- openpci tags
.pcitags	dc.l	TAG_DONE


**
* Convert number to HEX.
*
*	-> A0.l	^Target String buffer, or NULL
*	-> A4.l	^local variables
*	-> D1.w	Hex number
*	-> D2.w Max buffer length, including terminator
*	<> D0.l PRESERVED
*
toHex		move.l	a0,d3			; Is there a target buffer?
		beq	.exit			;   no: do nothing
		tst.b	(ipci_EmptyFlag,a4)	; Empty if unknown?
		bne	.zeroterm		;   zero term only
		cmp	#1,d2			; 1 byte buffer?
		beq	.zeroterm		;   zero term only
		cmp	#5,d2			; minimum of 5 bytes?
		blt	.qmark			;   question mark only
		move.b	#"$",(a0)+
		moveq	#3,d2			; 4 iterations
		lea	(.hextable,PC),a1
.loop		rol	#4,d1
		move	d1,d3
		and	#$000f,d3
		move.b	(a1,d3.w),(a0)+
		dbra	d2,.loop
		bra	.zeroterm
.qmark		move.b	#"?",(a0)+
.zeroterm	clr.b	(a0)
.exit		rts
.hextable	dc.b	"0123456789ABCDEF"
		even


**
* Copy class to target buffer
*
*	-> A0.l	^Target String buffer, or NULL
*	-> A4.l	^local variables
*	-> D0.b	Class ID
*	-> D1.w Max buffer length, including terminator
*
copyClass	lea	pci_classes,a1
		and	#$ff,d0
		subq	#1,d1
.loop		move	(a1)+,d2
		bmi	.notfound
		cmp	d0,d2
		beq	.found
		addq.l	#4,a1
		bra	.loop
.notfound	tst.b	(ipci_EmptyFlag,a4)	; Empty if unknown?
		bne	.zeroterm		;   zero term only
		lea	(.msg,PC),a1
		bra	.copy
.found		move.l	(a1),a1
.copy		move.b	(a1)+,(a0)+
		dbeq	d1,.copy
		clr.b	-(a0)
		rts
.zeroterm	clr.b	(a0)
		rts
.msg		dc.b	"(unknown)",0
		even
