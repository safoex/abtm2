# welcome to simplified version of ABTM

## Install ABTM2

`sudo apt install ros-*-rosbridge-server`

`git clone https://github.com/safoex/abtm2/`

`cd abtm2`

`git submodule update --init`

`cd abtm_dep`

`cd yaml-cpp && mkdir build && cd build && cmake .. -DYAML_BUILD_SHARED_LIBS=ON && make`

`cd ../../..`

`mkdir build && cd build`

`cmake .. `

`make`

## Install ABTM_UI

refer to https://github.com/safoex/abtm_ui/

## Use

start `./test_autoreload` from build/ (! as there are some relative paths :( )

note: if there is an error 1009 in connection, follow this link https://github.com/RobotWebTools/rosbridge_suite/issues/358

start ABTM_UI

`roslaunch rosbridge_server rosbridge_websocket.launch`

## Examples

open `examples/guide.yaml`

start turtlebot3 navigation example with `world` map:

`roslaunch turtlebot3_gazebo turtlebot3_world.launch`

`roslaunch turtlebot3_navigation turtlebot3_navigation.launch `



don't forget to press `start` button in ABTM UI :)
