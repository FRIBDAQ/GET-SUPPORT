/* =====================================================================================================================
 * dataRouter.cpp
 * ---------------------------------------------------------------------------------------------------------------------
 * dataRouter functions
 * Created on: Jun 20, 2012 at Irfu/Sedi/Lilas, CEA Saclay, F-91191, France
 * ---------------------------------------------------------------------------------------------------------------------
 * © Commissariat a l'Energie Atomique et aux Energies Alternatives (CEA)
 * ---------------------------------------------------------------------------------------------------------------------
 * Contributors: Shebli Anvar (shebli.anvar@cea.fr)
 * ---------------------------------------------------------------------------------------------------------------------
 * This software is part of the hardware access classes for embedded systems of the Mordicus Real-Time Software
 * Development Framework.
 * ---------------------------------------------------------------------------------------------------------------------
 * FREE SOFTWARE LICENCING
 * This software is governed by the CeCILL license under French law and abiding  * by the rules of distribution of free
 * software. You can use, modify and/or redistribute the software under the terms of the CeCILL license as circulated by
 * CEA, CNRS and INRIA at the following URL: "http://www.cecill.info". As a counterpart to the access to the source code
 * and rights to copy, modify and redistribute granted by the license, users are provided only with a limited warranty
 * and the software's author, the holder of the economic rights, and the successive licensors have only limited
 * liability. In this respect, the user's attention is drawn to the risks associated with loading, using, modifying
 * and/or developing or reproducing the software by the user in light of its specific status of free software, that may
 * mean that it is complicated to manipulate, and that also therefore means that it is reserved for developers and
 * experienced professionals having in-depth computer knowledge. Users are therefore encouraged to load and test the
 * software's suitability as regards their requirements in conditions enabling the security of their systems and/or data
 * to be ensured and, more generally, to use and operate it in the same conditions as regards security. The fact that
 * you are presently reading this means that you have had knowledge of the CeCILL license and that you accept its terms.
 * ---------------------------------------------------------------------------------------------------------------------
 * COMMERCIAL SOFTWARE LICENCING
 * You can obtain this software from CEA under other licencing terms for commercial purposes. For this you will need to
 * negotiate a specific contract with a legal representative of CEA.
 * =====================================================================================================================
 */

/**
 * Contribution/modification begun by NSCL/FRIB September 19, 2018
 * This notice is required by the CeCILL license above.
 * The intent of this modification is to allow the main to provide data from
 * GET into NSCL ring buffers.  Specifically
 *
 *   - Command line processing will be modified to allow ring output processing
 *      to be selected.
 *   - Command line processing will be modified to allow the user to supply a
 *     ringbuffer destination.
 *   - Command line processing will be modified to allow the user to specify a source id
 *     to be used to tag ring item body headers.
 *   - Command line processing will be modified to support the choice of
 *     trigger number or timestamp as the timestamp in event body headers.
 *
 *  @note to support the added flexibility required, it's likely we'll modify
 *        command processing to use gengetopt so that we can get it to do
 *        some of sanity checking for us (e.g. ICE parameters are only needed
 *        if ICE transport is specified.  Similarly the NSCLDAQ parameters
 *        are only needed if ring buffer outputter is selected).
*/

#include "mdaq/DefaultPortNums.h"
#include "mdaq/utl/CmdLineArgs.h"
#include "mdaq/utl/Server.h"
#include "mdaq/utl/ConsoleLoggingBackend.h"
#include "DataRouter.h"
#include "mfm/FrameDictionary.h"
#include "utl/Logging.h"

#include <boost/algorithm/string.hpp>
namespace ba = boost::algorithm;
#include <memory>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <CRingBuffer.h>
#include "datarouterargs.h"
#include "NSCLDAQDataProcessor.h"

using ::utl::net::SocketAddress;
using ::mdaq::utl::CmdLineArgs;
using ::mdaq::utl::Server;
using ::get::daq::DataRouter;

int main(int argc, char* argv[])
{
	gengetopt_args_info parsed;
	cmdline_parser(argc, argv, &parsed);
	
	
	// Setup logging backend
	::utl::BackendLogger::setBackend(::utl::LoggingBackendPtr(new ::mdaq::utl::ConsoleLoggingBackend));

	try
	{
		std::vector<std::string> args;             // For the ICE args.

		// Setting control endpoint.  This comes from
		// --controlservice_arg
		
		SocketAddress ctrlEndpoint;
		if (parsed.controlservice_given) ctrlEndpoint.fromString(parsed.controlservice_arg);
		if (ctrlEndpoint.port() == 0) ctrlEndpoint.port() = ::mdaq::Default::dataRouterCtrlPortNum;
		if (not ctrlEndpoint.isValid()) ctrlEndpoint.ipAddress().in_addr() = INADDR_ANY;

		// Setting data flow endpoint (ip address will be same as control if not defined)
		// This comes from dataservice_arg if supplied.
		
		SocketAddress flowEndpoint;
		if (parsed.dataservice_given) flowEndpoint.fromString(parsed.dataservice_arg);
		if (flowEndpoint.port() == 0) flowEndpoint.port() = ::mdaq::Default::dataRouterFlowPortNum;
		if (not flowEndpoint.isValid()) flowEndpoint.ipAddress().in_addr() = ctrlEndpoint.ipAddress().in_addr();

		// Setting data flow type.  This comes from protocol_arg.
		std::string flowType = parsed.protocol_arg;             // gengetopt handles default.
		ba::to_upper(flowType);

		// Setting data processor type - from outputtype
		
		const std::string processorType = parsed.outputtype_arg;
		

		/** TODO: Pull in CoboFormats from the installation dir instead of
		 *        wd.
		 */
		
		// Load frame formats
		if ("FrameStorage" == processorType)
		{
		  //mfm::FrameDictionary::instance().addFormats("CoboFormats.xcfg");
		  mfm::FrameDictionary::instance().addFormats(COBO_FORMAT_FILE);
		} else if (processorType == "RingBuffer") {
		  //	mfm::FrameDictionary::instance().addFormats("CoboFormats.xcfg");  // We'll also need to assemble/decode frames.
		  mfm::FrameDictionary::instance().addFormats(COBO_FORMAT_FILE);  // We'll also need to assemble/decode frames.
			
			// process ring buffer types.
		}

		// Disable IPv6 support
		// With Ice 3.5, support for IPv6 is enabled by default.
		// On sedipcg212 (Ubuntu 14.04), it causes "Address family is not supported by protocol" socket exceptions when using wildcard address.
		args.push_back("--Ice.IPv6=0");
		for (int i  = 0; i < parsed.icearg_given; i++) {
			args.push_back(parsed.icearg_arg[i]);
		}
		// Creating data router servant
		Server& server = Server::create(ctrlEndpoint, args);
		server.ic(); // Hack to ensure ICE communicator is initialized with desired arguments
		DataRouter* r = new DataRouter(flowEndpoint, flowType, processorType);
		IceUtil::Handle< DataRouter > dataRouter(r);
		
		// If the data router was for a "RingBuffer", we need to set the
		// ringbuffer name, source id and how the timestamp comes about.
		
		if (processorType == "RingBuffer") {
			NSCLDAQDataProcessor& proc(
				dynamic_cast<NSCLDAQDataProcessor&>(r->getDataReceiver()->dataProcessorCore())
			);
			proc.setSourceId(parsed.id_arg);
			
			// Figure out timestamp source:
			
			std::string tsSource = parsed.timestamp_arg;
			if(tsSource == "timestamp") {
				proc.useTimestamp();
			} else if (tsSource == "trigger_number") {
				proc.useTriggerId();
 			} else {
				throw "--timestamp value must be either 'timestamp' or 'trigger_number'";
			}
            // Figure out the ring buffer name.. if not provided, use the
			// default name
			
			std::string ringName = CRingBuffer::defaultRing();
			if (parsed.ring_given) ringName = parsed.ring_arg;
			proc.setRingBufferName(ringName);
			
		}
		
		server.addServant("DataRouter", dataRouter).start();
		dataRouter->runStart();
		server.waitForStop();
		return EXIT_SUCCESS;
	}
	catch (const std::exception & e) { LOG_FATAL() << e.what(); }
	catch (const std::string &  msg) { LOG_FATAL() << msg; }
	catch (const char*          msg) { LOG_FATAL() <<  msg; }
	catch (...)                      { LOG_FATAL() << "Unknown exception."; }
	return EXIT_FAILURE;
}
