#!/usr/bin/env python3

#encoding=utf-8
__author__ = 'addy.ke@rock-chips.com'

import traceback
import os
import sys
import threading
import time
import math
import getopt
import fnmatch

module = "hardware"
chips = ['Pisces', 'Koala', 'koala', 'Swallow', 'swallow', 'pisces', 'rv1126', 'rv1108', 'rk2108', 'rk2106', 'rk2206', 'rk3568', "rk1808"]

class BuildGn:
    def __init__(self, chip, board):
        self.chip = chip
        self.board = board
        self.fp = open("BUILD.gn", 'w+', encoding='utf-8')

    def head(self):
        head = "# Copyright (c) 2020-2021 Lockzhiner Electronics Co., Ltd.\n"
        head +="# limitations under the License.\n"
        head +="# Licensed under the Apache License, Version 2.0 (the \"License\");\n"
        head +="# you may not use this file except in compliance with the License.\n"
        head +="# You may obtain a copy of the License at\n"
        head +="#\n"
        head +="#     http://www.apache.org/licenses/LICENSE-2.0\n"
        head +="#\n"
        head +="# Unless required by applicable law or agreed to in writing, software\n"
        head +="# distributed under the License is distributed on an \"AS IS\" BASIS,\n"
        head +="# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
        head +="# See the License for the specific language governing permissions and\n"
        head +="#\n\n"
        head += "import(\"//drivers/adapter/khdf/liteos_m/hdf.gni\")\n"
        head += "import(\"//device/rockchip/%s/sdk_liteos/board.gni\")\n" % self.chip
        head += "\n"
        head += "static_library(\"%s\") {\n" % module 
        self.fp.write(head)

    def include(self):
        inc =  "  include_dirs = [\n"
        inc += "    \"./include\",\n"
        inc += "    \"./lib/hal/inc\",\n"
        inc += "    \"./lib/CMSIS/Device/%s/Include\",\n" % self.chip.upper()
        inc += "    \"./lib/CMSIS/Core/Include\",\n"
        inc += "    \"./lib/bsp/%s\",\n" % self.chip.upper()
        inc += "    \"./project/%s/src\",\n" % self.chip
        inc += "    \"$adapter_path/include\",\n"
        inc += "    \"$sdk_path/include\",\n"
        inc += "    \"$kernel_path/kernel/arch/include\",\n"
        inc += "    \"$hal_path/include\",\n"
        inc += "    \"./lz_hardware/wifi/include\",\n"
        inc += "    \"//third_party/musl/porting/liteos_m/kernel/include\",\n"
        inc += "  ]\n"
        self.fp.write(inc)

    def tail(self):
        tail = "}\n"
        self.fp.write(tail)

    def source(self, path, init):
        search = ['*.c']
        if init == 0:
            source = "  sources = [\n"
        else:
            source = "  sources += [\n"
        for root, dirnames, filenames in os.walk(path):
            for extension in search:
                for filename in fnmatch.filter(filenames, extension):
                    src = os.path.join(root, filename)
                    found = 1
                    for chip in chips:
                        if src.find(chip) >= 0 and chip != self.chip:
                            found = 0
                        if src.find(chip.upper()) >= 0 and chip != self.chip:
                            found = 0
                    if found == 1:
                        source += "    \"%s\",\n" % src
 
        source += "  ]\n"
        self.fp.write(source)
gn = BuildGn("rk2206", "TB-RK2206H0-A")

gn.head()
gn.source('./lib/hal', 0)
gn.source('./lib/bsp', 1)
gn.source('./lib/CMSIS/Device', 1)
gn.source('./driver', 1)
gn.source('./lz_hardware', 1)
gn.include()
gn.tail()
