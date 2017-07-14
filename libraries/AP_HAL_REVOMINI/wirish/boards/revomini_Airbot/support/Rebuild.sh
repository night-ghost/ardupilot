#git submodule init && git submodule update
export TOOLCHAIN

ROOT=`cd ../../../../../..; pwd`

export PATH=/usr/local/bin:$PATH

echo $ROOT


(  # AirBotF4 board
 cd $ROOT/ArduPlane
 make revomini-clean
 make revomini VERBOSE=1 BOARD=revomini_Airbot &&
 cp $ROOT/ArduPlane/revomini_Airbot.bin $ROOT/Release/Plane &&
 cp $ROOT/ArduPlane/revomini_Airbot.hex $ROOT/Release/Plane &&
 cp $ROOT/ArduPlane/revomini_Airbot.dfu $ROOT/Release/Plane 
) && (
 cd $ROOT/ArduCopter
 make revomini-clean
 make revomini VERBOSE=1 BOARD=revomini_Airbot &&
 
 cp $ROOT/ArduCopter/revomini_Airbot.bin $ROOT/Release/Copter &&
 cp $ROOT/ArduCopter/revomini_Airbot.hex $ROOT/Release/Copter &&
 cp $ROOT/ArduCopter/revomini_Airbot.dfu $ROOT/Release/Copter 
) 

