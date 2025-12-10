# pbm && ./build/oces_viewer data/velox-head.gltf 0.0002 -0.00056 0.0 -0.00005
pushd build && make && popd  &&  ./build/oces_viewer -f data/velox-head.gltf -r 0.0002 -c -0.00056,0.0,-0.00005
