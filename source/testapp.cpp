// IKSolverBasic.cpp : This file contains the 'main' function. Program execution begins and ends there.
// This is a version for automated testing

//on WINDOWS there is now an implicit include of pch.h here using /FI force-include switch in the Visual Studio project

#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/dll/import.hpp>
#include <boost/dll/library_info.hpp>
#include <boost/function.hpp>

int main (int argc, char* argv[]) // - _tmain macro related to UTF-8/wide chars https://stackoverflow.com/questions/895827/what-is-the-difference-between-tmain-and-main-in-c
{
	std::string executablePath = argv[0]; // argv[0] contains path to our executable
	std::cout << executablePath << std::endl;
	boost::filesystem::path libDirectory = boost::filesystem::path (executablePath).parent_path();
	boost::filesystem::path dlpath = libDirectory / "HelloSayer";

	{
        typedef std::shared_ptr<HelloSayer> hs_create_t();
        boost::function<hs_create_t> hscreator;
		try
		{
			std::cout << "=== Reading Symbols from shared library ===";
			// debugging pybind by reading symbols, see https://www.boost.org/doc/libs/1_73_0/doc/html/boost_dll/tutorial.html#boost_dll.tutorial.querying_libraries_for_symbols

			boost::dll::library_info libinfo(libDirectory / "PybindTest.dll" );

			std::vector<std::string> sections = libinfo.sections();
			
			for (auto& section : sections)
			{
				std::vector<std::string> exports = libinfo.symbols(section); //this is where boost exports live by default, can be changed with BOOST_DLL_ALIAS_SECTIONED
				if (exports.size() > 0)
				{
					for (auto& sym : exports)
					{
						std::cout << "Found symbol " << sym << " in section " << section << std::endl;
					}
				}
				else
				{
					std::cout << "Found no symbols in section " << section << std::endl;
				}
				
			}
			

			// Try loading without the name.https://www.boost.org/doc/libs/1_73_0/doc/html/boost_dll/tutorial.html#boost_dll.tutorial.searching_for_a_symbol_in_multiple_plugins
			bool alt_load = false;
			std::string createsym = "CreateHelloSayer";
			std::string dllpathstr = dlpath.generic_string();
			if (alt_load)
			{
				std::string msg = "Trying boost::dll::shared_library() to load " + createsym + " from " + dllpathstr;
				npth::print_console_header(msg.size()+6, msg, "-");
				boost::dll::shared_library lib(dlpath, boost::dll::load_mode::append_decorations);
				dhscreator = boost::dll::import_alias<dhsapi_create_t>(boost::move(lib), createsym);
			}
			else
			{
				std::string msg = "Trying boost::dll::import_alias() to load " + createsym + " from " + dllpathstr;
				npth::print_console_header(msg.size()+6, msg, "-");
				dhscreator = boost::dll::import_alias<dhsapi_create_t>(
					dlpath,
					createsym,
					boost::dll::load_mode::append_decorations //this is a cross-platform way to add .dll, .so, etc I believe.
					);
			}
			
			
		}
		catch (std::exception&ex)
		{
			std::cerr << ex.what() << std::endl;
			return EXIT_FAILURE;
		}
		catch (...)
		{
			std::cerr << "Unknown exception..." << std::endl;
			return EXIT_FAILURE;
		}

		npth::print_console_header(80, "Shared Library Loaded Successfully, Program Continuing");
        

		const std::string relative_machines_path = "/../../../../machines";
        const std::string default_machine_path = libDirectory.string() + relative_machines_path;

		po::variables_map povm;
		unsigned short port = 0;
		std::string IKMethod;
		double tolerance;
		double otolerance;
		double perr_gain;
		double oerr_gain;
		double nsbend;
		std::string json_filename;
		std::string machine_path;
		std::string machine_file;

		size_t maxiter;
		po::options_description podesc("IKServer options");
		try
		{
			podesc.add_options()
				("help", "Display this message")
				("method", po::value<std::string>(&IKMethod)->required()->default_value("SDLS", "SDLS"), "Method, required (SDLS)")
				("tol", po::value<double>(&tolerance)->default_value(1e-6, "1e-6"), "End effector position tolerance (All Methods)")
				("otol", po::value<double>(&otolerance)->default_value(1e-3, "1e-3"), "End effector orientation tolerance (SDLS)")
				("port", po::value<unsigned short>(&port)->default_value(8025), "Websocket port (All Methods)")
				("maxiter", po::value<size_t>(&maxiter)->default_value(100, "100"), "Maximum number of IK iterations (All Methods)")
				("pgain", po::value<double>(&perr_gain)->default_value(1.0, "1.0"), "Position error gain (SDLS)")
				("ogain", po::value<double>(&oerr_gain)->default_value(0.005, "0.005"), "Orientation error gain (SDLS)")
				("damping", po::value<double>()->default_value(0.005, "0.005"), "Damping constant (DLS)")
				("jumax", po::value<double>()->default_value(0.05, "0.05"), "Max joint move per iteration, rot radians, prism meters (SDLS/VE)")
				("jagain", po::value<double>()->default_value(0.01, "0.01"), "Joint limit avoid gain (SDLS/VE). Set negative for > 6DOF")
				("maxmove", po::value<double>()->default_value(0.025, "0.025"), "Max end-effector move per iteration (SDLS/VE)")
				("sendFK", po::value<bool>()->default_value(false, "false"), "If true, send back actual end effector position instead of target")
				("readquat", po::value<bool>()->default_value(true, "true"), "If true, read (x, y, z, w) quaternion orientation from next four floats after position")
				("machine_path", po::value<std::string>(&machine_path)->required()->default_value(default_machine_path, default_machine_path), "Robot configuration absolute path to machine JSON files.")
				("robot", po::value<std::string>(&machine_file)->required()->default_value("ur5.json", "ur5.json"), "Robot configuration file relative to machine_path.")
				("print_params", po::value<bool>()->required()->default_value(false, "false"), "Print DH and joint limit parameters")
				("use_tcp_transform", po::value<bool>()->required()->default_value(false, "false"), "Apply tool-center-point transform from file.")
				("nullspace_bend", po::value<double>(&nsbend)->required()->default_value(0, "0"), "Amount to set nullspace joints away from zero for >6DOF robots.")
				;

			po::store(po::parse_command_line(argc, argv, podesc), povm);
			if (povm.count("help"))
			{
				std::cout << podesc << std::endl;
				return EXIT_SUCCESS;
			}
			po::notify(povm);
		}
		catch (std::exception& e)
		{
			std::cerr << "***ERROR! " << e.what() << ".\n" << std::endl;
			std::cout << podesc << std::endl;
			return EXIT_FAILURE;
		}
		catch (...)
		{
			std::cerr << "ERROR! Unknown error." << std::endl;
			std::cout << podesc << std::endl;
			return EXIT_FAILURE;
		}
		bool method_is_unsupported = true;
		std::vector<std::string> supported_methods = { "SDLS" };
		for (const auto& method : supported_methods)
		{
			if (method == IKMethod)
			{
				method_is_unsupported = false;
			}
		}

		if (method_is_unsupported)
		{
			std::cerr << "***ERROR! \"--method " << IKMethod << "\" was supplied! Supported methods:\n";
			for (const auto& method : supported_methods)
			{
				std::cerr << "\t" << method << std::endl;
			}
			return EXIT_FAILURE;
		}
		// =========================== GET DH PARAMETERS, JOINT LIMITS, BASE TRANSFORM FROM JSON ============================

		npth::print_console_header(80, "Polarworks DHSolver Remote Server");
		boost::filesystem::path mpath(machine_path);
		boost::filesystem::path total_path = mpath /= boost::filesystem::path(machine_file);
		json_filename = total_path.generic_string();

		boost::shared_ptr<npth::JSONRobotLoader> loader(new npth::JSONRobotLoader());

		loader->loadFile(json_filename);
		std::vector<DHSolver::LinkParamsDH> RobotDH = loader->getDHParams();
		Nj = RobotDH.size();
		std::cout << "** Using " << Nj << " link robot from " << json_filename << std::endl;
		solver->setDHParams(RobotDH);
		std::vector<DHSolver::JointLimits> RobotJointLimits = loader->getJointLimits();
		solver->setJointLimits(RobotJointLimits);

		
		std::vector<double> nullspace_setpoints;
		nullspace_setpoints.clear();
		for (auto jlim : RobotJointLimits)
		{
			double midrange_angle = (jlim.pos_lower + jlim.pos_upper) / 2.0;
			double set_angle = midrange_angle + nsbend;
			nullspace_setpoints.push_back(set_angle);
		}
		
		solver->setNullspaceSetpoints(nullspace_setpoints);
		std::vector<double> ret_nssp;
		solver->getNullspaceSetpoints(ret_nssp);
		std::cout << npth::get_numvector_string(ret_nssp, 3, "** nullspace setpoint angles: ") << std::endl;
		solver->setBaseTransform(loader->getBaseTransform());

		if (povm["use_tcp_transform"].as<bool>())
		{
			DHSolver::TransformParams tcp_xform = loader->getTCPTransform();
			if (isnan(tcp_xform.x) || isnan(tcp_xform.y) || isnan(tcp_xform.z) ||
				isnan(tcp_xform.alpha) || isnan(tcp_xform.beta) || isnan(tcp_xform.gamma))
			{
				std::cerr << "ERROR! TCP Transform was not found in JSON file (found nans) but --use_tcp_transform was true." << std::endl;
				return EXIT_FAILURE;
			}
			std::cout << "** Param --use_tcp_transform is true, applying tool-center-point transform from file." << std::endl;
			std::cout << "tcp transform: " << npth::get_base_transform_string(tcp_xform) << std::endl;
			solver->setTCPTransform(tcp_xform);
			//Eigen::Matrix<double, 4, 4> tcpxm = solver->getTCPTransform();
			//std::cout << "tcp xform matrix:\n" << tcpxm << std::endl;
		}
		else
		{
			std::cout << "** Param --use_tcp_transform is false. No tool-center-point transform applied." << std::endl;
			solver->resetTCPTransform();
			//Eigen::Matrix<double, 4, 4> tcpxm = solver->getTCPTransform();
			//std::cout << "tcp xform matrix:\n" << tcpxm << std::endl;
		}
		

		if (povm["print_params"].as<bool>())
		{
			npth::print_console_header(30, "DH Params");
			std::vector<DHSolver::LinkParamsDH> checkDH;
			solver->getDHParams(checkDH);
			npth::print_dh_params_vec(checkDH);

			npth::print_console_header(30, "Joint Limits");
			std::vector<DHSolver::JointLimits> checkJL;
			solver->getJointLimits(checkJL);
			npth::print_joint_limits_vec(checkJL);

			npth::print_console_header(30, "Base Transform");
			Eigen::Matrix<double, 4, 4> bxfm = solver->getBaseTransform();
			std::cout << bxfm << std::endl;

			npth::print_console_header(30, "TCP Transform");
			Eigen::Matrix<double, 4, 4> txfm = solver->getTCPTransform();
			std::cout << txfm << std::endl;

			npth::print_console_header(30);
		}
		
		// =========================================== SET JOINT LIMIT PARAMETERS MANUALLY =================================

		DHSolver::SetupStatus ikstatus = solver->validateKinematicSetup();


		if (!ikstatus.is_valid) // joint speeds aren't yet specified in the JSON file 
		{
			std::wcerr << L"ERROR! solver kinematic setup not complete. Check code.";
			return EXIT_FAILURE;
		}
		else
		{
			std::cout << "** Kinematic setup validated..." << std::endl;
		}

		// ================ SET AND DISPLAY SOLVER PARAMETERS ===============================

		std::string pfmtr_str = "** Solver params:\n\ttol: %.3g | maxiter: %d | ";
		if ( (IKMethod == "SDLS") ) 
		{
			pfmtr_str += "jumax: %.3g | maxmove: %.3g | jagain: %.3g";
			pfmtr_str += "\n\totol: %.3g | pgain: %.3g | ogain: %.3g";
		}
		else if (IKMethod == "DLS")
		{
			pfmtr_str += "damping: %.3g";
		}
		boost::format pfmtr = boost::format(pfmtr_str);
		std::string connect_message;

		if (IKMethod == "DLS")
		{
			if (!(povm.count("tol") && povm.count("maxiter") && povm.count("damping")))
			{
				std::cerr << "ERROR! Missing tol, maxiter, and/or damping for DLS method. These should have defaults, check code!";
				return EXIT_FAILURE;
			}
			DHSolver::DLSParams DLSparams;
			DLSparams.tolerance = tolerance;
			DLSparams.maxiter = maxiter;
			DLSparams.damping = povm["damping"].as<double>();
			DLSparams.print_iter = false;
			solver->setSolverParams(DLSparams);
			DHSolver::SetupStatus DLSstatus = solver->validateSolverSetup(IKMethod);
			if (!(DLSstatus.is_fully_specified))
			{
				std::cerr << "ERROR! DLS solver parameter validation failed! Check code and supplied parameters!" << std::endl;
				return EXIT_FAILURE;
			}

			pfmtr% DLSparams.tolerance;
			pfmtr% DLSparams.maxiter;
			pfmtr% DLSparams.damping;
			connect_message = "Basic DLS Solver Connected";
		}
		else if ( (IKMethod == "SDLS") )
		{
			if (!(povm.count("tol") && povm.count("otol") && povm.count("maxiter")
				&& povm.count("pgain") && povm.count("ogain") && povm.count("jumax")
				&& povm.count("maxmove") && povm.count("jagain")))
			{
				std::cerr << "ERROR! Missing one or more of tol, otol, maxiter, pgain, ogain, jumax, maxmove, jagain for SDLS method w/ orientation. These should have defaults, check code!";
				return EXIT_FAILURE;
			}
			DHSolver::SDLSParams SDLSparams;
			SDLSparams.tolerance = tolerance;
			SDLSparams.orientation_tolerance = otolerance;
			SDLSparams.maxiter = maxiter;
			SDLSparams.position_err_gain = perr_gain;
			SDLSparams.orientation_err_gain = oerr_gain;
			SDLSparams.max_joint_update = povm["jumax"].as<double>();
			SDLSparams.max_move_distance = povm["maxmove"].as<double>();
			SDLSparams.joint_avoidance_gain = povm["jagain"].as<double>();
			SDLSparams.print_iter = false;
			solver->setSolverParams(SDLSparams);
			DHSolver::SetupStatus SDLSstatus = solver->validateSolverSetup(IKMethod);
			if (!(SDLSstatus.is_fully_specified))
			{
				std::cerr << "ERROR! " << IKMethod << " solver parameter validation failed! Check code and supplied parameters!" << std::endl;
				return EXIT_FAILURE;
			}

			pfmtr% SDLSparams.tolerance;
			pfmtr% SDLSparams.maxiter;
			pfmtr% SDLSparams.max_joint_update;
			pfmtr% SDLSparams.max_move_distance;
			pfmtr% SDLSparams.joint_avoidance_gain;
			pfmtr% SDLSparams.orientation_tolerance;
			pfmtr% SDLSparams.position_err_gain;
			pfmtr% SDLSparams.orientation_err_gain;
			connect_message = "Joint-Avoiding " + IKMethod + " Solver Connected";
		}


		std::wcout << L"** Solver initialized successfully..." << std::endl;
		auto const address = boost::asio::ip::make_address("127.0.0.1");
		std::wcout << L"** Starting websocket server, listening localhost:" + std::to_wstring(port) + L"..." << std::endl;
		if (povm["sendFK"].as<bool>())
		{
			std::wcout << L"** Param --sendFK is true: Responding with forward kinematic end effector position in x, y, z slots..." << std::endl;
		}
		else
		{
			std::wcout << L"** Param --sendFK is false: Responding with target position in x, y, z slots..." << std::endl;
		}
		if (povm["readquat"].as<bool>())
		{
			std::wcout << L"** Param --readquat is true: Parsing target quaternion..." << std::endl;
			if (povm["sendFK"].as<bool>())
			{
				std::wcout << L"\t * Param --sendFK is true: Responding with solved quaternion..." << std::endl;
			}
			else
			{
				std::wcout << L"\t * Param --sendFK is false: Responding with target quaternion..." << std::endl;
			}
		}
		
		std::cout << pfmtr.str() << std::endl;

		try
		{
			// ================= WAIT FOR WEBSOCKET CONNECTION ON SPECIFIED PORT =================================
			// -- following https://www.boost.org/doc/libs/develop/libs/beast/example/websocket/server/sync/websocket_server_sync.cpp but not multithreaded
			boost::asio::io_context ioc{ 1 };
			boost::beast::websocket::stream<boost::beast::tcp_stream> ws(ioc);
			boost::asio::ip::tcp::acceptor acceptor{ ioc, {address, port} }; // -- what is the difference between curly braces and parens!?

			acceptor.accept(boost::beast::get_lowest_layer(ws).socket());
			std::wcout << L"** Connection accepted by acceptor..." << std::endl;
			ws.accept();
			std::wcout << L"** Connection accepted by websocket..." << std::endl;
			npth::print_console_header(80, connect_message);

			// ============ GET READY FOR MESSAGES AND CALCULATIONS ============================
		
			boost::beast::flat_buffer buf; //raw buffer to get input message
			std::string num_as_str; //to hold string stream output
			std::vector<double> vals; //to hold values finally parsed from input mesage

			std::vector<size_t> iteration_counts;
			std::vector<DHSolver::JointConfiguration> joint_cfgs;
			DHSolver::JointConfiguration jcfg;
			jcfg.jsq.resize(Nj);
			
			std::vector<DHSolver::QuatWaypoint> waypoints;
			std::vector<DHSolver::QuatWaypoint> act_posns;
			DHSolver::QuatWaypoint wp;

			// -- formatters to build an output string and response message string

			
			std::string console_fmt_str = "iter: %05d||x:";
			std::string resp_fmt_str = "%011.6f";
			for (int i = 0; i < 3; i++)
			{
				console_fmt_str += "|%+3.2f"; //position
				resp_fmt_str += "|%08.6f"; //position
			}

			// --- optionally add slots for (x, y, z, w) quaternion ---
			if (povm["readquat"].as<bool>())
			{
				console_fmt_str += "||q:";
				for (int i = 0; i < 4; i++)
				{
					console_fmt_str += "|%+5.3f"; //quaternion
					resp_fmt_str += "|%09.8f"; //quaternion
				}
			}
			// --- now add angles to the format strings ---
			console_fmt_str += "||ang:";
			for (int i = 0; i < Nj; i++)
			{
				console_fmt_str += "|%+5.3f"; //angles
				resp_fmt_str += "|%09.8f"; //angles
			}
			resp_fmt_str += "|%05d"; // iterations

			
			boost::format afmtr = boost::format(console_fmt_str); // printed itercount / x / (maybe quat) / angles / 
			boost::format rfmtr = boost::format(resp_fmt_str); //websocket response timestamp/ x/ (maybe quat) /angles / itercount
			
			
			bool gotnan = true;
			vals.push_back(nan("missing"));
			
			std::string oss;
			size_t max_printed_length = 0; // maximum length of a printed line for blanking

			while (!interrupted)
			{
				oss.clear();

				// =============== BLOCK UNTIL MESSAGE RECIEVED ===================
				ws.read(buf); 
				ws.text(ws.got_text()); // -- set to text mode if websocket got text
				std::cout << "\r"; // blank the console line 

				// ================= READ DATA FROM RXed MESSAGE =================
				
				boost::asio::const_buffer cb = boost::beast::buffers_front(buf.data());
				// -- info on boost::beast buffer ops here https://github.com/boostorg/beast/issues/936
				// looks like it's boost::asio::ip::tcp::buffer to send)
				//std::cout << boost::beast::string_view(reinterpret_cast<char const*>(cb.data()), cb.size());
				
				std::string dstr(reinterpret_cast<char const*>(cb.data()), cb.size());
				std::istringstream ss(dstr);

				vals.clear();
				while (std::getline(ss, num_as_str, '|')) // get pipe-sep'd numbers from istringstream
				{
					vals.push_back(std::stod(num_as_str));
				}
				buf.consume(buf.size()); // empty the buffer

				gotnan = isnan(vals[0]);
				int angstart_idx = 0;
				if (povm["readquat"].as<bool>())
				{
					angstart_idx = 8;
				}
				else
				{
					angstart_idx = 4;
				}
				// =================== LOAD VALUES INTO SOLVER DATA STRUCTURES =================
				if (!gotnan)
				{
					double timestamp = vals[0];
					wp.x[0] = vals[1];
					wp.x[1] = vals[2];
					wp.x[2] = vals[3];
					
					if (povm["readquat"].as<bool>())
					{
						for (int i = 0; i < 4; i++)
						{
							wp.quat[i] = vals[i + 4]; //4, 5, 6, 7 if readquat, skip if not
						}
					}
					else
					{
						// manual quaternion target:
						//set to (x, y, z, w) = (1, 0, 0, 0) ---> "down"
						// 45 AROUND X FIRST 0.3826834, 0, 0, 0.9238795
						// 150 AROUND X [ 0.9659258, 0, 0, 0.258819 ]
						wp.quat[0] = 1.0;
						wp.quat[1] = 0.0;
						wp.quat[2] = 0.0;
						wp.quat[3] = 0.0;
					}
					
					
					for (int i = 0; i < Nj; i++)
					{
						jcfg.jsq[i] = vals[i + angstart_idx]; //4, 5, 6, 7, 8... if not readquat, 8, 9, 10, 11... if readquat
					}

					wp.t = boost::chrono::high_resolution_clock::now();
					waypoints.push_back(wp);
					

					// ======================= GET IK SOLUTION WITH APPROPRIATE METHOD =======================================
					// --- streamlined the call
					solver->getJointConfigsFromWaypointsUnstableExperimental(waypoints, joint_cfgs, jcfg, IKMethod);
					solver->getIterationHistory(iteration_counts);
					act_posns.clear();

					if (povm["sendFK"].as<bool>())
					{
						solver->getFKToolPositions(joint_cfgs, act_posns); 
					}
					
					jcfg = joint_cfgs[0];

					// ==================== BUILD FORMATTED OUTPUT TO CONSOLE AND CLIENT ======================================

					// -- feed iteration count to console 
					
					afmtr% iteration_counts[0];
					// -- feed timestamps to angle formatters for response output - console has none

					rfmtr% timestamp;

					// -- feed x positions to console formatter for console output and response formatter for response output
					for (int i = 0; i < 3; i++)
					{
						if (povm["sendFK"].as<bool>())
						{
							rfmtr% act_posns[0].x[i]; //respond with FK end effector position 
							
						}
						else
						{
							rfmtr% wp.x[i]; // respond with target x position
						}
						afmtr% wp.x[i]; //always print target to console
					}

					// --- if quaternions are being streamed, add them. Send actual if --sendFK is true, target if not
					if (povm["readquat"].as<bool>())
					{
						for (int i = 0; i < 4; i++)
						{
							if (povm["sendFK"].as<bool>())
							{
								rfmtr% act_posns[0].quat[i];
							}
							else
							{
								rfmtr% wp.quat[i];
							}
							afmtr% wp.quat[i]; // always print target to console
						}
					}


					// -- feed angles to angle formatters for console and response outputs
					
					for (int i = 0; i < Nj; i++)
					{
						afmtr % jcfg.jsq[i];
						rfmtr% jcfg.jsq[i];
					}
					
					// -- feed iteration count to response formatter 
					rfmtr% iteration_counts[0];

					oss += afmtr.str(); //append to console string
					// =============== RESPOND TO CLIENT =============================
					std::string resp_str = rfmtr.str(); // --fix for "cannot dereference string iterator" abort in Debug executable
					auto response_buf = boost::asio::buffer(resp_str);
					
					ws.write(response_buf);
	
					// -- clear the vectors
					joint_cfgs.clear();
					waypoints.clear();
					iteration_counts.clear();
				}
				
				
				// ========================== PRINT CALCULATIONS TO CONSOLE ==============================================
				if (oss.length() > max_printed_length)
				{
					max_printed_length = oss.length(); //record the maximum printed length
				}

				if (oss.length() < max_printed_length)
				{
					size_t sn = max_printed_length - oss.length();
					for (int i = 0; i < sn; i++)
					{
						oss += " "; // pad with spaces to the maximum printed length
					}
				}
				std::cout << oss;
				oss.clear();
				
			}
			if (interrupted) //this currently never happens, need to figure out how to hook interrupts into blocking boost::beast::websocket
			{
				std::wcout << L"\nCaught <Ctrl-C>, ending program!" << std::endl;
			}
		}
		catch (std::exception& e)
		{
			std::cout << "\n\nException: \"" << e.what() << "\"" << std::endl;
		}

		
		std::wcout << L"Finished, exiting..." << std::endl;
		return EXIT_SUCCESS;

	}
	
	
}

