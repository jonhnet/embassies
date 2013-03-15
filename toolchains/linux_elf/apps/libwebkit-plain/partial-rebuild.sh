(cd build/source-files/webkit-1.2.7 && automake && cd build && make -j2 )
rm -f build/libwebkit*stripped && make
