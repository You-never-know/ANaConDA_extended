#
# Copyright (C) 2023 Jan Fiedor <fiedorjan@centrum.cz>
#
# This file is part of ANaConDA.
#
# ANaConDA is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# ANaConDA is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with ANaConDA. If not, see <http://www.gnu.org/licenses/>.
#

#
# Description:
#   Builds an ANaConDA image based on specified required arguments:
#     - BASE_IMAGE: URI of the image to extend when creating this image
# Author:
#   Jan Fiedor
#   Daniel Marek
# Version:
#   1.1
# Created:
#   10.08.2023
# Last Update:
#   18.04.2025
#

# Having base image as argument allows choosing which OS to base the image on
ARG BASE_IMAGE
FROM $BASE_IMAGE
WORKDIR /

RUN apt-get update && \
    # Packages needed for initial setup
    apt-get install -y sudo git && \
    # Packages required to build ANaConDA
    apt-get install -y g++ make m4 wget nano

# Account for building and running ANaConDA (name: 'anaconda', pwd: 'anaconda')
RUN useradd -m -s /bin/bash -g users -G sudo anaconda && \
    echo anaconda:anaconda | chpasswd

# Building ANaConDA under root is not recommended, use the 'anaconda' account
USER anaconda

WORKDIR /home/anaconda/dev/projects

# Build ANaConDA
RUN git config --global http.sslverify false && \
    # Download the specified version of ANaConDA with all submodules it uses
    git clone --recursive https://github.com/You-never-know/ANaConDA_extended.git && \
    cd ./ANaConDA_extended && \
    # Build all ANaConDA dependencies from sources
    tools/build.sh --setup-environment && \
    # Build the ANaConDA framework with all its builtin analysers
    tools/build.sh all && \
    # Cleanup the build environment: remove source archives and build folders
    rm -rf ./build/*.tar.gz ./build/*.tar.bz2 && \
    rm -rf ./build/boost* ./build/cmake* ./build/gcc* && \
    rm -rf ./build/analysers ./build/framework ./build/libraries ./build/shared ./build/tests ./build/wrappers

# Set the initial directory of the image to the root directory of ANaConDA
WORKDIR /home/anaconda/dev/projects/ANaConDA_extended

# This keeps containers based on this image running until they are stopped
CMD ["sleep", "infinity"]

# End of Docker file
