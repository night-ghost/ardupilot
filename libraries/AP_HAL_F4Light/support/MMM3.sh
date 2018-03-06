#git submodule init && git submodule update
export TOOLCHAIN

ROOT=`cd ../../..; pwd`

export PATH=/usr/local/bin:$PATH

echo $ROOT

 ( # AirBotF4 board
 cd $ROOT/ArduCopter
 make f4light VERBOSE=1 BOARD=f4light_AirbotV2  && (

 cp $ROOT/ArduCopter/f4light_AirbotV2.bin $ROOT/Release/Copter
 cp $ROOT/ArduCopter/f4light_AirbotV2.hex $ROOT/Release/Copter
 cp $ROOT/ArduCopter/f4light_AirbotV2.dfu $ROOT/Release/Copter


 )
) && (
 cd $ROOT/ArduPlane
 make f4light-clean
 make f4light VERBOSE=1 BOARD=f4light_AirbotV2 && (

 cp $ROOT/ArduPlane/f4light_AirbotV2.bin $ROOT/Release/Plane
 cp $ROOT/ArduPlane/f4light_AirbotV2.hex $ROOT/Release/Plane
 cp $ROOT/ArduPlane/f4light_AirbotV2.dfu $ROOT/Release/Plane
 )

) && (
 cd $ROOT

 zip -r latest.zip Release
 git add . -A
)











