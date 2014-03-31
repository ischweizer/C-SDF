make sdf-client TARGET=sky

make sdf-client TARGET=sky sdf-client.upload

msp430-nm -S sdf-client.sky > sdf-client.map

msp430-size sdf-client.sky
