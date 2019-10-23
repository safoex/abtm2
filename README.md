# welcome to simplified version of ABTM

## Install ABTM2

`sudo apt install ros-*-rosbridge-server`

`mkdir build && cd build`

`cmake ../ -DCMAKE_MY_LIBS_PATH="../abtm_dep"`

`make`

## Install ABTM_UI

refer to https://github.com/safoex/abtm_ui/

## Use

start `./test_autoreload`

start ABTM_UI

`roslaunch rosbridge_server rosbridge_websocket.launch`

## Examples

open `examples/move_base.yaml`

start turtlebot3 navigation example with `world` map.
