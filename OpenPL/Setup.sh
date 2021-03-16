#!/bin/bash
#
# Builds and Downloads any and all external content
#

# Colours
NOCOLOR='\033[0m'
RED='\033[0;31m'

# Move into directory we were invoked in
cd "`dirname "$0"`"

echo "${RED}Generating OpenPL${NOCOLOR}"

# If libigl isn't downloaded, clone
if [ ! -d "External/libigl/" ]
then
    echo "${RED}Downloading libigl${NOCOLOR}"

    # Clone into External/
    cd External/
    mkdir libigl
    git clone https://github.com/libigl/libigl.git --branch v2.3.0 --depth 1 libigl
    
    cd libigl
    mkdir external
    cd external

    echo "${RED}Downloading libigl dependancies${NOCOLOR}"
    mkdir eigen
    mkdir glad
    mkdir glfw

    git clone https://github.com/libigl/eigen.git --depth 1 eigen
    git clone https://github.com/libigl/libigl-glad.git --depth 1 glad
    git clone https://github.com/glfw/glfw.git --depth 1 glfw

    # Go back up a folder for the following commands to work
    cd ../../
fi

# If matplot isn't downloaded, clone
if [ ! -d "External/matplotplusplus/" ]
then
    echo "${RED}Downloading matplotplusplus${NOCOLOR}"

    cd External/
    mdir matplotplusplus
    git clone https://github.com/alandefreitas/matplotplusplus.git --branch v1.0.1 --depth 1 matplotplusplus
    
    cd ../
fi

echo "${RED}Done!${NOCOLOR}"
