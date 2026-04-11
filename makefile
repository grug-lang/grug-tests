check:
	cmake --build ./build

run: check
	./build/smoketest.exe

