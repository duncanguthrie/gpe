#
# treecc_0.3.6.bb - OpenEmbedded bb file for the cross-compiled version 
#			of the DotGNU Tree Compiler Compiler ver. 0.3.6
#
# Copyright (C) 2005  Kirill Kononenko <Kirill.Kononenko@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

DESCRIPTION = "Tree Compiler-Compiler from dotGNU"
SECTION = "devel"
LICENSE = "GPL-2"
HOMEPAGE = "http://www.southern-storm.com.au/treecc.html"
MAINTAINER = "Kirill Kononenko <Kirill.Kononenko@gmail.com"
PROVIDES = "treecc"
PRIORITY = "optional"

SRC_URI = "http://www.southern-storm.com.au/download/treecc-0.3.6.tar.gz"

CFLAGS = "-O0 -g"

inherit autotools
do_install () {
autotools_do_install
}