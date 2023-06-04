mkdir build
cd build

cmake ..

cmake --build .

./clox

cd ..

rm -r build