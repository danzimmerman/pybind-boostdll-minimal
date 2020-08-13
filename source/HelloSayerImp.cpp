#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <boost/dll/alias.hpp>

#include "HelloSayer.h" // defines the interface of HelloSayer, consisting of a pure virtual sayHello()

#define EXPORT_PYTHON

namespace py = pybind11;

class HelloSayerImp : HelloSayer
{
	public:
		std::string sayHello()
		{
			return "Hello, I'm a HelloSayer running pybind11 & boost::dll tests.";
		}

		static std::shared_ptr<HelloSayerImp> Create()
		{
			return std::shared_ptr<HelloSayerImp>(new HelloSayerImp);
		}
};

#ifdef EXPORT_PYTHON

PYBIND11_MODULE(HelloSayerLib, m)
{
	m.doc() = "pybind11 & boost::dll test Python module"; // optional module docstring

	py::class_<HelloSayerImp, std::shared_ptr<HelloSayerImp>>(m, "HelloSayer")
		.def(py::init(&HelloSayerImp::Create))		
		.def("sayHello", &HelloSayerImp::sayHello);
}

#endif

BOOST_DLL_ALIAS(HelloSayerImp::Create, CreateHelloSayer);
