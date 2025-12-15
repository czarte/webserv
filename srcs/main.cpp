#include <iostream>
#include <unistd.h>

int main() {
	std::cout << "WebServ started " << std::endl;
	while (true) {
		sleep(1000);
	}
	return 0;
}
