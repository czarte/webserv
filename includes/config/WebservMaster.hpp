//
// Created by Vojtěch Parkán on 15.12.2025.
//

#ifndef WEBSERV_WEBSERVMASTER_H
#define WEBSERV_WEBSERVMASTER_H
#include <iostream>
#include <string>

class WebservMaster {
	private:
		std::string config;
	public:
		std::string getConfig(std::string configFilePath);
};


#endif
