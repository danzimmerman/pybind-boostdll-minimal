# pybind-boostdll-minimal
A minimal example using pybind11 with boost/dll

# Resolution

The issue here was that the C++ executable is linked against the Python DLL and needs it to run.

In retrospect, this makes sense.

Copying `python37.dll` from the Anaconda directory to the C++ test app directory solves the issue.

# Notes for Posterity

## Setup

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

 * [`source/HelloSayerLibCppTest.cpp`](source/HelloSayerLibCppTest.cpp)
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
  File "<projectpath>\HelloSayer\source\HelloSayerLibPythonTests.py", line 14, in <module>
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

## Notes

Asked a Stack Overflow question [here](https://stackoverflow.com/questions/63401931/pybind-with-boost-dll-dual-use-dll).


## Modifications

At line 100 of `\debug_libs\boost_1_72_0\boost\dll\detail\windows\shared_library_impl.hpp`

Breakpoints on the two `handle_` lines. Seems like both go to the `.c_str()` branch below.

```
if (sl.has_extension()) {
      std::cout << "LoadLibraryEXW c_str() branch sl is " << sl << std::endl;
      std::cout << "sl.c_str() is " << sl.c_str()  << std::endl;
    for (size_t i = 0; i < sl.size(); i++)
    {
        std::cout << sl.c_str()[i] << "|";
    }
    std::cout << "\n";
    std::cout << "mode is " << static_cast<native_mode_t>(mode) << std::endl;
    handle_ = boost::winapi::LoadLibraryExW(sl.c_str(), 0, static_cast<native_mode_t>(mode));
} else {
    std::cout << "LoadLibraryEXW native() branch sl is " << sl << std::endl;
    handle_ = boost::winapi::LoadLibraryExW((sl.native() + L".").c_str(), 0, static_cast<native_mode_t>(mode));
}

// LoadLibraryExW method is capable of self loading from program_location() path. No special actions
// must be taken to allow self loading.
if (!handle_) {
    ec = boost::dll::detail::last_error_code();
}
```

### With `pybind`

```
*** getting libinfo on "C:\Code\<user>\minimal_examples\HelloSayer\x64\Debug\HelloSayerLib.dll"
        Found no symbols in section .textbss
        Found symbol PyInit_HelloSayerLib in section .text
        Found no symbols in section .rdata
        Found no symbols in section .data
        Found no symbols in section .pdata
        Found no symbols in section .idata
        Found symbol CreateHelloSayer in section boostdll
        Found no symbols in section .msvcjmc
        Found no symbols in section .tls
        Found no symbols in section .00cfg
        Found no symbols in section .rsrc
        Found no symbols in section .reloc

*** Trying boost::dll::import_alias() to load CreateHelloSayer from "C:\Code\<user>\minimal_examples\HelloSayer\x64\Debug\HelloSayerLib.dll"
LoadLibraryEXW c_str() branch sl is "C:\Code\<user>\minimal_examples\HelloSayer\x64\Debug\HelloSayerLib.dll"
sl.c_str() is 000002C3E5EA46B0
67|58|92|67|111|100|101|92|100|97|110|92|109|105|110|105|109|97|108|95|101|120|97|109|112|108|101|115|92|72|101|108|108|111|83|97|121|101|114|92|120|54|52|92|68|101|98|117|103|92|72|101|108|108|111|83|97|121|101|114|76|105|98|46|100|108|108|
mode is 0

*** ERROR! Library load attempt threw:

        "boost::dll::shared_library::load() failed: The specified module could not be found"
```

### Without `pybind`

```
*** getting libinfo on "C:\Code\<user>\minimal_examples\HelloSayer\x64\Debug\HelloSayerLib.dll"
        Found no symbols in section .textbss
        Found no symbols in section .text
        Found no symbols in section .rdata
        Found no symbols in section .data
        Found no symbols in section .pdata
        Found no symbols in section .idata
        Found symbol CreateHelloSayer in section boostdll
        Found no symbols in section .msvcjmc
        Found no symbols in section .00cfg
        Found no symbols in section .rsrc
        Found no symbols in section .reloc

*** Trying boost::dll::import_alias() to load CreateHelloSayer from "C:\Code\<user>\minimal_examples\HelloSayer\x64\Debug\HelloSayerLib.dll"
LoadLibraryEXW c_str() branch sl is "C:\Code\<user>\minimal_examples\HelloSayer\x64\Debug\HelloSayerLib.dll"
sl.c_str() is 000001F4709238A0
67|58|92|67|111|100|101|92|100|97|110|92|109|105|110|105|109|97|108|95|101|120|97|109|112|108|101|115|92|72|101|108|108|111|83|97|121|101|114|92|120|54|52|92|68|101|98|117|103|92|72|101|108|108|111|83|97|121|101|114|76|105|98|46|100|108|108|
mode is 0

*** Successfully loaded CreateHelloSayer

*** Trying to use HelloSayer()

*** HelloSayer instance hs->sayHello() said
        "Hello, I'm a HelloSayer running pybind11 & boost::dll tests."
```




