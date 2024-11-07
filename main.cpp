#include "SCLDump.h"
#include "SCLCompile.h"
#include <iostream>

#ifdef _DEBUG

int main(int argc, char** argv) {
	std::string name;
	std::string header_name;

	switch (argc) {
	case 1:
		//std::cout << "Enter filename: "; std::cin >> name;
		//std::cout << "Enter header name: "; std::cin >> header_name;
		break;
	case 2:
		name = argv[1];
		break;
	}
	if (!CompileSCL("stage01.txt", "stage01.dat.json", "output.dat")) std::cout << "Failed compiling file\n";
	//if (!DumpSCL(name.c_str())) std::cout << "Failed loading file\n";

}
#else 
int main() {

}
#endif