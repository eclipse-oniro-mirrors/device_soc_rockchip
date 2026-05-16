#
# Copyright (c) 2022 FuZhou Lockzhiner Electronic Co., Ltd. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

TARGET = liteos
ELF_FILE = $(ELFFILE)
BUILD_DIR = $(OUTDIR)

PREFIX = arm-none-eabi-
CC =$(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size

HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

####################################
all:
	@echo "Create $(BUILD_DIR)/$(TARGET).elf"
	cp -f ${ELF_FILE} $(BUILD_DIR)/$(TARGET).elf
	$(SZ) $(BUILD_DIR)/$(TARGET).elf
	@echo "Create $(BUILD_DIR)/$(TARGET).hex"
	rm -rf $(BUILD_DIR)/$(TARGET).hex
	$(HEX) $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex
	@echo "Create $(BUILD_DIR)/$(TARGET).bin"
	rm -rf $(BUILD_DIR)/$(TARGET).bin
	$(BIN) $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin
	md5sum $(BUILD_DIR)/$(TARGET)*