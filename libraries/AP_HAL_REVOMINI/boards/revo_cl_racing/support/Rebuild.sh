#git submodule init && git submodule update
export TOOLCHAIN

ROOT=`cd ../../../../..; pwd`

export PATH=/usr/local/bin:$PATH

echo $ROOT


( # AirBotF4 board
 cd $ROOT/ArduCopter
 make revomini-clean
 make revomini BOARD=revo_cl_racing
) && (
 cd $ROOT/ArduPlane
 make revomini-clean
 make revomini BOARD=revo_cl_racing
)

