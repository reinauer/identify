#!/usr/bin/env python3
#
# identify.library
#
# Copyright (C) 2025 Richard "Shred" Koerber
#	http://identify.shredzone.org
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

#
# This tool updates the PCI database.
#

import re

header = """*
* identify.library
*
* === WARNING: AUTO-GENERATED FILE! MANUAL CHANGES WILL BE LOST! ===
*
* This file bases on https://github.com/pciutils/pciids and was
* automatically converted by update-pci.py .
*
"""

# Exclude AMD and Intel, it's a huge table of CPUs and chipsets that is
# very unlikely to be found in Amiga extensions.
excludes = [0x1022, 0x8086]


currentManuf = None
manufacturers = {}
products = {}
classes = {}


def newManufacturer(id, name):
    global currentManuf, manufacturers, products
    currentManuf = id
    if id in excludes:
        return
    manufacturers[id] = name
    products[id] = {}

def newProduct(id, name):
    global currentManuf, products
    if currentManuf is None:
        raise ValueError("Product without manufacturer, pci.ids is broken!")
    if currentManuf in excludes:
        return
    products[currentManuf][id] = name

def newClass(id, name):
    global classes
    classes[id] = name

def escape(str):
    return str.replace("'", r"\'")

def normalize(str):
    return re.sub(r"\[[^\]]+\]\s*", "", str).strip()

def writeClasses(file):
    with open(file, "w", encoding="iso-8859-1", errors="ignore") as file:
        firstEntry = True
        file.write(header)
        file.write("\n")
        file.write("\t\tSECTION tables,DATA\n\n")
        file.write("\t\tpublic\tpci_classes\n")
        file.write("pci_classes:\t")
        for classId, className in classes.items():
            if firstEntry:
                firstEntry = False
            else:
                file.write("\t\t")
            file.write("dc.w\t${:02x}\n".format(classId))
            file.write("\t\tdc.l\tpci_class_{:02x}_name\n".format(classId))
        file.write("\t\tdc.w\t-1\n")
        file.write("\n")
        file.write("\t\tSECTION strings,DATA\n\n")
        for classId, className in classes.items():
            file.write("pci_class_{:02x}_name:\tdc.b\t'{}',0\n".format(classId, className))
        file.write("\t\t\teven\n")
        file.write("\n")

def writeDatabase(file):
    known = {}
    with open(file, "w", encoding="iso-8859-1", errors="ignore") as file:
        file.write(header)
        file.write("\n")
        file.write("\t\tORG\t0\n\n")
        file.write("\n")
        file.write("\t\tdc.w\t1\t; FILE VERSION\n")
        file.write("\t\tdc.w\t{}\t; NUMBER OF MANUFACTURERS\n".format(len(manufacturers)))

        for manufId, manufName in manufacturers.items():
            file.write("\t\tdc.w\t${:04x}\n".format(manufId))
            file.write("\t\tdc.l\tmanuf_{:04x}\n".format(manufId))

        file.write("\n")
        for manufId, manufName in manufacturers.items():
            prodList = products[manufId]
            file.write("manuf_{:04x}:\tdc.l\tmanuf_{:04x}_name\n".format(manufId, manufId))
            file.write("\t\tdc.w\t{}\n".format(len(prodList)))
            for prodId, prodName in prodList.items():
                file.write("\t\tdc.w\t${:04x}\n".format(prodId))
                file.write("\t\tdc.l\tprod_{:04x}_{:04x}_name\n".format(manufId, prodId))

        file.write("\n")
        for manufId, manufName in manufacturers.items():
            label = "manuf_{:04x}_name".format(manufId)
            if manufName in known:
                file.write("{}:\tequ\t{}\n".format(label, known[manufName]))
            else:
                known[manufName] = label
                file.write("{}:\tdc.b\t'{}',0\n".format(label, escape(manufName)))

        for manufId, prodList in products.items():
            for prodId, prodName in prodList.items():
                label = "prod_{:04x}_{:04x}_name".format(manufId, prodId)
                normalizedProdName = normalize(prodName)
                if normalizedProdName in known:
                    file.write("{}:\tequ\t{}\n".format(label, known[normalizedProdName]))
                else:
                    known[normalizedProdName] = label
                    file.write("{}:\tdc.b\t'{}',0\n".format(label, escape(normalizedProdName)))
        file.write("\n")



print("-- Reading PCI database --")
with open("pciids/pci.ids") as file:
    for line in file:
        vm = re.match(r"^([0-9a-f]{4})\s+(.*)\s*$", line)
        if vm:
            newManufacturer(int(vm.group(1), 16), vm.group(2))
        vp = re.match(r"^\t([0-9a-f]{4})\s+(.*)\s*$", line)
        if vp:
            newProduct(int(vp.group(1), 16), vp.group(2))
        vc = re.match(r"^C\s([0-9a-f]{2})\s+(.*)\s*$", line)
        if vc:
            newClass(int(vc.group(1), 16), vc.group(2))

print("-- Generating Database Source --")
writeDatabase("src/identify/pci/database.s")

print("-- Generating Classes Source --")
writeClasses("src/identify/pci/pciclasses.s")

print()
print("-- STATISTICS --")
prodCount = 0
strLen = 0
for _, manuf in manufacturers.items():
    strLen = max(strLen, len(manuf))
for _, prods in products.items():
    prodCount += len(prods)
    for _, prod in prods.items():
        strLen = max(strLen, len(prod))
print("  Manufacturers:  {}".format(len(manufacturers)))
print("  Products:       {}".format(prodCount))
print("  PCI Classes:    {}".format(len(classes)))
print("  Max Str Length: {}".format(strLen))
print()
