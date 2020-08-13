# pybind-boostdll-minimal
A minimal example using pybind11 with boost/dll

# Setup

The tests here were run with MS Visual Studio 2019 Community on Windows 10. 

Boost is provided a folder `external` four directories up from the solution. 
This is `..\..\..\..\external\boost_1_72_0` on my computers.

Boost built libs (for filesystem, etc) are in

`..\..\..\..\external\boost_1_72_0\stage\lib`

`pybind11` is included as a submodule. Run `git submodule update --init --recursive` to pull it in.

## Motivation

I'm trying to create a single Windows DLL binary that allows:
 * Direct loading and use from C++ [boost/dll](https://github.com/boostorg/dll)
 * Copying and renaming to a `.pyd` extension to load and use from Python using [pybind11](https://github.com/pybind/pybind11)
 
 This is a follow-on minimal example of the issue described in this [gist](https://gist.github.com/danzimmerman/62aac05da80a1d9097b8590be76830ff).
 
 It will also serve as a template for this dual-use `.dll` if I get it working.
 
 As it stands, including the pybind11 code causes the C++ DLL load to fail with:
 
 ```
 boost::dll::shared_library::load() failed: The specified module could not be found
 ```
 
 ## Project Structure and Source Files
 
 The functionality exposed to both Python and C++ is a single class, `HelloSayer`, with a single method `sayHello()`.

 * [`source/HelloSayerImp.cpp`](source/HelloSayerImp.cpp) 
   * `HelloSayer` implementation and DLL exports using pybind's `PYBIND11_MODULE` and boost/dll's `BOOST_DLL_ALIAS`.

 * [`source/HelloSayerLib.h`](source/HelloSayerLib.h)
   * Header file defining the `HelloSayer` interface for inclusion in C++ applications that load `HelloSayerLib.dll`.

 * [`source/HelloSayerLibPythonTests.py`](source/HelloSayerLibPythonTests.py) 
   * Python test application.

 * [`source/HelloSayerLibCppTest.cpp](source/HelloSayerLibCppTest.cpp)
   * C++ test application source code.

 * [`python`](./python)
   * Destination for `HelloSayerLib.pyd` - add the full path as system environment variable `HELLOSAYER_PYTHON_PATH` or modify `HelloSayerLibPythonTests.py` as appropriate.


 ## C++ Details
  
 In C++ there is an abstract class `HelloSayer` defined in [`source/HelloSayerLib.h`](source/HelloSayerLib.h) that implements a pure virtual `sayHello()`. This is implemented by the class `HelloSayerImp` in [`source/HelloSayerImp.cpp`](source/HelloSayerImp.cpp) which
 also includes a factory function `Create()` that returns a `std::shared_ptr` to an instance of itself:
 
 ```
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
 ```
 
 `BOOST_DLL_ALIAS` is used to export this factory function `HelloSayerImp::Create()` as the symbol `CreateHelloSayer` in the compiled DLL `HelloSayerLib.dll`:
 
 ```
 BOOST_DLL_ALIAS(HelloSayerImp::Create, CreateHelloSayer);
 ```
 
 
 This is built by the `HelloSayerLib` project 
 in the Microsoft Visual Studio Solution `HelloSayer.sln`.
 
 This solution also includes a C++ test application, `HelloSayerLibCppTest`, that uses `boost::dll::import_alias<>()` to import the symbol `CreateHelloSayer` from the DLL, and creates and uses
 the resulting `HelloSayer` stored in a `std::shared_ptr`.
 
 ## Python Details

 
 The `pybind11::init()` method that binds a function to a Python class's `__init__` method can accept a factory that returns a `std::shared_ptr` to an object.
 If `EXPORT_PYTHON` is defined in `HelloSayerImp.cpp`, a module is exported using the `PYBIND11_MODULE` macro.
 
 This defines one class `HelloSayer`, with its `__init__` method bound to `HelloSayerImp::Create()`:
 
 ```
 PYBIND11_MODULE(HelloSayerLib, m)
{
	m.doc() = "pybind11 & boost::dll test Python module"; // optional module docstring

	py::class_<HelloSayerImp, std::shared_ptr<HelloSayerImp>>(m, "HelloSayer")
		.def(py::init(&HelloSayerImp::Create))		
		.def("sayHello", &HelloSayerImp::sayHello);
}
 ```
 
 A post-build action copies `HelloSayerLib.dll` to `HelloSayer\python\HelloSayerLib.pyd`. 
 
 The solution includes a Python test application [`source/HelloSayerLibPythonTests.py`](source/HelloSayerLibPythonTests.py). 
 
 ⚠️To run properly, the user needs to add the system environment variable `HELLOSAYER_PYTHON_PATH` to the full path to `<repodir>\HelloSayer\python`. I tried to do
 this with a pre-build event but I can't get it to work.
 
 You could also use `sys.path.append('path/to/HelloSayerLib.pyd')` but I don't want to hard-code that here.
 
 ## Results without `EXPORT_PYTHON` defined, `BOOST_DLL_ALIAS` alone exporting symbols to DLL
 
 ### Output of `HelloSayerLibCppTest.exe`
 
 ```
 *** boost::dll test HelloSayerLibCppTest.exe running from "x64\Release"

*** Reading sections and symbols from shared library "x64\Release\HelloSayerLib"

        Found no symbols in section .text
        Found no symbols in section .rdata
        Found no symbols in section .data
        Found no symbols in section .pdata
        Found symbol CreateHelloSayer in section boostdll
        Found no symbols in section .rsrc
        Found no symbols in section .reloc

*** Trying boost::dll::import_alias() to load CreateHelloSayer from x64/Release/HelloSayerLib

*** Successfully loaded CreateHelloSayer

*** Trying to use HelloSayer()

*** HelloSayer instance hs->sayHello() said
        "Hello, I'm a HelloSayer running pybind11 & boost::dll tests."
 ```
 
 ### Output of `HelloSayerLibPythonTests.py`
 
 ```
--------------------- HelloSayerLibPythonTests.py trying to load HelloSayerLib ---------------------


-------------- Using directory specified with system env var 'HELLOSAYER_PYTHON_PATH' --------------

Traceback (most recent call last):
  File "C:\Code\dan\minimal_examples\HelloSayer\source\HelloSayerLibPythonTests.py", line 14, in <module>
    import HelloSayerLib as hsl
ImportError: dynamic module does not define module export function (PyInit_HelloSayerLib)
 ```
 
## Results with `#define EXPORT_PYTHON` defined, `BOOST_DLL_ALIAS` and `PYBIND11_MODULE` exporting symbols to DLL
### Output of `HelloSayerLibCppTest.exe`
 
 ```
 *** boost::dll test HelloSayerLibCppTest.exe running from "x64\Release"

*** Reading sections and symbols from shared library "x64\Release\HelloSayerLib"

        Found symbol PyInit_HelloSayerLib in section .text
        Found no symbols in section .rdata
        Found no symbols in section .data
        Found no symbols in section .pdata
        Found symbol CreateHelloSayer in section boostdll
        Found no symbols in section .rsrc
        Found no symbols in section .reloc

*** Trying boost::dll::import_alias() to load CreateHelloSayer from x64/Release/HelloSayerLib

*** ERROR! Library load attempt threw:

        "boost::dll::shared_library::load() failed: The specified module could not be found"
 ```
 
 ### Output of `HelloSayerLibPythonTests.py`

 ```
--------------------- HelloSayerLibPythonTests.py trying to load HelloSayerLib ---------------------


-------------- Using directory specified with system env var 'HELLOSAYER_PYTHON_PATH' --------------


------------------------- HelloSayerLib loaded from .pyd... Starting Tests -------------------------

hs.sayHello() said:
  "Hello, I'm a HelloSayer running pybind11 & boost::dll tests."

HelloSayerLib instance docstring is:
  pybind11 & boost::dll test Python module

------------------------------------------ Tests Complete ------------------------------------------
```
