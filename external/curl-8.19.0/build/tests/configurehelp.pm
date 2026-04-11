#***************************************************************************
#                                  _   _ ____  _
#  Project                     ___| | | |  _ \| |
#                             / __| | | | |_) | |
#                            | (__| |_| |  _ <| |___
#                             \___|\___/|_| \_\_____|
#
# Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
#
# This software is licensed as described in the file COPYING, which
# you should have received as part of this distribution. The terms
# are also available at https://curl.se/docs/copyright.html.
#
# You may opt to use, copy, modify, merge, publish, distribute and/or sell
# copies of the Software, and permit persons to whom the Software is
# furnished to do so, under the terms of the COPYING file.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
# SPDX-License-Identifier: curl
#
###########################################################################

package configurehelp;

use strict;
use warnings;
use Exporter;

use vars qw(
    @ISA
    @EXPORT_OK
    $Cpreprocessor
    );

@ISA = qw(Exporter);

@EXPORT_OK = qw(
    $Cpreprocessor
    );

$Cpreprocessor = '"C:/msys64/mingw64/bin/cc.exe" -E  -IE:/personal progects/wow log analyser/external/curl-8.19.0/include -IE:/personal progects/wow log analyser/external/curl-8.19.0/build/lib -IE:/personal progects/wow log analyser/external/curl-8.19.0/lib';

1;
