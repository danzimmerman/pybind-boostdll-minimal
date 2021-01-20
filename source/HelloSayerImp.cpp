#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <boost/dll/alias.hpp> // using system-level boost 1.7.2, in ..\..\..\..\external\boost_1_72_0. 

#include <HelloSayerLib.h> // defines the interface of HelloSayer, consisting of a pure virtual sayHello()

//#define EXPORT_PYTHON //If EXPORT_PYTHON is defined, the C++ DLL will depend on a python DLL. Copy python37.dll or whatever is in anaconda3 to the output directory!

namespace py = pybind11;

class HelloSayerImp : HelloSayer
{
	public:
		std::string sayHello()
		{
			return "Hello, I'm a HelloSayer running pybind11 & boost::dll tests.";
		}

		std::string getVersionInfo()
		{
			std::string ver(HelloSayerVersionInfo::versionString);
			std::string company(HelloSayerVersionInfo::companyName);
			std::string project(HelloSayerVersionInfo::projectName);
			return company + " " + project + ", Version " + ver;
		}

		static std::shared_ptr<HelloSayerImp> Create()
		{
			return std::shared_ptr<HelloSayerImp>(new HelloSayerImp);
		}

};

BOOST_DLL_ALIAS(HelloSayerImp::Create, CreateHelloSayer);
//BOOST_DLL_ALIAS(HelloSayerImp::getVersionInfo, GetVersionInfo);

#ifdef EXPORT_PYTHON

PYBIND11_MODULE(HelloSayerLib, m)
{
	m.doc() = "pybind11 & boost::dll test Python module"; // optional module docstring

	py::class_<HelloSayerImp, std::shared_ptr<HelloSayerImp>>(m, "HelloSayer")
		.def(py::init(&HelloSayerImp::Create))
		.def("sayHello", &HelloSayerImp::sayHello);
}

#endif


