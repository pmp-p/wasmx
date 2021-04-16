#!/bin/bash

URL_EpoxyDuino="https://github.com/pmp-p/EpoxyDuino.git"


CC=${CC:-clang}
CXX=${CC}++

CDEFS="-DEPOXY_DUINO -Dd_m3HasWASI=1 -Dd_m3HasTracer=1 -D__ARDUINO__=1 -I./EpoxyDuino/ -I./src/ -Wno-deprecated"

BUILD=build/$(arch)
mkdir -p $BUILD

OK=true



if [ -d EpoxyDuino ]
then
    if [ -f localdev ]
    then
        echo "using local version of $URL_EpoxyDuino"
    else
        cd EpoxyDuino
        git pull --ff-only
        cd ..
    fi
else
    git clone $URL_EpoxyDuino
fi


for obj in src/*.c EpoxyDuino/*.cpp
do
    EXT="c"
    if echo $obj|grep -q cpp$
    then
        EXT="cpp"
    fi

    bname=$(basename $obj .$EXT)

    if [ -f "$BUILD/${bname}.o" ]
    then
        echo $bname up2date
    else
        if $CXX ${CDEFS} -c -o $BUILD/${bname}.o $obj
        then
            echo $obj
        else
            echo "error building ${bname}.c"
            OK=false
            break
        fi
    fi
done


if $OK
then
    rm $BUILD/wapy.out

    ar rcs ${BUILD}/libwasmx.a $BUILD/*.o
    ${CXX} ${CDEFS} -x c++ -o $BUILD/wapy.out examples_wasi/wapy/wapy.ino -L${BUILD} -lwasmx

    echo running exe

    ./$BUILD/wapy.out

else
    echo failed
fi



