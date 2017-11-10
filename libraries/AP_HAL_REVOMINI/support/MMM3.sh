#git submodule init && git submodule update
export TOOLCHAIN

ROOT=`cd ../../..; pwd`

export PATH=/usr/local/bin:$PATH

echo $ROOT


 ( # AirBotF4 board
 cd $ROOT/ArduCopter
 make revomini-clean
 make revomini VERBOSE=1 BOARD=revomini_Airbot  && (

 cp $ROOT/ArduCopter/revomini_Airbot.bin $ROOT/Release/Copter
 cp $ROOT/ArduCopter/revomini_Airbot.hex $ROOT/Release/Copter
 cp $ROOT/ArduCopter/revomini_Airbot.dfu $ROOT/Release/Copter

 make revomini-clean

 )
) && (
 cd $ROOT/ArduPlane
 make revomini-clean
 make revomini VERBOSE=1 BOARD=revomini_Airbot && (

 cp $ROOT/ArduPlane/revomini_Airbot.bin $ROOT/Release/Plane
 cp $ROOT/ArduPlane/revomini_Airbot.hex $ROOT/Release/Plane
 cp $ROOT/ArduPlane/revomini_Airbot.dfu $ROOT/Release/Plane

 make revomini-clean

 )
)  && ( # AirBotV2 board
 cd $ROOT/ArduCopter
 make revomini-clean
 make revomini VERBOSE=1 BOARD=revomini_AirbotV2  && (

 cp $ROOT/ArduCopter/revomini_AirbotV2.bin $ROOT/Release/Copter
 cp $ROOT/ArduCopter/revomini_AirbotV2.hex $ROOT/Release/Copter
 cp $ROOT/ArduCopter/revomini_AirbotV2.dfu $ROOT/Release/Copter

 make revomini-clean

 )
) && (
 cd $ROOT/ArduPlane
 make revomini-clean
 make revomini VERBOSE=1 BOARD=revomini_AirbotV2 && (

 cp $ROOT/ArduPlane/revomini_AirbotV2.bin $ROOT/Release/Plane
 cp $ROOT/ArduPlane/revomini_AirbotV2.hex $ROOT/Release/Plane
 cp $ROOT/ArduPlane/revomini_AirbotV2.dfu $ROOT/Release/Plane

 make revomini-clean

 )
) && (
 cd $ROOT

 zip -r latest.zip Release
 git add . -A
)

