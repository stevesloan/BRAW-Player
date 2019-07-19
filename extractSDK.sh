#!/bin/bash

tar xzf CameraCodecSDK_Linux_1.3.tar.gz 
mv Blackmagic\ RAW\ SDK/Linux/Libraries ./BlackmagicRawAPI
mv Blackmagic\ RAW\ SDK/Linux/Include .
rm -rf ./Blackmagic\ RAW\ SDK
