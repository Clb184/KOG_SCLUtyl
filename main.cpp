#include "SCLDump.h"
#include <iostream>

#ifdef _DEBUG
int main(int argc, char** argv) {
	std::string name;
	switch (argc) {
	case 1:
		std::cout << "Enter filename: "; std::cin >> name;
		break;
	case 2:
		name = argv[1];
		break;
	}
	if (!DumpSCL(name.c_str())) std::cout << "Failed loading file\n";

}
#else 
int main() {

}
#endif