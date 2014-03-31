make sdf-server TARGET=sky

make sdf-server TARGET=sky sdf-server.upload

msp430-nm -S sdf-server.sky > sdf-server.map

msp430-size sdf-server.sky
