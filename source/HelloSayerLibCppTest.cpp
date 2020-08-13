// HelloSayerLibCppTest.cpp : Testing HelloSayerLib DLL Load

#include <iostream>
#include <sstream>

#include <boost/dll/import.hpp>
#include <boost/dll/library_info.hpp>
#include <boost/function.hpp>

#include <HelloSayerLib.h>

int main(int argc, char* argv[])
{
	std::string exePath = argv[0];

	boost::filesystem::path libDir = boost::filesystem::path(exePath).parent_path();
	std::cout << "\n*** boost::dll test HelloSayerLibCppTest.exe running from " << libDir << std::endl;
	std::string libFileStr = "HelloSayerLib";
	boost::filesystem::path libFile = libDir / libFileStr;

	{
		typedef std::shared_ptr<HelloSayer> hs_create_function_t(); // a function signature for the creator function
		boost::function<hs_create_function_t> hscreator; // a function to hold the imported code from the DLL symbol
		
        // === GET AND PRINT LIBRARY INFO ===
		try 
		{
			std::cout << "\n*** Reading sections and symbols from shared library " << libFile << std::endl;
			boost::filesystem::path libSuffix = boost::dll::shared_library::suffix();

			std::cout << "\n*** Shared library suffix is " << libSuffix.generic_string() << std::endl;
			std::cout << "\n*** Shared library suffix as boost::filesystem::path is " << libSuffix << std::endl;
			boost::filesystem::path libFileExt = libDir / (libFileStr + libSuffix.generic_string());
			std::cout << "\n*** getting libinfo on " << libFileExt << std::endl;
			boost::dll::library_info libinfo(libFileExt);
			std::vector<std::string> sections = libinfo.sections();

			for (auto& section : sections)
			{
				std::vector<std::string> exported_sections = libinfo.symbols(section);
				if (exported_sections.size() > 0)
				{
					for (auto& sym : exported_sections)
					{
						std::cout << "\tFound symbol " << sym << " in section " << section << std::endl;
					}
				}
				else
				{
					std::cout << "\tFound no symbols in section " << section << std::endl;
				}

			}
		}
		catch (std::exception& ex)
		{
			std::cerr << "\n*** ERROR! Getting library info threw:\n" << std::endl;
			std::cerr << "\t\"" << ex.what() << "\"" << std::endl;
			return EXIT_FAILURE;
		}

		// === NOW LOAD THE DLL AND TRY TO USE THE FUNCTION ===
		try
		{
			std::string createFunctionSym = "CreateHelloSayer";
			std::string libFileStr = libFile.generic_string();

			std::cout << "\n*** Trying boost::dll::import_alias() to load " << createFunctionSym <<  " from " << libFileStr << std::endl;
			hscreator = boost::dll::import_alias<hs_create_function_t>(
				libFile,
				createFunctionSym,
				boost::dll::load_mode::append_decorations
				);
			std::cout << "\n*** Successfully loaded " << createFunctionSym << std::endl;
		}
		catch (std::exception& ex)
		{
			std::cerr << "\n*** ERROR! Library load attempt threw:\n" << std::endl;
			std::cerr << "\t\"" << ex.what() << "\"" << std::endl;
			return EXIT_FAILURE;
		}

		// === FINALLY USE THE LOADED DLL ===
		try
		{
			std::cout << "\n*** Trying to use HelloSayer()" << std::endl;
			std::shared_ptr<HelloSayer> hs = hscreator();
			std::string msg = hs->sayHello();
			std::cout << "\n*** HelloSayer instance hs->sayHello() said \n\t\"" << msg << "\"" << std::endl;
		}
		catch (std::exception& ex)
		{
			std::cerr << "\n*** ERROR! Using library threw:\n" << std::endl;
			std::cerr << "\t\"" << ex.what() << "\"" << std::endl;
		}

	}
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
