#!/bin/bash
CC=${CC:-clang}
CXX=${CC}++

CDEFS="-DEPOXY_DUINO -D__ARDUINO__=1 -I./EpoxyDuino/ -Wno-deprecated"
BUILD=build/$(arch)
mkdir -p $BUILD

OK=true
LIB=false


for obj in src/*.c EpoxyDuino/*.cpp
do
    EXT="c"
    if echo $obj|grep -q cpp$
    then
        EXT="cpp"
        if $LIB
        then
            if echo $obj|grep -q main.cpp$
            then
                continue
            fi
        fi
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

if $LIB
then
    ar rcs ${BUILD}/libwasmx.a $BUILD/*.o
    cat EpoxyDuino/main.cpp examples_wasi/wapy/wapy.ino > $BUILD/ino.cpp
    ${CXX} ${CDEFS} -x c++ -o $BUILD/wapy $BUILD/ino.cpp -L${BUILD} -lwasmx
else
    ar rcs ${BUILD}/libwasmx.a $BUILD/*.o
    #${CXX} ${CDEFS} -x c++ -o $BUILD/wapy examples_wasi/wapy/wapy.ino $BUILD/*.o
    ${CXX} ${CDEFS} -x c++ -o $BUILD/wapy.out examples_wasi/wapy/wapy.ino -L${BUILD} -lwasmx
fi
    echo running exe

    ./$BUILD/wapy.out

else
    echo failed
fi



