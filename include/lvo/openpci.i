
_PCIpci_bus                     EQU     -30
_PCIpci_find_device             EQU     -90
_PCIpci_find_class              EQU     -96
_PCIpci_find_slot               EQU     -102
_PCIpci_read_config_byte        EQU     -108
_PCIpci_read_config_word        EQU     -114
_PCIpci_read_config_long        EQU     -120
_PCIpci_write_config_byte       EQU     -126
_PCIpci_write_config_word       EQU     -132
_PCIpci_write_config_long       EQU     -138
_PCIpci_set_master              EQU     -144
_PCIpci_add_intserver           EQU     -150
_PCIpci_rem_intserver           EQU     -156
_PCIpci_allocdma_mem            EQU     -162
_PCIpci_freedma_mem             EQU     -168
_PCIpci_logic_to_physic_addr    EQU     -174
_PCIpci_physic_to_logic_addr    EQU     -180
_PCIpci_obtain_card             EQU     -186
_PCIpci_release_card            EQU     -192
_PCIFindBoardTagList            EQU     -204
_PCIGetBoardAttrsA              EQU     -210
_PCISetBoardAttrsA              EQU     -216
_PCIAllocateDMAMemoryForBoard   EQU     -222
_PCIReleaseDMAMemoryForBoard    EQU     -228
_PCIAddMemoryHandlerForBoard    EQU     -234
_PCIRemMemoryHandlerForBoard    EQU     -240
_PCIObtainPCIRegion             EQU     -246
_PCIReleasePCIRegion            EQU     -252

pci		MACRO
		IFNC	"\0","q"
		 move.l	openpcibase(PC),a6
		ENDC
		jsr	_PCI\1(a6)
		ENDM
