#pragma once


namespace HelloSayerVersionInfo
{
	const char* const projectName = "HelloSayer";
	const char* const companyName = "DanZimmerman";
	const char* const versionString = "0.1.0";
}

class HelloSayer
{
public:
	virtual std::string sayHello() = 0;
	virtual std::string getVersionInfo() = 0; // UNSTABLE API 
};

struct TooManyParams
{
	double parameter_a = 1;
	double parameter_b = 2;
	double parameter_c = 3;
	std::string parameter_s = "some text";
};

//int main()
//{
//	TooManyParams params;
//	double x = params.parameter_a;
//}