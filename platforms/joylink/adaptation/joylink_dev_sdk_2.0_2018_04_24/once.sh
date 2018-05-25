#!/bin/bash
MAKE_RULE=./Makefile.rule

set_project_path()
{
    sed -i "s%\(^PROJECT_ROOT_PATH:=\).*%\1$PWD%" $MAKE_RULE
}

create_project_path()
{
    if [ ! -d ./target/lib ];then
        mkdir -p ./target/lib
    fi

    if [ ! -d ./target/bin ];then
        mkdir -p ./target/bin
    fi

    if [ ! -d ./test/bin ];then
        mkdir -p ./test/bin
    fi
}

make_version()
{
    TVER=`awk '/VERSION:/{n++}n==1{print;exit}' RELEASENOTES |awk '{split($0,a,"[:]");print a[2]}'`
    V1=`echo $TVER | awk '{split($0,a,"[.]");print a[1]}'`
    V2=`echo $TVER | awk '{split($0,a,"[.]");print a[2]}'`
    V3=`echo $TVER | awk '{split($0,a,"[.]");print a[3]}'`

    let V3=$V3+1
    NEW_VER=$V1.$V2.$V3

    sed -i "s%\(^VERSION:\).*%\1$NEW_VER%" ./RELEASENOTES 
    sed -i "s%\(^#define _VERSION_  \).*%\1\"$NEW_VER\"%" ./joylink/joylink.h

    echo "NEW VERSION:$NEW_VER"
}

make_git_head()
{
    GIT_HEAD=`git log -1 |grep commit | awk '{split($0,a,"[ ]");print a[2]}'`
    sed -i "s%\(^HEAD:\).*%\1$GIT_HEAD%" ./RELEASENOTES 
    sed -i "s%\(^#define _GIT_HEAD_  \).*%\1\"$GIT_HEAD\"%" ./joylink/joylink.h
    echo "GIT_HEAD:$GIT_HEAD"
}

make_date()
{
    DATE_V=`date "+%D"`
    sed -i "s%\(^DATE:\).*%\1$DATE_V%" ./RELEASENOTES 
}

release_sdk(){  
    DATE_V=`date "+%Y_%m_%d"`
    SDK_NAME=`pwd |xargs basename`
    R_SDK_NAME=${SDK_NAME}_${DATE_V}

    make clean

    cd ../
    cp -rf $SDK_NAME $R_SDK_NAME

    cd $R_SDK_NAME 
    rm -rf .git
    rm -rf tags
    rm -rf cscope.*
    rm -rf joylink_info.txt*
    rm -rf devs_info.txt*
    cd -

    tar czvf ${R_SDK_NAME}.tar.gz $R_SDK_NAME
    rm -rf $R_SDK_NAME
}  

help_info(){  
    echo "NAME"  
    echo "    $0"  
    echo "SYNOPSIS"  
    echo "    $0 process options"  
    echo "DESCRIPTION"  
    echo "   -v make a new version"  
    echo "   -r release sdk"  
    echo "   -h help info"  
}  

if [ $# -eq 0 ] 
then
    set_project_path
    create_project_path
else
    if [ $1 == "-v" ]; then
        make_version 
        make_git_head
        make_date
    fi
    if [ $1 == "-h" ]; then
        help_info
    fi

    if [ $1 == "-r" ]; then
        release_sdk
    fi
fi
