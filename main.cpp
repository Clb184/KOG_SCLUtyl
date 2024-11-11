#include "SCLDump.h"
#include "SCLCompile.h"
#include <iostream>
#include <random>

#ifdef _DEBUG

void PrintUsage(char* name) {
	printf(
		"Usage: %s \n"
		"d JSONOUT (extension will be added automatically)\n"
		"c SOURCE HEADER.json OUTPUT\n",
		name
	);
}

int main(int argc, char** argv) {
	std::string name;
	std::string header_name;
	if (argc < 2) {
		PrintUsage(argv[0]);
		return -1;
	}
	std::string option = argv[1];
	if (option == "d" && argc == 3) {
		if (!DumpSCL(argv[2])) std::cout << "Failed dumping file\n";
	}
	else if (option == "c" && argc == 5) {
		if (!CompileSCL(argv[2], argv[3], argv[4])) std::cout << "Failed compiling file\n";
	}
	else {
		PrintUsage(argv[0]);
		return -1;
	}
	//

}
#else 
int main() {

}
#endif