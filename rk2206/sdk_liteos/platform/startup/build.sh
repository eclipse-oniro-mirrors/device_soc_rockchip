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

# error out on errors
set -e

LIBS_DIR="$1"
SDK_DIR=$(pwd)
# link.h dir
INCLUDES_DIR=$(pwd)/../../liteos_m
LDSCRIPT=board.ld
CFLAGS="SDK_DIR=${SDK_DIR} INCLUDES_DIR=${INCLUDES_DIR} LDSCRIPT=${LDSCRIPT} LIBS_DIR=${LIBS_DIR}"

function main()
{
    # make ld script
    make ${CFLAGS} -f build/link.mk
}

main "$@"
