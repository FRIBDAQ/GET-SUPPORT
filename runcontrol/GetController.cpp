/* =====================================================================================================================
 * TestController.cpp
 * ---------------------------------------------------------------------------------------------------------------------
 * class TestController
 * Created on: 9 sept. 2010 at Irfu/Sedi/Ldef, CEA Saclay, F-91191, France
 * ---------------------------------------------------------------------------------------------------------------------
 * © Commissariat a l'Energie Atomique et aux Energies Alternatives (CEA)
 * ---------------------------------------------------------------------------------------------------------------------
 * Contributors: Frederic Druillole (frederic.druillole@cea.fr)
 *               Shebli Anvar (shebli.anvar@cea.fr)
 * ---------------------------------------------------------------------------------------------------------------------
 * This software is part of the GET (General Electronics for TPC) data acquisition framework @ CEA-Irfu.
 * Find out more on: http://www-actar-get.cea.fr
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
 * Original header left above
 *
 * Author:
 *      Genie Jhang
 *      Facility for Rare Isotope Beams
 *      Michigan State University
 *      East Lansing, MI 48824-1321
 */

/**
 * @file GetController.cpp
 * @brief Essential part of TestController class to prepare/configure CoBo.
 *        Enabled to configure multiple CoBos.
 * @author Genie Jhang <changj@frib.msu.edu>
 */

#include "EccClient.h"
#include "GetController.h"

#include <get/rc/CoBoConfigParser.h>
#include <get/DefaultPortNums.h>
#include <get/daq/FrameStorage.h>
#include <get/utl/Time.h>

#include <get/asad/utl.h>
#include <mdaq/utl/net/SocketAddress.h>
#include <CCfg/Attribute.h>
#include <mfm/Serializer.h>
#include <mfm/Frame.h>
#include <mfm/Field.h>
#include <mfm/BitField.h>
#include <mfm/Exception.h>
#include <mfm/FrameFormat.h>
#include <mdaq/utl/Server.h>
#include <utl/BitFieldHelper.hpp>
#include <utl/Logging.h>

#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
namespace ba = boost::algorithm;
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>
namespace io = boost::iostreams;

using namespace get;
using namespace ::mdaq::utl::net;

#include <iostream>
#include <sstream>
#include <ctime>
using std::size_t;

#define DBG_IF(cond)  if(!(cond)) ; else LOG_DEBUG()
#undef MSG_IF
#define MSG_IF(cond)  DBG_IF(cond)

bool GetController::verbose = true;

inline uint64_t logtwo(uint64_t seed)
{
	uint64_t result = 0;
	int maxBits = 64;
	for (int i=0; i < maxBits; ++i)
	{
		switch (seed >> result)
		{
		case 1:
			return result;

		case 0:
			result -= maxBits >> i;
			break;

		default:
			result += maxBits >> i;
			break;
		}
	}
	return result;
}

GetController::GetController(std::string& conditionFile) :
	coboInstanceName_("CoBo"),
  eccServerAddress("127.0.0.1:46002")
{
	std::cout << "Creating test controller" << std::endl;

	// Load run conditions
	CCfg::CConfig cfg(&runCfgDoc_.loadFromFile(conditionFile));
	if (cfg("Node", "CoBo").exists())
	{
		runCfg_.setBaseAttribute(&cfg("Node", "CoBo").find());
		coboInstanceName_ = "Instance";
	}
	else
	{
		runCfg_.setBaseAttribute(cfg.getBaseAttribute());
		coboInstanceName_ = "CoBo";
	}
	try
	{
		// Setting verbose mode from parameter file
		setVerbose(runCfg_("ECC")("Verbose").getBoolValue());
	}
	catch (const CCfg::CConfig::NotFound&)
	{ /* keep default value*/
	}

//	MSG_IF(verbose) << "ECC -> " << testSummary().eccEndpoint();
//	MSG_IF(verbose) << "TARGET -> " << testSummary().targetEndpoint();

	// Enable data display or not

	std::cout << "Verbose mode is " << (isVerbose()?"":"NOT ") << "enabled" << std::endl;
}
//__________________________________________________________________________________________________
/**
 * Setting eccServer address
 */
void GetController::setEccServerAddress(std::string address) {
  eccServerAddress = address;
}
//__________________________________________________________________________________________________
/**
 * Method for removing all nodes is exposed to public
 */
void GetController::removeAllNodes() {
  eccClient().removeAllNodes();
}
//__________________________________________________________________________________________________
/**
 * Establishes connection to ECC server by allocating EccClient if not yet done.
 * @return Reference to allocated EccClient object.
 */
EccClient& GetController::eccClient()
{
	if (!eccClient_.get())
	{
		eccClient_.reset(new EccClient(eccServerAddress));
		eccClient_->setVerbose(isVerbose());
		std::cerr << "Created client for connection to ECC server " << "FRIB ECC CLIENT" << std::endl;
		MSG_IF(verbose) << "Verbose mode is " << (isVerbose() ? "" : "NOT ") << "enabled";
	}
	return *eccClient_;
}
//__________________________________________________________________________________________________
/**
 * Checks in configuration which AsAd boards and AGET chips to enable.
 * @param coboCfg Configuration of the CoBo board.
 */
void GetController::setMasks(CCfg::CConfig coboCfg)
{
	asadMask_ = 0;
	agetMask_ = 0;

	if (coboCfg("isActive").getBoolValue())
	{
		// Loop over 4 AsAd boards
		for (size_t asadIdx=0; asadIdx < 4; ++asadIdx)
		{
			try
			{
				bool isBoardActive = coboCfg("AsAd", asadIdx)("Control")("isActive").getBoolValue();
				if (isBoardActive)
				{
					::utl::BitFieldHelper<uint16_t>::setField(asadMask_, asadIdx, 1, 1);
					// Loop over 4 AGET chips
					for (size_t agetIdx=0; agetIdx < 4; ++agetIdx)
					{
						bool isChipActive = coboCfg("AsAd", asadIdx)("Aget", agetIdx)("Control")("isActive").getBoolValue();
						::utl::BitFieldHelper<uint16_t>::setField(agetMask_, 4*asadIdx+agetIdx, 1, (isChipActive?1u:0));
					}
				}
			}
			catch (const CCfg::CConfig::NotFound &)
			{}
		}
	}

	std::cout << "Mask of active AsAd boards: " << std::showbase << std::hex << asadMask_ << std::dec << std::endl;
	std::cout << "Mask of active AGET chips: " << std::showbase << std::hex << agetMask_ << std::dec << std::endl;
}
//_________________________________________________________________________________________________
/**
 * Initialization of ECC server connection.
 * Initialization of the system for test algorithms
 * Creates data server and connects to target
 * Reduced-GET system parameters initialization.
 */
void GetController::loadHw(std::size_t coboIdx, std::string& targetEndpoint,
                           std::string& datarouterEndpoint, std::string& hwDescription)
{
  // See which AsAd/AGET are active
  setMasks(coboConfig(coboIdx));

  // Load hardware description
  eccClient().loadHwDescription(targetEndpoint, hwDescription, coboIdx);
  eccClient().setActiveAsAdMask(asadMask_);
  eccClient().setActiveAGetMask(agetMask_);

  // Set 'initDone' bit to 'not configured' (0) state
  eccClient().setCoBoInitDone(false);
  // Tell CoBo board it will not be driven by Mutant
  eccClient().setCoBoMutantMode(false);

  // Check hardware type
  coboVersion_ = eccClient().checkHardwareVersion();

  // Check firmware version
  eccClient().checkFirmwareRelease();

  // Initialize firmware pipeline
  eccClient().pipeInit();

  // Set function of CoBo LEMO connectors
  setCoBoLemoModes(coboIdx);

  // Initialize trigger mode
  eccClient().triggerInit();

  // Initialize circular buffer
  eccClient().circularBufferInit();
  // Initialize AsAd registers
  asadInit(10, coboIdx);
  // Initialize CoBo and AsAd clock frequencies
  setCoBoWritingClockFrequency(coboIdx);
  asadSamplingClockInit(coboIdx);
  setCoBoReadingClockFrequency(coboIdx);
  asadReadingClockInit(coboIdx);
  // Set manual SCR data delay
  setReadDataDelay(coboIdx);
  // Set AsAd AGET input protections
  asadAgetInputManagerInit(coboIdx);
  // Initialize AGET registers
  agetInit(coboIdx);
  // Initialize AsAd ADC parameters
  eccClient().asadAdcInit(EccClient::DDR);
  // Initialize AsAd DAC registers
  eccClient().asadDacInit();
  // Initialize AsAd auto-protection
  asadAlertsInit(coboIdx);
  // Subscribe to alarm notifications
  subscribeToAlarms(coboIdx);
  asadMonitoringRead();
  // Initialize value of AGET global threshold (why now?)
  initializeGlobalThresholds(coboIdx, 2);
  // Compute automatic SCR data delay
  calibrateAutoReadDataDelay(coboIdx);
  // Select automatic or manual SCR data delay
  selectReadDataDelay(coboIdx);

  eccClient().sleep_ms(500);

  eccClient().daqConnect(datarouterEndpoint, "TCP");
}

//_________________________________________________________________________________________________
/**
 * Configures the board(s).
 */
void GetController::configure(std::size_t coboIdx, std::string& targetEndpoint)
{
  // See which AsAd/AGET are active
  setMasks(coboConfig(coboIdx));

	// Reconnect to node in case another client of the ECC changed current node
  eccClient().connectNode(targetEndpoint);
  eccClient().setActiveAsAdMask(asadMask_);
  eccClient().setActiveAGetMask(agetMask_);

  // Loop over each of the 4 AsAd boards
  for (std::size_t asadIdx = 0; asadIdx < 4u; ++asadIdx)
  {
    // Skip inactive AsAd board
    if (not isAsadActive(coboIdx, asadIdx))
      continue;
    std::cerr << "Configuring AsAd board no. " << asadIdx << std::endl;

    // Select AsAd board for slow control
    selectAsAdForSlowControl(asadIdx);

    // Loop over AGET chips
    switchToSlowControl(coboIdx);
    for (std::size_t agetIdx = 0; agetIdx < 4u; ++agetIdx)
    {
      // Skip inactive AGET
      if (not isAgetActive(coboIdx, asadIdx, agetIdx))
        continue;
      std::cerr << "Configuring registers of AGET no. " << (4*asadIdx+agetIdx) << std::endl;

      // Select AGET chip for slow control
      selectAgetForSlowControl(asadIdx, agetIdx);

      // Enable/disable readout of AGET FPN channels
      enableFPNRead(coboIdx, asadIdx, agetIdx);
      // Set AGET chip registers
      const bool scCheck = agetIsScCheck(coboIdx, asadIdx, agetIdx);
      bool agetRegConfigOk = true;
      agetRegConfigOk = agetRegConfigOk and setAget_reg1(coboIdx, asadIdx, agetIdx, scCheck);
      agetRegConfigOk = agetRegConfigOk and setAget_reg2(coboIdx, asadIdx, agetIdx, scCheck);
      agetRegConfigOk = agetRegConfigOk and setAget_reg34(coboIdx, asadIdx, agetIdx, scCheck);
      agetRegConfigOk = agetRegConfigOk and setAget_reg67(coboIdx, asadIdx, agetIdx, scCheck);
      agetRegConfigOk = agetRegConfigOk and setAget_reg89(coboIdx, asadIdx, agetIdx, scCheck);
      agetRegConfigOk = agetRegConfigOk and setAget_reg1011(coboIdx, asadIdx, agetIdx, scCheck);
      agetRegConfigOk = agetRegConfigOk and setAget_reg12(coboIdx, asadIdx, agetIdx, scCheck);
      if (not agetRegConfigOk)
      {
        std::ostringstream oss;
        oss << "Error configuring registers of AGET no. " << asadIdx*4+agetIdx;
        throw std::runtime_error(oss.str());
      }
      // Set CoBo registers for AGET chip 'readAlways' and 'readIfHit' masks
      setReadIfHitMask(coboIdx, asadIdx, agetIdx);
      setReadAlwaysMask(coboIdx, asadIdx, agetIdx);
      // Set CoBo thresholds for zero suppression mode
      setZeroSuppressionThresholds(coboIdx, asadIdx, agetIdx);
    }
    switchToSlowControl(coboIdx);

    // Select AsAd board for slow control
    selectAsAdForSlowControl(asadIdx);

    // Set AsAd generator mode
    asadPulserConfigure(coboIdx, asadIdx);

    // Read AsAd voltages, currents, temperatures
    readAsAdVdd(asadIdx);
    readAsAdVdd1(asadIdx);
    readAsAdIdd(asadIdx);
    const int16_t Ti = readAsAdIntTemp(asadIdx);
    std::cout << "Read AsAd " << asadIdx << " internal temperature: " << Ti << ' ' << char(0xB0) << 'C' << std::endl;
    const int16_t Te = readAsAdExtTemp(asadIdx);
    std::cout << "Read AsAd " << asadIdx << " external temperature: " << Te << ' ' << char(0xB0) << 'C' << std::endl;

    // Set AsAd inspection manager
    setAsAdInspectionManager(coboIdx, asadIdx);
  }
  // Reset AsAd and AGET masks
  eccClient().setAsAdSlowControlMask(asadMask_);
  eccClient().setAgetSlowControlMask(agetMask_);

  // Enable/disable hit register modification
  setWriteHitRegisterEnabled(coboIdx);
  // Enable/disable CoBo test acquisition mode
  setAcqMode(coboIdx);
  // Set parameters of multiplicity trigger mode
  setMultiplicityMode(coboIdx);
  setMultThres2p(coboIdx);
  // Set multiplicity trigger moving average/deviation subtraction
  setCoBoMultSubtractMode(coboIdx);
  // Select which chips to ignore in the multiplicity trigger
  setCoBoSuppressMultiplicity(coboIdx);
  // Enable/disable 2p mode in CoBo
  enableMem2pMode(coboIdx);
  // Set data source ID
  setCoBoDataSourceId(coboIdx);
  // Set CoBo ID
  setCoBoId(coboIdx);
  // Set trigger period, delays, ...
  setTriggerPeriod(coboIdx);
  setTriggerDelay_10ns(coboIdx);
  setTriggerDeadTime2p_10ns(coboIdx);
  setTriggerDelay2p_10ns(coboIdx);
  setTriggerTime2p_10ns(coboIdx);
  setTriggerTimeOut2p_10ns(coboIdx);
  // Set circular buffer parameters
  setCircularBuffers(coboIdx);
  // Select trigger mode
  setTriggerModes(coboIdx);
  setScwMultDelay(coboIdx);
  setScwScrDelay(coboIdx);
  // Set CoBo readout mode
  setReadOutMode(coboIdx);
  // Set number of time buckets to readout
  setReadOutDepth(coboIdx);
  // Set readout offset in data frame header
  setReadOutIndex(coboIdx);
  // Enable zero suppression or not
  setZeroSuppressionEnabled(coboIdx);

	// Select which circular buffers to read
	eccClient().daqSetCircularBuffersEnabled();

	// Initialize 2p mode
	eccClient().aget2pInitNow();

	// Configure AsAd periodic pulser
	configureAsAdPeriodicPulser();

	// Make all CoBo boards send 'ready' status to Mutant modules.
	// Must be done before configuring Mutant modules as it affects Mutant B's MRDY status bit.
	eccClient().setCoBoInitDone(true);
}
//__________________________________________________________________________________________________
/**
 * Sets function of the 4 LEMO connectors on the CoBo front panel.
 */
void GetController::setCoBoLemoModes(const size_t& coboIdx)
{
	if (coboVersion_ < 2u) return;

	std::cerr << "Configuring LEMO connectors" << std::endl;
	try
	{
		for (size_t lemoIndex = 0; lemoIndex < 4u; lemoIndex++)
		{
			uint8_t lemoMode = rc::CoBoConfigParser::getLemoConnectorMode(coboConfig(coboIdx), lemoIndex);
			eccClient().setCoBoLemoMode(lemoIndex, lemoMode);
		}
	}
	catch (const mdaq::utl::CmdException & e)
	{
		std::cerr << e.what() << std::endl;
	}
}
//_________________________________________________________________________________________________
/**
 * Initialize AsAd registers
 */
void GetController::asadInit(const int& delay_ms, const size_t& coboIdx)
{
	// Check whether to abort if AsAd power supply connection is not detected
	uint8_t abortIfNoPowerSupply = 0xF;
	for (size_t asadIdx = 0; asadIdx < 4u; ++asadIdx)
	{
		BitFieldHelper< uint8_t >::setBit(abortIfNoPowerSupply, asadIdx, asadConfig(coboIdx, asadIdx)("Control")("checkPowerSupply").getBoolValue(true));
	}
	// Power on
	eccClient().asadInit(delay_ms, abortIfNoPowerSupply);

	eccClient().setAsAdSlowControlMask(asadMask_);

	// Calibrates the AsAd slow control
	eccClient().asadCal();

	// Configure and start AsAd monitoring (without auto-protection)
	asadMonitoringInit(coboIdx);

	// Check AsAd VDD voltages
	asadMonitoringCheckVDD(coboIdx);

	// Set AGET input protections
	eccClient().asadSetAgetInputProtections();
}
//__________________________________________________________________________________________________
/**
 * Fetches sampling frequency in configuration file.
 */
float GetController::getWritingClockFrequency(const size_t& coboIdx, bool& pllConfigEnabled)
{
	if (isCoboActive(coboIdx))
	{
		try
		{
			pllConfigEnabled = not coboConfig(coboIdx)("Module")("skipPLLConfig").getBoolValue();
		}
		catch (const CCfg::CConfig::NotFound & e)
		{
			if (coboVersion_ >= 2u)
			{
				std::cerr << e.what() << std::endl;
			}
			pllConfigEnabled = true;
		}
		return coboConfig(coboIdx)("Module")("writingClockFrequency").getRealValue();
	}
	return 100.0;
}
//__________________________________________________________________________________________________
/**
 * Sets CoBo writing clock frequency.
 */
void GetController::setCoBoWritingClockFrequency(const size_t& coboIdx)
{
	// Check whether to skip configuration of CoBo PLL device
	bool pllConfigEnabled = true;
	// Check sampling frequency
	float freq_MHz = getWritingClockFrequency(coboIdx, pllConfigEnabled);
	// Check advanced PLL parameters
	get::cobo::PllRegisterMap regs;
	if (coboVersion_ >= 2)
	{
		for (size_t regIndex = 10; regIndex <= 13; ++regIndex)
		{
			regs[regIndex] = rc::CoBoConfigParser::getPllRegValue(coboConfig(coboIdx), regIndex);
		}
	}

	eccClient().setWritingClockFrequency(freq_MHz, pllConfigEnabled, regs);
}
//__________________________________________________________________________________________________
/**
 * Adjusts the value of the LCM1 register of all active AsAd boards using the configuration file.
 * @param freqCKW_MHz Frequency of the CKW sampling clock provided by CoBo.
 */
void GetController::asadSamplingClockInit(const size_t& coboIdx)
{
	// Get CKR frequency
	const float freqCKW_MHz = coboConfig(coboIdx)("Module")("writingClockFrequency").getRealValue();
	for (size_t asadIdx = 0; asadIdx < 4; ++asadIdx)
	{
		if (isAsadActive(coboIdx, asadIdx))
		{
			// Compute LCM1 value
			float samplingClockDivider = 1; // WCKn = CKW
			try
			{
				samplingClockDivider = asadConfig(coboIdx, asadIdx)("Clocking")("samplingClockDivider").getRealValue();
			}
			catch (const CCfg::CConfig::NotFound & e)
			{
				std::cerr << e.what() << std::endl;
			}
			uint64_t lcm1Value = asad::utl::buildLcm1RegisterValue(freqCKW_MHz, samplingClockDivider);

			// Patch LCM1 value
			CCfg::CConfig asadCfg(asadConfig(coboIdx, asadIdx));
			const bool lcm1DebugMode = asadCfg("Clocking")("Debug")("LCM1")("debugModeEnabled").getBoolValue(false);
			if (lcm1DebugMode)
			{
				::get::asad::utl::Lcm1Config lcm1Config;
				asad::utl::parseAsAdLcm1DebugConfig(asadCfg, lcm1Config);
				asad::utl::patchLcm1RegisterValue(lcm1Value, lcm1Config);
			}

			// Select AsAd
			selectAsAdForSlowControl(asadIdx);
			// Configure register LCM1 of AsAd Local Clock Manager device
			eccClient().setAsAdLcm1Register(lcm1Value);
		}
	}
	eccClient().setAsAdSlowControlMask(asadMask_);
}
//_________________________________________________________________________________________________
/**
 * Gets clock frequency for chip readout from configuration.
 */
float GetController::getCoBoReadingClockFrequency(int iCobo)
{
	if (isCoboActive(iCobo))
	{
		return coboConfig(iCobo)("Module")("readingClockFrequency").getRealValue();
	}
	return 25.0;
}
//__________________________________________________________________________________________________
/**
 * Sets CoBo clock frequency for AGET chip readout.
 */
void GetController::setCoBoReadingClockFrequency(int iCobo)
{
	eccClient().setCoBoReadingClockFrequency(
			getCoBoReadingClockFrequency(iCobo));
}
//__________________________________________________________________________________________________
/**
 * Adjusts the value of the LCM2 register of all active AsAd boards using the configuration file.
 */
void GetController::asadReadingClockInit(const size_t& coboIdx)
{
	for (size_t asadIdx = 0; asadIdx < 4u; ++asadIdx)
	{
		if (isAsadActive(coboIdx, asadIdx))
		{
			// Get value
			uint64_t lcm2Value = getAsAdLcm2Value(coboIdx, asadIdx);
			// Select AsAd
			selectAsAdForSlowControl(asadIdx);
			// Set LCM2 register
			eccClient().setAsAdLcm2Register(lcm2Value);
		}
	}
}
//__________________________________________________________________________________________________
/**
 * Checks value of delay for AEGT chip readout in configuration.
 */
int32_t GetController::getReadDataDelay(int iCobo)
{
	if (isCoboActive(iCobo))
	{
		return coboConfig(iCobo)("Module")("readDataDelay").getIntValue();
	}
	return 16;
}
//__________________________________________________________________________________________________
/**
 *  Writes manual read data delay into CoBo board.
 */
void GetController::setReadDataDelay(int iCobo)
{
	eccClient().setReadDataDelay(getReadDataDelay(iCobo));
}
//_________________________________________________________________________________________________
/**
 * Configures the activation of the AsAd ADC channels and the AsAd AGET Inputs Manager.
 * See Texas Instruments ADS6422 Data Sheet.
 * See AsAd Slow Control Protocol & Registers Mapping
 * See AsAd Switched Capacitor Array Management, "AGET Inputs Manager".
 *
 *  ADC register A (ADC0):
 * 	D10	<RST> 		S/W RESET
 * 	D9	0
 * 	D8	0
 * 	D7	0
 * 	D6	0
 * 	D5	<REF> 		INTERNAL OR EXTERNAL
 * 	D4	<PDN CHD>	POWER DOWN CH D
 * 	D3	<PDN CHC>	POWER DOWN CH C
 * 	D2	<PDN CHB>	POWER DOWN CH B
 * 	D1	<PDN CHA>	POWER DOWN CH A
 * 	D0	<PDN>		GLOBAL POWER DOWN
 *
 * Input Manager IM0 register:
 *  SWZ(3..0) Dbi(3..0)
 *  SWZ(x) = 1	Connect ground to ZAP
 *  SWZ(x) = 0	Connect VDD to ZAP
 *
 *  Dbi(x) = 1	Enable anti-sparkling protection for AGET
 *  Dbi(x) = 0	Disable anti-sparkling protection for AGET
 *
 * Input Manager IM1 register:
 *  ENW(3..0) ENR(3..0)
 *  ENW(x) = 1	Enable WCK clock at the input of AGET x
 *  			Enable SCA Write signal to AGETx
 *  ENW(x) = 0	Disable WCK clock at the input of AGET x
 *  			Disable SCA Write signal to AGETx
 *  			e.g. disable sampling command
 *  ENR(x) = 1	Enable RCK clock at the input of AGET x
 *  			Enable SCA Read signal to AGETx
 *  ENR(x) = 0	Disable RCK clock at the input of AGET x
 *  			Disable SCA Read signal to AGETx
 *  			e.g. disable readout command
 */
void GetController::asadAgetInputManagerInit(const size_t& coboIdx)
{
	for (size_t asadIdx = 0; asadIdx < 4u; ++asadIdx)
	{
		if (not isAsadActive(coboIdx, asadIdx)) continue;
		MSG_IF(verbose) << "Activating AsAd no. " << asadIdx << " ADC channels";

		// Default values
		uint8_t adc0 = 0;	// Enable all four ADC channels
		uint8_t im0 = 0xF; 	// Connect VDD to ZAP, Enable anti-sparkling protection
		uint8_t im1 = 0xFF;	// Allow sampling and readout for all four AGET chips

		// Loop over AGET chips
		for (size_t agetIdx = 0; agetIdx < 4u; ++agetIdx)
		{
			// Check whether to activate slot
			bool isChipActive = isAgetActive(coboIdx, asadIdx, agetIdx);
			bool enablePosition = true;
			bool connectGroundToZAP = false; // SWZ
			bool antiSparklingEnabled = true; // Dbi

			try
			{
				enablePosition = agetConfig(coboIdx, asadIdx, agetIdx)("Control")("enableAsAdPosition").getBoolValue();
				connectGroundToZAP = agetConfig(coboIdx, asadIdx, agetIdx)("Control")("connectGroundToZAP").getBoolValue();
				antiSparklingEnabled = agetConfig(coboIdx, asadIdx, agetIdx)("Control")("antiSparklingEnabled").getBoolValue();
			}
			catch (const CCfg::CConfig::NotFound & e)
			{
				//std::cerr << e.what() << std::endl;
			}

			// Disable slot
			if (not isChipActive or not enablePosition)
			{
				MSG_IF(verbose) << "Disabling position no. " << agetIdx << " of AsAd no. " << asadIdx;
				// Power down ADC channel
				BitFieldHelper< uint8_t >::setBit(adc0, 1 + agetIdx, true);
				// Disable sampling command on AGET input
				BitFieldHelper< uint8_t >::setBit(im1, 4 + agetIdx, false);
				// Disable readout command on AGET input
				BitFieldHelper< uint8_t >::setBit(im1, agetIdx, false);
			}

			// Connect/disconnect VDD to/from ZAP interface
			BitFieldHelper< uint8_t >::setBit(im0, 4 + agetIdx, connectGroundToZAP);
			// Enable/disable anti-sparkling protection for AGET
			BitFieldHelper< uint8_t >::setBit(im0, agetIdx, antiSparklingEnabled);
		}

		// Set mask for broadcast of AsAd slow control command
		selectAsAdForSlowControl(asadIdx);

		// Send command
		eccClient().asadConfigureAgetInputs(adc0, im0, im1);
	}

	eccClient().setAsAdSlowControlMask(asadMask_);
}
//_________________________________________________________________________________________________
/**
 * Initializes CoBo registers to communicate with AGET
 */
void GetController::agetInit(const size_t& coboIdx)
{
	eccClient().selectCalibChips(rc::CoBoConfigParser::getCalibrationChips(coboConfig(coboIdx)));
	eccClient().agetInit();
}
//__________________________________________________________________________________________________
/**
 * Initializes AsAd monitoring registers.
 * See GET-AS-002-0004: AsAd Power Supply and Monitoring document.
 */
void GetController::asadAlertsInit(const size_t& coboIdx)
{
	std::cout << "Configuring AsAd auto-protection..." << std::endl;
	for (size_t asadIdx = 0; asadIdx < 4u; ++asadIdx)
	{
		if (isAsadActive(coboIdx, asadIdx))
		{
			// Select AsAd board for slow control
			selectAsAdForSlowControl(asadIdx);

			// Step 1: define upper and lower limits
			asadMonitoringSetLimits(coboIdx, asadIdx);
			// Step 2: define auto-protection level
			asadSetAutoProtectionLevel(coboIdx, asadIdx);
			// Step 3: define operational mode
			asadMonitoringSetOperationalMode(coboIdx, asadIdx);
		}
	}
	eccClient().setAsAdSlowControlMask(asadMask_);
}
//__________________________________________________________________________________________________
void GetController::subscribeToAlarms(const size_t& coboIdx)
{
	try
	{
		// Check whether to monitor AsAd alarms
		bool checkAsAd = true;
		try
		{
			checkAsAd = coboConfig(coboIdx)("Alarms")("checkAsAd").getBoolValue();
		}
		catch (const CCfg::CConfig::NotFound & e)
		{
			std::cerr << e.what() << std::endl;
		}
		// Enable monitoring of AsAd alarms
		std::cerr << "Enabling periodic check of AsAd alarm bits by CoBo" << std::endl;
		eccClient().ecc()->setAsAdAlarmMonitoringEnabled(checkAsAd);
		// Remove all subscribers
		mt::AlarmServicePrx alarmSvce = eccClient().ecc()->alarmService();
		alarmSvce->reset();
		// Check whether to subscribe
		bool subscribe = true;
		try
		{
			subscribe = coboConfig(coboIdx)("Alarms")("subscribe").getBoolValue();
		}
		catch (const CCfg::CConfig::NotFound & e)
		{
			std::cerr << e.what() << std::endl;
		}
		// Subscribe
		if (subscribe)
		{
			std::cerr << "Subscribing to CoBo alarm notifications" << std::endl;
			const std::string id = std::string("GetController-");// + QUuid::createUuid().toString().toStdString();
//			alarmSvce->subscribe(id, alarmLogger());
		}
	}
	catch (::mdaq::utl::CmdException & e)
	{
		std::cerr << e.reason << std::endl;
	}
}
//_________________________________________________________________________________________________
/**
 * Read AsAd voltages, currents, temperatures.
 */
void GetController::asadMonitoringRead(const size_t& coboIdx)
{
	eccClient().setAsAdSlowControlMask(asadMask_);
	for (size_t asadIdx = 0; asadIdx < 4; ++asadIdx)
	{
		if (isAsadActive(coboIdx, asadIdx))
		{
			// Read AsAd voltages, currents, temperatures
			readAsAdVdd(asadIdx);
			readAsAdVdd1(asadIdx);
			readAsAdIdd(asadIdx);
			const int16_t Ti = readAsAdIntTemp(asadIdx);
			std::cout << "Read AsAd " << asadIdx << " temperature far from ADC: " << Ti << " deg C" << std::endl;
			const int16_t Te = readAsAdExtTemp(asadIdx);
			std::cout << "Read AsAd " << asadIdx << " temperature close to ADC: " << Te << " deg C" << std::endl;
		}
	}
}
//__________________________________________________________________________________________________
/**
 * Configure and start AsAd monitoring.
 * See GET-AS-002-0004: AsAd Power Supply and Monitoring
 * See email from J. Pibernat, Apr 7, 2017:
 * The goal of this step is to ensure the AsAd boards are fully powered
 * (the previous steps only showed that the FPGA and the monitoring chips were powered).
 * Since each GET setup uses a different power supply, the powering delay can vary a lot from setup to setup.
 * Therefore, we should ensure that the VDD voltage is high enough and, for that,
 * we need to first configure and start the monitoring device.
 * At this step, we only want to enable the monitoring, not the protection of the board.
 * So, before starting the monitoring we disable auto-protection.
 */
void GetController::asadMonitoringInit(const size_t& coboIdx)
{
	std::cout << "Initializing AsAd monitoring registers" << std::endl;
	for (size_t asadIdx = 0; asadIdx < 4; ++asadIdx)
	{
		if (isAsadActive(coboIdx, asadIdx))
		{
			// Select AsAd board for slow control
			selectAsAdForSlowControl(asadIdx);

			// Reset AsAd monitoring device
			eccClient().asadMonitoringReset();

			// Disable auto-protection
			eccClient().asadDisableAutoProtection(asadIdx);

			// Define upper and lower limits
			asadMonitoringSetLimits(coboIdx, asadIdx);

			// Define operational mode
			const bool fastMode = asadConfig(coboIdx, asadIdx)("Monitoring")("fastConversionEnabled").getBoolValue(false);
			const uint8_t singleChannelItem = asadGetSingleChannelItem(coboIdx, asadIdx);
			const bool singleChannelMode = asadConfig(coboIdx, asadIdx)("Monitoring")("singleChannelModeEnabled").getBoolValue(false);
			const bool averagingDisabled = asadConfig(coboIdx, asadIdx)("Monitoring")("averagingDisabled").getBoolValue(false);
			const bool enableAlarms = false;
			asadMonitoringSetOperationalMode(enableAlarms, fastMode, singleChannelItem, singleChannelMode, averagingDisabled);
		}
	}
	eccClient().setAsAdSlowControlMask(asadMask_);
}
//_________________________________________________________________________________________________
void GetController::asadMonitoringCheckVDD(const size_t& coboIdx, const size_t maxReads, const double delay_ms)
{
	for (size_t asadIdx = 0; asadIdx < 4; ++asadIdx)
	{
		if (isAsadActive(coboIdx, asadIdx))
		{
			bool ok = false;
			const double vddMin = asadConfig(coboIdx, asadIdx)("Monitoring")("LimitsVDD")("lowerValue").getRealValue(3.2);
			std::cerr << "Waiting for AsAd voltage in AsAd no. " << asadIdx << " to settle above " << vddMin << " V ..." << std::endl;
			for (size_t i = 0; i < maxReads; ++i)
			{
				// Read VDD until high enough
				const double vdd = readAsAdVdd(asadIdx);
				if (vdd >= vddMin)
				{
					ok = true;
					break;
				}
        usleep(delay_ms*1000);
			}
			if (not ok)
			{
				std::ostringstream msg;
				msg << "VDD voltage of AsAd board no. " << asadIdx << " did not reach " << vddMin
						<< " V! Check the hardware setup of your power supply or contact CENBG for further information.";
        std::cerr << msg.str() << std::endl;
				throw std::runtime_error(msg.str());
			}

		}
	}
}
//__________________________________________________________________________________________________
/**
 * Sets AsAd board auto-protection level.
 * See GET-AS-002-0004: AsAd Power Supply and Monitoring
 */
void GetController::asadSetAutoProtectionLevel(const size_t& coboIdx, const size_t& asadIdx)
{
	const bool enableAlarms = asadConfig(coboIdx, asadIdx)("Monitoring")("isAlarmSet").getBoolValue();

	// Set either highest or lowest auto-protection level
	// "interruptMask1" (highest protection level: 0x23, lowest: 0xFF)
	//     B0: (reserved) Write 1 only (disables Tint high limit)
	//     B1: (reserved) Write 1 only (disables Tint low limit)
	//     B2: If set, disables auto-protection caused by Text high limit.
	//     B3: If set, disables auto-protection caused by Text low limit.
	//     B4: If set, disables auto-protection caused by Text out of order.
	//     B5: (reserved) Write 1 only.
	//     B6: If set, disables auto-protection caused by VDD limits.
	//     B7: If set, disables auto-protection caused by IDD limits.
	uint8_t interruptMask1 = enableAlarms ? 0x23 : 0xFF;
	if (enableAlarms)
	{
		bool flag = false;
		try
		{
			flag = asadConfig(coboIdx, asadIdx)("Monitoring")("LimitsText")("disableAlertsTooHigh").getBoolValue();
			::utl::BitFieldHelper< uint8_t >::setBit(interruptMask1, 2u, flag);
			flag = asadConfig(coboIdx, asadIdx)("Monitoring")("LimitsText")("disableAlertsTooLow").getBoolValue();
			::utl::BitFieldHelper< uint8_t >::setBit(interruptMask1, 3u, flag);
			flag = asadConfig(coboIdx, asadIdx)("Monitoring")("LimitsText")("disableAlertsOutOfOrder").getBoolValue();
			::utl::BitFieldHelper< uint8_t >::setBit(interruptMask1, 4u, flag);
			flag = asadConfig(coboIdx, asadIdx)("Monitoring")("LimitsVDD")("disableAlerts").getBoolValue();
			::utl::BitFieldHelper< uint8_t >::setBit(interruptMask1, 6u, flag);
			flag = asadConfig(coboIdx, asadIdx)("Monitoring")("LimitsIDD")("disableAlerts").getBoolValue();
			::utl::BitFieldHelper< uint8_t >::setBit(interruptMask1, 7u, flag);
		}
		catch (const CCfg::CConfig::NotFound & e)
		{
			std::cerr << e.what() << std::endl;
		}
	}

	// Leaving "interruptMask2" to its default value (has to do with VDD1)
	const uint8_t interruptMask2 = 0;

	eccClient().asadSetAutoProtectionLevel(interruptMask1, interruptMask2);
}
//__________________________________________________________________________________________________
/**
 * Initializes the 3-bit global threshold of all active AGET chips. (Why?)
 */
void GetController::initializeGlobalThresholds(const size_t& coboIdx, const uint16_t value)
{
	std::cerr << "Initializing global thresholds" << std::endl;

	eccClient().setAgetSlowControlMask(agetMask_);

	for (size_t asadIdx=0; asadIdx < 4; ++asadIdx)
	{
		for (size_t agetIdx=0; agetIdx < 4; ++agetIdx)
		{
			if (not isAgetActive(coboIdx, asadIdx, agetIdx)) continue;

			setGlobalRegThreshold(asadIdx, agetIdx, value);
		}
	}
}
//__________________________________________________________________________________________________
/**
 * Adjusts the 3-bit global threshold in AGET register 1.
 */
void GetController::setGlobalRegThreshold(const std::size_t& asadIdx, const std::size_t& agetIdx, const uint32_t& value)
{
	MSG_IF(verbose) << "Setting global threshold for AGET no. " << asadIdx*4+agetIdx << " to " << value;

	uint64_t regValue = eccClient().agetRegRSC(asadIdx, agetIdx, "reg1");
	::utl::BitFieldHelper< uint64_t >::setField(regValue, 19, 3, value);

	eccClient().agetRegWSC(asadIdx, agetIdx, "reg1", regValue);
}
//__________________________________________________________________________________________________
/**
 * Triggers automatic calibration of data delays, if required by configuration.
 */
void GetController::calibrateAutoReadDataDelay(int coboIdx)
{
	if (isCoboActive(coboIdx))
	{
		bool calcEnabled = true;
		try
		{
			calcEnabled = coboConfig(coboIdx)("Module")("calibrateAutoReadDataDelay").getBoolValue();
		}
		catch (const CCfg::CConfig::NotFound &)
		{
			// Keep default value
		}
		if (calcEnabled)
			eccClient().calibrateAutoReadDataDelay();
	}
}
//__________________________________________________________________________________________________
/**
 * Checks whether to use the automatically calibrated read data delays.
 *
 */
bool GetController::isSCRAutoDelayEnabled(const size_t& coboIdx, const size_t& asadIdx)
{
	bool enable = false;

	try
	{
		enable = asadConfig(coboIdx, asadIdx)("Control")("enableAutoReadDataDelay").getBoolValue();
	}
	catch (const CCfg::CConfig::NotFound &)
	{
		try
		{
			enable = coboConfig(coboIdx)("Module")("enableAutoReadDataDelay").getBoolValue();
		}
		catch (const CCfg::CConfig::NotFound &)
		{
			// Keep default value
		}
	}

	return enable;
}
//_________________________________________________________________________________________________
/**
 * Selects automatic or manual SCR data delay, depending on the value of the
 * Setup[Conditions].CoBo[*].Module.enableAutoReadDataDelay parameter
 * in the the configuration.
 * @param coboId Index of the CoBo board.
 */
void GetController::selectReadDataDelay(const size_t& coboIdx)
{
	if (isCoboActive(coboIdx))
	{
		for (size_t asadIdx = 0; asadIdx < 4u; asadIdx++ )
		{
			if (isAsadActive(coboIdx, asadIdx))
			{
				eccClient().asadSetAutoReadDataDelayEnabled(asadIdx, isSCRAutoDelayEnabled(coboIdx, asadIdx));
			}

		}
	}
}
//__________________________________________________________________________________________________
/**
 * Selects a unique AsAd board for a slow control command.
 * @param asadIdx Index of the AsAd board to select.
 */
void GetController::selectAsAdForSlowControl(const size_t& asadIdx)
{
	const uint16_t mask = (1ul << asadIdx);
	eccClient().setAsAdSlowControlMask(mask);
	eccClient().selectAsAdForRead(asadIdx);
}
//__________________________________________________________________________________________________
/**
 * Sets ReadIfHit registers for a given AGET chip according to values in configuration file.
 * @param coboIdx Index of the CoBo board.
 * @param asadIdx Index of the AsAd board the AGET belongs to (0 to 3).
 * @param agetIdx Index of the AGET chip within its AsAd board (0 to 3).
 */
void GetController::setReadIfHitMask(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx)
{
	const uint32_t value1_4 = getChannelReadingMask(coboIdx, asadIdx, agetIdx, 0, 3, "only_if_hit");
	const uint32_t value5_36 = getChannelReadingMask(coboIdx, asadIdx, agetIdx, 4, 35, "only_if_hit");
	const uint32_t value37_68 = getChannelReadingMask(coboIdx, asadIdx, agetIdx, 36, 67, "only_if_hit");

	eccClient().setReadIfHitMask(asadIdx, agetIdx, value1_4, value5_36, value37_68);
}
//__________________________________________________________________________________________________
/**
 * Sets ReadAlways registers for a given AGET chip according to values in configuration file.
 * @param coboIdx Index of the CoBo board.
 * @param asadIdx Index of the AsAd board the AGET belongs to (0 to 3).
 * @param agetIdx Index of the AGET chip within its AsAd board (0 to 3).
 */
void GetController::setReadAlwaysMask(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx)
{
	const uint32_t value1_4 = getChannelReadingMask(coboIdx, asadIdx, agetIdx, 0, 3, "always");
	const uint32_t value5_36 = getChannelReadingMask(coboIdx, asadIdx, agetIdx, 4, 35, "always");
	const uint32_t value37_68 = getChannelReadingMask(coboIdx, asadIdx, agetIdx, 36, 67, "always");

	eccClient().setReadAlwaysMask(asadIdx, agetIdx, value1_4, value5_36, value37_68);
}
//__________________________________________________________________________________________________
/**
 * Sets thresholds of the 68 channels for zero suppression.
 * @param coboIdx CoBo ID
 * @param asadIdx AsAd ID
 * @param agetIdx AGET chip ID
 */
void GetController::setZeroSuppressionThresholds(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx)
{
	std::cerr << "Setting zero suppression thresholds for AGET no. " << asadIdx*4+agetIdx << std::endl;
	try
	{
		for (size_t channelId = 0; channelId < 68; ++channelId)
		{
			const uint16_t threshold = agetConfig(coboIdx, asadIdx, agetIdx)("channel", channelId)("zeroSuppressionThreshold").getIntValue();
			eccClient().setCoBoZeroSuppressionThreshold(asadIdx, agetIdx, channelId, threshold);
		}
	}
	catch (const CCfg::CConfig::NotFound & e)
	{
		std::cerr << e.what() << " Zero suppression thresholds have not been configured." << std::endl;
	}
}
//_________________________________________________________________________________________________
void GetController::asadMonitoringSetLimits(const size_t& coboIdx, const size_t& asadIdx)
{
	DBG_IF(verbose) << "Defining AsAd no. " << asadIdx << " monitoring limits";
	eccClient().asadMonitoringStop();

	asadSetExtTempLimits(coboIdx, asadIdx);
	asadSetVddLimits(coboIdx, asadIdx);
	asadSetIddLimits(coboIdx, asadIdx);
	//asadSetTintLimits(coboIdx, asadIdx);
	//asadSetVdd1Limits(coboIdx, asadIdx);
}
//_________________________________________________________________________________________________
/**
 * Returns ID for channel chosen for single channel monitoring.
 * Used to set the fist 3 bits of the 'configuration2' AsAd register.
 */
uint8_t GetController::asadGetSingleChannelItem(const size_t& coboIdx, const size_t& asadIdx)
{
	std::string itemString = asadConfig(coboIdx, asadIdx)("Monitoring")("singleChannelItem").getStringValue("VDD1");
	// 000: VDD1; 001: Internal temperature; 010: External temperature; 100: VDD; 101: IDD
	uint8_t itemId = 0;
	if ("Tint" == itemString)
		itemId = 1;
	else if ("Text" == itemString)
		itemId = 2;
	else if ("VDD" == itemString)
		itemId = 4;
	else if ("IDD" == itemString)
		itemId = 5;
	return itemId;
}
//__________________________________________________________________________________________________
/**
 * Sets AsAd monitoring device operational mode.
 * See GET-AS-002-0004: AsAd Power Supply and Monitoring
 */
void GetController::asadMonitoringSetOperationalMode(bool enableAlarms, bool averagingDisabled,
                                                     bool singleChannelMode, uint8_t singleChannelItem, bool fastMode)
{
	// asad()->writeRegister("intTempOffset", 0x0);
	// asad()->writeRegister("extTempOffset", 0x0);

	// "configuration3" (default value:0x0)
	//                B0 0: ADC for monitoring in slow conversion mode (clock @  1.4 kHz)
	//                   1: ADC for monitoring in fast conversion mode (clock @ 22.5 kHz) and analog filters disabled
	//                B1 0: (not used)
	//                B2 0: (reserved)
	//                B3 0: (not used)
	//                B4 0: internal ADC reference given by an internal Vref
	//                B5 0: (not used)
	//                B6 0: (not used)
	//                B7 0: (reserved)
	uint8_t configuration3 = 0;
	::utl::BitFieldHelper< uint8_t>::setBit(configuration3, 0, fastMode);

	// "configuration2" (default value:0x0)
	//               <B0:B2> Defines the monitored item in single channel mode
	//                        000: VDD1; 001: Internal temperature; 010: External temperature; 100: VDD; 101: IDD
	//                        (Round robin mode has to be selected by default instead of single channel mode)
	//                B3 0: (reserved)
	//                B4 0: round robin mode
	//                   1: single channel mode
	//                B5 0: enable averaging every measurement on all item 16 times
	//                   1: disable averaging
	//                B6 0: (not used)
	//                B7 1: software reset
	uint8_t configuration2 = 0;
	::utl::BitFieldHelper< uint8_t>::setField(configuration2, 0, 3, singleChannelItem);
	::utl::BitFieldHelper< uint8_t>::setBit(configuration2, 4, singleChannelMode);
	::utl::BitFieldHelper< uint8_t>::setBit(configuration2, 5, averagingDisabled);


	// "configuration1" (default value:0x0)
	//    B0 0: stop monitoring
	//       1:start monitoring
	//    <B1:B2> 10: ADS6422 temperature measurement
	//    B3 1: VDD value measurement
	//    B4 0: (reserved)
	//    B5 0: Alert enabled
	//       1: Alert disabled
	//    B6 0: Alert output active low on AsAd (high on CoBo)
	//    PD 0: Power up
	//       1: Power down
	uint8_t configuration1 = 0x0D;
	::utl::BitFieldHelper< uint8_t>::setBit(configuration1, 5, enableAlarms?0:1);

	eccClient().asadSetMonitoringMode(configuration1, configuration2, configuration3);

  std::cout << "Set operational mode of AsAd monitoring device to:" << std::hex << std::showbase
			<< " cfg1=" << (uint16_t) configuration1
			<< " cfg2=" << (uint16_t) configuration2
			<< " cfg3=" << (uint16_t) configuration3 << std::dec << std::endl;
}
//_________________________________________________________________________________________________
/**
 * Defines AsAd auto-protection level and monitoring mode.
 */
void GetController::asadMonitoringSetOperationalMode(const size_t& coboIdx, const size_t& asadIdx)
{
	// Define operational mode
	// "configuration3" (default value:0x0)
	//                B0 0: ADC for monitoring in slow conversion mode (clock @  1.4 kHz)
	//                   1: ADC for monitoring in fast conversion mode (clock @ 22.5 kHz) and analog filters disabled
	//                B1 0: (not used)
	//                B2 0: (reserved)
	//                B3 0: (not used)
	//                B4 0: internal ADC reference given by an internal Vref
	//                B5 0: (not used)
	//                B6 0: (not used)
	//                B7 0: (reserved)
	bool fastMode = asadConfig(coboIdx, asadIdx)("Monitoring")("fastConversionEnabled").getBoolValue(false);

	// "configuration2" (default value:0x0)
	//               <B0:B2> Defines the monitored item in single channel mode
	//                        000: VDD1; 001: Internal temperature; 010: External temperature; 100: VDD; 101: IDD
	//                        (Round robin mode has to be selected by default instead of single channel mode)
	//                B3 0: (reserved)
	//                B4 0: round robin mode
	//                   1: single channel mode
	//                B5 0: enable averaging every measurement on all item 16 times
	//                   1: disable averaging
	//                B6 0: (not used)
	//                B7 1: software reset
	bool singleChannelMode = asadConfig(coboIdx, asadIdx)("Monitoring")("singleChannelModeEnabled").getBoolValue(false);
	bool averagingDisabled = asadConfig(coboIdx, asadIdx)("Monitoring")("averagingDisabled").getBoolValue(false);
	uint8_t singleChannelItem = asadGetSingleChannelItem(coboIdx, asadIdx);

	// "configuration1" (default value:0x0)
	//    B0 0: stop monitoring
    //       1:start monitoring
	//    <B1:B2> 10: ADS6422 temperature measurement
	//    B3 1: VDD value measurement
	//    B4 0: (reserved)
	//    B5 0: Alert enabled
  	//       1: Alert disabled
	//    B6 0: Alert output active low on AsAd (high on CoBo)
	//    PD 0: Power up
	//       1: Power down
	const bool enableAlarms = asadConfig(coboIdx, asadIdx)("Monitoring")("isAlarmSet").getBoolValue();

	asadMonitoringSetOperationalMode(enableAlarms, fastMode, singleChannelItem, singleChannelMode, averagingDisabled);
}
//_________________________________________________________________________________________________
/**
 * Reads AsAd VDD voltage (see AsAd Power Supply and Monitoring document).
 * Converts straight binary value to Volt.
 * @return Value of VDD voltage, in Volts.
 */
double GetController::readAsAdVdd(const std::size_t& asadIdx)
{
	uint64_t binValue = 0;
	for (int i = 0; i < 10 and binValue == 0; ++i)
	{
		binValue = eccClient().readVdd(asadIdx);
	}
	double value = (binValue != 0) ? (2.28 / 256) * (3.3 / 2) * binValue : 0.0;
	std::cerr << "Read AsAd " << asadIdx << " VDD voltage: " << value << " V" << std::endl;
	return value;
}
//_________________________________________________________________________________________________
/**
 * Reads AsAd VDD1 voltage (see AsAd Power Supply and Monitoring document).
 * Converts straight binary value to Volt.
 * @return Value of VDD1 voltage, in Volts.
 */
double GetController::readAsAdVdd1(const std::size_t& asadIdx)
{
	uint64_t binValue = 0;
	for (int i = 0; i < 10 and binValue == 0; ++i)
	{
		binValue = eccClient().readVdd1(asadIdx);
	}
	double value = (binValue != 0) ? (60e-3 + 7 * ((double) binValue) / 256) : 0.0;
	std::cerr << "Read AsAd " << asadIdx << " VDD1 voltage: " << value << " V" << std::endl;
	return value;
}
//_________________________________________________________________________________________________
/**
 * Reads AsAd IDD current (see AsAd Power Supply and Monitoring document).
 * Converts straight binary value to A.
 * @return Value of IDD current, in A.
 */
double GetController::readAsAdIdd(const std::size_t& asadIdx)
{
	uint64_t binValue = 0;
	for (int i = 0; i < 10 and binValue == 0; ++i)
	{
		binValue = eccClient().readIdd(asadIdx);
	}
	double value = binValue != 0 ? (2.28 / 256) * (1 / 0.755) * binValue : 0.0;
	std::cerr << "Read AsAd " << asadIdx << " IDD current: " << value << " A" << std::endl;
	return value;
}
//_________________________________________________________________________________________________
/**
 * Reads AsAd on-chip temperature (see AsAd Power Supply and Monitoring document).
 * Converts straight binary value to degrees C.
 * @return Value of on-chip temperature, in degrees C.
 */
int8_t GetController::readAsAdIntTemp(const std::size_t& asadIdx)
{
	uint8_t binValue = 0;
	for (int i = 0; i < 10 and binValue == 0; ++i)
	{
		binValue = eccClient().readIntTemp(asadIdx);
	}
	uint8_t value = ((binValue & 0x80) == 0x8)? ~(binValue - 1) : binValue;
	return value;
}
//_________________________________________________________________________________________________
/**
 * Reads AsAd ADC-chip surroundings temperature (see AsAd Power Supply and Monitoring document).
 * Converts straight binary value to degrees C.
 * @return Value of ADC-chip surroundings temperature, in degrees C.
 */
int8_t GetController::readAsAdExtTemp(const std::size_t& asadIdx)
{
	uint8_t binValue = 0;
	for (int i = 0; i < 10 and binValue == 0; ++i)
	{
		binValue = eccClient().readExtTemp(asadIdx);
	}
	uint8_t value = ((binValue & 0x80) == 0x8) ? ~(binValue - 1) : binValue;
	return value;
}
//_________________________________________________________________________________________________
uint8_t GetController::getExtTempLowLimit(int iCobo, int iAsad)
{
	uint8_t lowerLimit = 0;
	if (isAsadActive(iCobo, iAsad))
	{
		try
		{
			lowerLimit = asadConfig(iCobo, iAsad)("Monitoring")("LimitsText")("lowerValue").getIntValue();
		}
		catch (const CCfg::CConfig::NotFound &)
		{
			lowerLimit = asadConfig(iCobo, iAsad)("Monitoring")("ExternalTempLowLimit").getIntValue();
		}
	}
	return lowerLimit;
}
//_________________________________________________________________________________________________
uint8_t GetController::getExtTempHighLimit(int iCobo, int iAsad)
{
	uint8_t upperLimit = 0xFF;
	if (isAsadActive(iCobo, iAsad))
	{
		try
		{
			upperLimit = asadConfig(iCobo, iAsad)("Monitoring")("LimitsText")("upperValue").getIntValue();
		}
		catch (const CCfg::CConfig::NotFound &)
		{
			upperLimit = asadConfig(iCobo, iAsad)("Monitoring")("ExternalTempHighLimit").getIntValue();
		}
	}
	return upperLimit;
}
//_________________________________________________________________________________________________
void GetController::asadSetExtTempLimits(const size_t& coboIdx, const size_t& asadIdx)
{
	uint8_t lowerLimit = getExtTempLowLimit(coboIdx, asadIdx);
	uint8_t upperLimit = getExtTempHighLimit(coboIdx, asadIdx);

	eccClient().wExtTempLimit(lowerLimit, upperLimit);
}
//_________________________________________________________________________________________________
double GetController::getVddLowLimit(int iCobo, int iAsad)
{
	double lowerVoltage = 0.0;
	if (isAsadActive(iCobo, iAsad))
	{
		try
		{
			lowerVoltage = asadConfig(iCobo, iAsad)("Monitoring")("LimitsVDD")("lowerValue").getRealValue();
		}
		catch (const CCfg::CConfig::NotFound &)
		{
			lowerVoltage = asadConfig(iCobo, iAsad)("Monitoring")("VddLowLimit").getRealValue();
		}
	}
	return lowerVoltage;
}
//_________________________________________________________________________________________________
double GetController::getVddHighLimit(int iCobo, int iAsad)
{
	double upperVoltage = 3.74;
	if (isAsadActive(iCobo, iAsad))
	{
		try
		{
			upperVoltage = asadConfig(iCobo, iAsad)("Monitoring")("LimitsVDD")("upperValue").getRealValue();
		}
		catch (const CCfg::CConfig::NotFound &)
		{
			upperVoltage = asadConfig(iCobo, iAsad)("Monitoring")("VddHighLimit").getRealValue();
		}
	}
	return upperVoltage;
}
//_________________________________________________________________________________________________
void GetController::asadSetVddLimits(const size_t& coboIdx, const size_t& asadIdx)
{
	const double lowerVoltage = getVddLowLimit(coboIdx, asadIdx);
	const double upperVoltage = getVddHighLimit(coboIdx, asadIdx);

	const uint8_t lowerLimit = 512. * lowerVoltage / 7.524;
	const uint8_t upperLimit = 512. * upperVoltage / 7.524;

	eccClient().wVddLimit(lowerLimit, upperLimit);
}
//_________________________________________________________________________________________________
double GetController::getIddLowLimit(int iCobo, int iAsad)
{
	double lowerCurrent = 0.0;
	if (isAsadActive(iCobo, iAsad))
	{
		try
		{
			lowerCurrent = asadConfig(iCobo, iAsad)("Monitoring")("LimitsIDD")("lowerValue").getRealValue();
		}
		catch (const CCfg::CConfig::NotFound &)
		{
			lowerCurrent = asadConfig(iCobo, iAsad)("Monitoring")("IddLowLimit").getRealValue();
		}
	}
	return lowerCurrent;
}
//_________________________________________________________________________________________________
double GetController::getIddHighLimit(int iCobo, int iAsad)
{
	double upperCurrent = 3.0;
	if (isAsadActive(iCobo, iAsad))
	{
		try
		{
			upperCurrent = asadConfig(iCobo, iAsad)("Monitoring")("LimitsIDD")("upperValue").getRealValue();
		}
		catch (const CCfg::CConfig::NotFound &)
		{
			upperCurrent = asadConfig(iCobo, iAsad)("Monitoring")("IddHighLimit").getRealValue();
		}
	}
	return upperCurrent;
}
//_________________________________________________________________________________________________
void GetController::asadSetIddLimits(const size_t& coboIdx, const size_t& asadIdx)
{
	const double lowerCurrent = getIddLowLimit(coboIdx, asadIdx);
	const double upperCurrent = getIddHighLimit(coboIdx, asadIdx);

	const uint8_t lowerLimit = 193.28 * lowerCurrent / 2.28;
	const uint8_t upperLimit = 193.28 * upperCurrent / 2.28;

	eccClient().wIddLimit(lowerLimit, upperLimit);
}
//__________________________________________________________________________________________________
/**
 * Computes value to give to AsAd LCM2 register from configuraton parameters.
 */
uint64_t GetController::getAsAdLcm2Value(const size_t& coboIdx, const size_t& asadIdx)
{
	if (isCoboActive(coboIdx))
	{
		// Get CKR frequency
		float freqCKR_MHz = coboConfig(coboIdx)("Module")("readingClockFrequency").getRealValue(25.);

		if (isAsadActive(coboIdx, asadIdx))
		{
			CCfg::CConfig asadCfg(asadConfig(coboIdx, asadIdx));

			// Get ADC phase shift
			int32_t adcClockPhaseShift_deg = asadCfg("Clocking")("phaseShiftAdcClock").getIntValue(270);

			// Get ADC delay
			int32_t adcClockDelay_ps = asadCfg("Clocking")("delayAdcClock").getIntValue(5335);

			// Compute LCM2 value
			uint64_t lcm2Value = asad::utl::buildLcm2RegisterValue(freqCKR_MHz, adcClockPhaseShift_deg, adcClockDelay_ps);

			// Patch LCM2 value
			const bool lcm2DebugMode = asadCfg("Clocking")("Debug")("LCM2")("debugModeEnabled").getBoolValue(false);
			if (lcm2DebugMode)
			{
				::get::asad::utl::Lcm2Config lcm2Config;
				asad::utl::parseAsAdLcm2DebugConfig(asadCfg, lcm2Config);
				asad::utl::patchLcm2RegisterValue(lcm2Value, lcm2Config);
			}

			return lcm2Value;
		}
	}
	return UINT64_C(0);
}
//_________________________________________________________________________________________________
/**
 * Switches to slow control mode.
 */
void GetController::switchToSlowControl(const size_t & /* coboIdx */)
{
	eccClient().switchToSlowControl();
}
//__________________________________________________________________________________________________
/**
 * Selects a unique AGET chip for a slow control command.
 * @param asadIdx Index of the AsAd board.
 * @param asadIdx Index of the AGET chip to select.
 */
void GetController::selectAgetForSlowControl(const size_t& asadIdx, const size_t agetIdx)
{
	const uint16_t mask = (1ul << (4*asadIdx + agetIdx));
	eccClient().setAgetSlowControlMask(mask);
	eccClient().selectAgetForRead(asadIdx, agetIdx);
}
//__________________________________________________________________________________________________
void GetController::enableFPNRead(int iCobo, int iAsad, int iAget)
{
	eccClient().enableFPNRead(isFPNRead(iCobo,iAsad,iAget));
}
//_________________________________________________________________________________________________
/**
 * Checks whether we should verify AGET slow control data have been correctly written.
 */
bool GetController::agetIsScCheck(int iCobo, int iAsad, int iAget)
{
	bool isCheck = agetConfig(iCobo, iAsad, iAget)("Control")("scCheckMode").getBoolValue();
	MSG_IF(verbose) << "Check of the configuration of AGET registers is " << (isCheck ? "" : "NOT ") << "enabled";
	return isCheck;
}
//_________________________________________________________________________________________________
/**
 * Sets AsAd register for inspection manager to select signals.
 */
void GetController::setAsAdInspectionManager(const std::size_t& coboIdx, const std::size_t& asadIdx)
{
	std::string device, isp1, isp2;
	if (getInspectionLinesParameters(device, isp1, isp2, coboIdx, asadIdx))
	{
		std::cerr << "Setting AsAd no. " << asadIdx << " inspection manager for signals (" << isp1 << ", " << isp2 << ") of device " << device << std::endl;
		eccClient().asadSetISPM(coboIdx, asadIdx, device, isp1, isp2);
	}
}
//__________________________________________________________________________________________________
/**
 * Enables or disables the modification of the hit register before readout.
 */
void GetController::setWriteHitRegisterEnabled(int iCobo)
{
	const bool enabled = coboConfig(iCobo)("Module")("enableWriteHittedregister").getBoolValue();
	std::cerr << "Modification of hit pattern before readout has been " << (enabled?"enabled":"disabled") << std::endl;
	eccClient().setWriteHitRegisterEnabled(enabled);
}
//__________________________________________________________________________________________________
/**
 * Configures the test mode where the CoBo frames contain an incremental counter instead of data.
 */
void GetController::setAcqMode(const size_t& coboIdx)
{
	if (isCoboActive(coboIdx))
	{
		eccClient().acqTestMode(coboConfig(coboIdx)("Module")("isAcqTestMode").getBoolValue());
	}
}
//__________________________________________________________________________________________________
/**
 * Sets multiplicity mode configuration parameters.
 */
void GetController::setMultiplicityMode(int iCobo)
{
	const uint32_t threshold = getCoBoMultThres(iCobo);
	MSG_IF(verbose) << "Setting multiplicity threshold to " << threshold;
	eccClient().setMultThreshold(threshold);
	const uint32_t windowWidth = getCoBoSlidWin(iCobo);
	MSG_IF(verbose) << "Setting width of multiplicity sliding window to " << windowWidth;
	eccClient().setSlidingWin(windowWidth);
}
//__________________________________________________________________________________________________
/**
 * Checks in config. file the desired value for the multiplicity threshold.
 */
uint32_t GetController::getCoBoMultThres(int iCobo)
{
	if (isCoboActive(iCobo))
	{
		return coboConfig(iCobo)("Module")("multiplicityThreshold").getIntValue();
	}
	return 0;
}
//__________________________________________________________________________________________________
/**
 * Checks in config. file the desired value for the width of the multiplicity trigger sliding window.
 */
uint32_t GetController::getCoBoSlidWin(int iCobo)
{
	if (isCoboActive(iCobo))
	{
		return coboConfig(iCobo)("Module")("multWindowSize").getIntValue();
	}
	return 10;
}
//__________________________________________________________________________________________________
/**
 * Checks in config. file the desired value for the multiplicity threshold of the 2p mode second trigger.
 */
uint32_t GetController::getMultThres2p(int iCobo)
{
	if (isCoboActive(iCobo))
	{
		return coboConfig(iCobo)("Module")("multiplicityThreshold_2p").getIntValue();
	}
	return 0;
}
//__________________________________________________________________________________________________
/**
 * Sets multiplicity 2p mode configuration parameters.
 */
void GetController::setMultThres2p(int iCobo)
{
	const uint32_t secondThreshold = getMultThres2p(iCobo);
	MSG_IF(verbose) << "Setting multiplicity threshold to " << secondThreshold;
	eccClient().setMultThreshold2p(secondThreshold);
}
//_________________________________________________________________________________________________
/**
 * Configures the subtraction of the moving average and deviation from the multiplicity signal
 *  in the CoBo multiplicity trigger mode.
 */
void GetController::setCoBoMultSubtractMode(const size_t& coboIdx)
{
	bool subtractAverage = getCoBoMultSubtractAverage(coboIdx);
	uint8_t subtractDevs = getCoBoMultSubtractDevs(coboIdx);
	eccClient().setCoBoMultSubtractMode(subtractAverage, subtractDevs);
}
//_________________________________________________________________________________________________
/**
 * Checks in the configuration file whether to subtract the moving average from the AGET multiplicity signal.
 */
bool GetController::getCoBoMultSubtractAverage(const size_t& coboIdx)
{
	bool enabled = false;
	if (isCoboActive(coboIdx))
	{
		try
		{
			enabled = coboConfig(coboIdx)("Module")("multSubtractAverage").getBoolValue();
		}
		catch (const CCfg::CConfig::NotFound&)
		{
			// keep default value
		}
	}
	return enabled;
}

/**
 * Checks in the configuration file how many moving deviations to subtract from the AGET multiplicity signal.
 */
uint8_t GetController::getCoBoMultSubtractDevs(const size_t& coboIdx)
{
	uint8_t value = 0;
	if (isCoboActive(coboIdx))
	{
		try
		{
			value = coboConfig(coboIdx)("Module")("multSubtractDevs").getIntValue();
		}
		catch (const CCfg::CConfig::NotFound&)
		{
			// keep default value
		}
	}
	return value;
}
//_________________________________________________________________________________________________
/**
 * Configures the 4 bits of the CoBo 'pipeCtrl' register indicating which of the 4 chips to ignore in the multiplicity trigger mode.
 *
 * On Micro-TCA CoBo, the same setting applies to all AsAd boards.
 */
void GetController::setCoBoSuppressMultiplicity(const size_t& coboIdx)
{
	const uint8_t suppressMultiplicity = getCoBoSuppressMultiplicity(coboIdx);
	eccClient().setCoBoSuppressMultiplicity(suppressMultiplicity);
}
//_________________________________________________________________________________________________
/**
 * Checks in the configuration file the value of the suppressMultiplicity field.
 * Looks at CoBo[*].AsAd[*].Aget[*].Global.suppressMultiplicity
 */
uint8_t GetController::getCoBoSuppressMultiplicity(const size_t& coboIdx)
{
	// Loop over AGET positions
	uint8_t suppressMultiplicity = 0;
	for (size_t agetIdx = 0; agetIdx < 4; agetIdx++)
	{
		// Check in configuration file
		size_t suppressChipCount = 0;
		size_t activeChipCount = 0;
		for (size_t asadIdx = 0; asadIdx < 4u; ++asadIdx)
		{
			if (not isAsadActive(coboIdx, asadIdx)) continue;


			if (isAgetActive(coboIdx, asadIdx, agetIdx))
			{
				try
				{
					activeChipCount++;
					if (agetConfig(coboIdx, asadIdx, agetIdx)("Global")("suppressMultiplicity").getBoolValue())
					{
						suppressChipCount++;
					}

				}
				catch (const CCfg::CConfig::NotFound &)
				{
					// keep default value
				}
			}
		}
		// Check consistency
		bool suppressChipPosition = false;
		if (activeChipCount <= 0)
		{
			suppressChipPosition = true;
		}
		else if (suppressChipCount > 0)
		{
			if (suppressChipCount == activeChipCount)
				suppressChipPosition = true;
			else if (suppressChipCount < activeChipCount)
			{
				std::ostringstream msg;
				msg << "Cannot suppress multiplicity from AGET position " << agetIdx << " on one of CoBo '" << coboIdx
								<< "' 's AsAd board while keeping it on other AsAd boards. Check parameter 'suppressMultiplicity'";
				throw std::runtime_error(msg.str());
			}
		}
		// Suppress multiplicity
		BitFieldHelper<uint8_t>::setField(suppressMultiplicity, agetIdx, 1, suppressChipPosition ? 1 : 0);
	}
	return suppressMultiplicity;
}
//_________________________________________________________________________________________________
/**
 * Sets 2p mode in CoBo.
 */
void GetController::enableMem2pMode(const size_t& coboIdx)
{
	eccClient().enableMem2pMode(is2pModeEnabled(coboIdx));
}
//_________________________________________________________________________________________________
/**
 * Configures the value of the 'dataSource' field in the header of the data frames.
 * This Data source ID typically refers to the detector or detector type from which the frame data have been extracted.
 * FIXME: We should ensure that all boards use the same dataSource value.
 */
void GetController::setCoBoDataSourceId(const size_t& coboIdx)
{
	if (isCoboActive(coboIdx))
	{
		uint8_t dataSourceId = 0;
		try
		{
			dataSourceId = coboConfig(coboIdx)("Module")("dataSource").getIntValue();
		}
		catch (const CCfg::CConfig::NotFound & e)
		{
			std::cerr << e.what() << std::endl;
		}
		eccClient().setDataSourceId(dataSourceId);
	}
}
//_________________________________________________________________________________________________
/**
 * Configures the value of the CoBo index in the header of the data frames.
 * FIXME: We should ensure that each board uses a different ID value.
 */
void GetController::setCoBoId(const size_t& coboIdx)
{
	if (isCoboActive(coboIdx))
	{
		uint8_t coboIdentifier = 0;
		try
		{
			coboIdentifier = coboConfig(coboIdx)("Module")("coboId").getIntValue();
		}
		catch (const CCfg::CConfig::NotFound & e)
		{
			std::cerr << e.what() << std::endl;
		}
		eccClient().setCoBoId(coboIdentifier);
	}
}
//_________________________________________________________________________________________________
/**
 * Extracts automatic trigger period from the configuration file.
 * @param coboIdx Index of the CoBo board.
 * @return Period in units of 10 ns.
 */
uint32_t GetController::getTriggerPeriod_10ns(const size_t& coboIdx)
{
	uint32_t period_10ns = 100000000u; // 1s
	if (isCoboActive(coboIdx))
	{
		try
		{
			period_10ns = coboConfig(coboIdx)("Module")("triggerPeriod_10ns").getIntValue();
		}
		catch (const CCfg::CConfig::NotFound&)
		{
			period_10ns = 100000*coboConfig(coboIdx)("Module")("triggerPeriod").getIntValue(); // ms
		}
	}
	return period_10ns;
}
//_________________________________________________________________________________________________
/**
 * Sets the period of the trigger periodic mode for a CoBo target.
 * @param coboIdx Index of the CoBo board.
 */
void GetController::setTriggerPeriod(const size_t& coboIdx)
{
	eccClient().triggerPeriod_10ns(getTriggerPeriod_10ns(coboIdx));
}
//_________________________________________________________________________________________________
/**
 * Checks value of trigger delay.
 */
int64_t GetController::getTriggerDelay_10ns(const size_t& iCobo)
{
	if (isCoboActive(iCobo))
	{
		return coboConfig(iCobo)("Module")("triggerDelay").getIntValue();
	}
	return 800;
}
//_________________________________________________________________________________________________
/**
 * Sets value of delay between trigger event and actual freeze of the AGET memory.
 */
void GetController::setTriggerDelay_10ns(const size_t& coboIdx)
{
	eccClient().setTriggerDelay_10ns(getTriggerDelay_10ns(coboIdx));
}
//_________________________________________________________________________________________________
/**
 * recupere la valeur de retard du trigger
 */
int64_t GetController::getTriggerDeadTime2p_10ns(int iCobo)
{
	if (isCoboActive(iCobo))
	{
		return coboConfig(iCobo)("Module")("triggerDeadTime_2p").getIntValue();
	}
	return 800;
}
//_________________________________________________________________________________________________
/**
 * programme le retard du trigger
 */
void GetController::setTriggerDeadTime2p_10ns(int iCobo)
{
	eccClient().setTriggerDeadTime2p_10ns(getTriggerDeadTime2p_10ns(iCobo));
}
//_________________________________________________________________________________________________
/**
 * Checks value of 2p mode second trigger delay.
 */
int64_t GetController::getTriggerDelay2p_10ns(const size_t& coboIdx)
{
	if (isCoboActive(coboIdx))
	{
		return coboConfig(coboIdx)("Module")("triggerDelay_2p").getIntValue();
	}
	return 800;
}
//_________________________________________________________________________________________________
/**
 * Sets value of delay between 2nd trigger event and actual freeze of the AGET memory.
 */
void GetController::setTriggerDelay2p_10ns(const size_t& coboIdx)
{
	eccClient().setTriggerDelay2p_10ns(getTriggerDelay2p_10ns(coboIdx));
}
/**
 * recupere la valeur de retard du trigger
 */
int64_t GetController::getTriggerTime2p_10ns(int iCobo)
{
	if (isCoboActive(iCobo))
	{
		return coboConfig(iCobo)("Module")("triggerTime_2p").getIntValue();
	}
	return 800;
}
//_________________________________________________________________________________________________
/**
 * programme le retard du trigger
 */
void GetController::setTriggerTime2p_10ns(int iCobo)
{
	eccClient().setTriggerTime2p_10ns(getTriggerTime2p_10ns(iCobo));
}
//_________________________________________________________________________________________________
/**
 * recupere la valeur de retard du trigger
 */
int64_t GetController::getTriggerTimeOut2p_10ns(int iCobo)
{
	if (isCoboActive(iCobo))
	{
		return coboConfig(iCobo)("Module")("triggerTimeOut_2p").getIntValue();
	}
	return 800;
}
//_________________________________________________________________________________________________
/**
 * programme le retard du trigger
 */
void GetController::setTriggerTimeOut2p_10ns(int iCobo)
{
	eccClient().setTriggerTimeOut2p_10ns(getTriggerTimeOut2p_10ns(iCobo));
}
//__________________________________________________________________________________________________
/**
 * Extracts from the configuration file the parameters of the circular buffer.
 */
uint32_t GetController::getCircularBufferParameter(const std::string& param, const size_t& coboIdx)
{
	if (isCoboActive(coboIdx))
	{
		return coboConfig(coboIdx)("CircularBuffer")(param).getHexValue();
	}
	return 0;
}
//__________________________________________________________________________________________________
/**
 * Writes into the CoBo board the parameters of the circular buffer.
 */
void GetController::setCircularBuffers(const size_t& coboIdx)
{
	// Set parameters shared by all 4 buffers
	uint32_t eventsPerInterrupt = 1u;
	try
	{
		eventsPerInterrupt = getCircularBufferParameter("interruptRate", coboIdx);
	}
	catch (const CCfg::CConfig::NotFound &) {}
	const uint32_t levelAlmostFullMemory = getCircularBufferParameter("levelAlmostFullMemory", coboIdx);
	const uint32_t levelAlmostEmptyMemory = getCircularBufferParameter("levelAlmostEmptyMemory", coboIdx);
	eccClient().setCircularBufferCommonParameters(eventsPerInterrupt, levelAlmostFullMemory, levelAlmostEmptyMemory);

	// Try to customize the size of each buffer
	try
	{
		for (size_t asadIdx=0; asadIdx <4u; ++asadIdx)
		{
			const uint32_t bufferStart = asadConfig(coboIdx, asadIdx)("CircularBuffer")("startOfMemory").getHexValue();
			const uint32_t bufferEnd = asadConfig(coboIdx, asadIdx)("CircularBuffer")("endOfMemory").getHexValue();
			eccClient().setAsAdCircularBuffer(asadIdx, bufferStart, bufferEnd);
		}
	}
	// Divide memory in equal parts
	catch (const CCfg::CConfig::NotFound &)
	{
		eccClient().setCircularBuffers(
			getCircularBufferParameter("startOfMemory", coboIdx),
			getCircularBufferParameter("endOfMemory", coboIdx));
	}
}
//_________________________________________________________________________________________________
/**
 * Parses the configuration to extract parameters for setting the AsAd inspection manager.
 */
bool GetController::getInspectionLinesParameters(std::string& moduleName,
		std::string& signalNameISP1, std::string& signalNameISP2, int iCobo,
		int iAsad)
{
	try
	{
		if (isAsadActive(iCobo, iAsad))
		{
			moduleName = asadConfig(iCobo, iAsad)("InspectionLines")("deviceToInspect").getStringValue();
			signalNameISP1 = asadConfig(iCobo, iAsad)("InspectionLines")(moduleName)("ISP1").getStringValue();
			signalNameISP2 = asadConfig(iCobo, iAsad)("InspectionLines")(moduleName)("ISP2").getStringValue();
			return true;
		}
	}
	catch (const CCfg::CConfig::NotFound & e)
	{
		std::cerr << e.what() << std::endl;
	}

	moduleName = "ADC";
	signalNameISP1 = "SPI_CS";
	signalNameISP2 = "CAS";
	return false;
}
//_________________________________________________________________________________________________
/**
 * Extracts the trigger mode from the configuration file.
 */
std::string GetController::getTriggerMode(int iCobo)
{
	std::string mode = "periodically";
	if (isCoboActive(iCobo))
	{

		mode = coboConfig(iCobo)("Module")("triggerMode").getStringValue();
		MSG_IF(verbose) << "triggerMode: " << mode;
	}
	return mode;
}
//_________________________________________________________________________________________________
/**
 * Extracts the secondary trigger mode from the configuration file.
 */
std::string GetController::getSecondaryTriggerMode(int iCobo)
{
	std::string mode = "noTrigger";
	if (isCoboActive(iCobo))
	{
		try
		{
			try
			{
				mode = coboConfig(iCobo)("Module")("secondaryTriggerMode").getStringValue();
			}
			catch (const CCfg::CConfig::NotFound &)
			{
				mode = coboConfig(iCobo)("Module")("externalTriggerMode").getStringValue();
			}
		}
		catch (const CCfg::CConfig::NotFound &)
		{
			mode = getTriggerMode(iCobo);
			std::cerr << "Using same mode for primary and secondary trigger logics." << std::endl;
		}
		MSG_IF(verbose) << "secondaryTriggerMode: " << mode;
	}
	return mode;
}
//_________________________________________________________________________________________________
/**
 * Sets CoBo trigger modes.
 */
void GetController::setTriggerModes(int iCobo)
{
	eccClient().setTriggerMode(getTriggerMode(iCobo));
	eccClient().setSecondaryTriggerMode(getSecondaryTriggerMode(iCobo));
}
//_________________________________________________________________________________________________
/*
 * Extracts the value of the ScwMultDelay register from the configuration file.
 */
int GetController::getScwMultDelay(const int coboIdx)
{
	uint64_t delay = 200;
	if (isCoboActive(coboIdx))
	{
		try
		{
			delay = coboConfig(coboIdx)("Module")("scwMultDelay").getIntValue();
		}
		catch (const CCfg::CConfig::NotFound&)
		{ /* keep default value*/
		}
	}
	return delay;
}
//_________________________________________________________________________________________________
/**
 * Sets the delay from rising edge of SCW signal to start of multiplicity sliding window.
 */
void GetController::setScwMultDelay(const int coboIdx)
{
	eccClient().setScwMultDelay(getScwMultDelay(coboIdx));
}
//_________________________________________________________________________________________________
/*
 * Extracts the value of the ScwScrDelay register from the configuration file.
 */
int GetController::getScwScrDelay(const int coboIdx)
{
	uint64_t delay_10ns = 0;
	if (isCoboActive(coboIdx))
	{
		try
		{
			delay_10ns = coboConfig(coboIdx)("Module")("scwScrDelay").getIntValue();
		}
		catch (const CCfg::CConfig::NotFound&)
		{
			try
			{
				delay_10ns = coboConfig(coboIdx)("Module")("writeReadDelay").getIntValue();
			}
			catch (const CCfg::CConfig::NotFound&)
			{
				// Keep default value
			}
		}
	}
	MSG_IF(verbose) << "ScwScrDelay: " << delay_10ns;
	return delay_10ns;
}
//_________________________________________________________________________________________________
/**
 * Sets the delay between SCW low and SCR high.
 */
void GetController::setScwScrDelay(const int coboIdx)
{
	eccClient().setScwScrDelay(getScwScrDelay(coboIdx));
}
//_________________________________________________________________________________________________
/**
 * Sets the CoBo in partial/full readout mode.
 * All AGET chips within a CoBo should be configured accordingly (see AGET register 1 bit 14).
 */
void GetController::setReadOutMode(const size_t& coboIdx)
{
	eccClient().setFullReadOutMode(isFullReadoutModeEnabled(coboIdx));
}
//__________________________________________________________________________________________________
/**
 * Checks in config. whether CoBo should be in partial or full readout mode.
 */
bool GetController::isFullReadoutModeEnabled(const size_t& coboIdx)
{
	// Check readout mode of all chips
	size_t partialReadoutCount = 0;
	size_t activeChipCount = 0;
	for (size_t asadIdx = 0; asadIdx < 4; ++asadIdx)
	{
		for (size_t agetIdx = 0; agetIdx < 4; ++agetIdx)
		{
			if (not isAgetActive(coboIdx, asadIdx, agetIdx)) continue;

			activeChipCount++;
			if (not agetConfig(coboIdx, asadIdx, agetIdx)("Global")("Reg1")("isAllChannelRead").getBoolValue())
				partialReadoutCount++;
		}
	}

	// Ensure all chips are in the same readout mode
	bool fullReadoutModeEnabled = true;
	if (partialReadoutCount > 0)
	{
		if (partialReadoutCount == activeChipCount)
		{
			fullReadoutModeEnabled = false;
		}
		else
		{
			std::ostringstream msg;
			msg << "All AGET chips of CoBo board '" << coboIdx
				<< "' should be in the same (full or partial) readout mode! Check the 'isAllChannelRead' parameters.";
			throw std::runtime_error(msg.str());
		}
	}
	return fullReadoutModeEnabled;
}
//__________________________________________________________________________________________________
bool GetController::isFPNRead(int iCobo, int iAsad, int iAget)
{
	if (isAgetActive(iCobo, iAsad, iAget))
	{
		return agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("isFPNRead").getBoolValue();
	}
	return true;
}
//_________________________________________________________________________________________________
/**
 * Sets the readout depth (number of time buckets).
 *
 * If the corresponding parameter is not found, the number of buckets to read is set to 512 or 256 instead.
 */
void GetController::setReadOutDepth(const size_t& coboIdx)
{
	const bool is2pEnabled = is2pModeEnabled(coboIdx);
	uint16_t readoutDepth = is2pEnabled ? 256 : 512 ;
	try
	{
		readoutDepth = coboConfig(coboIdx)("Module")("readoutDepth").getIntValue();
	}
	catch (const CCfg::CConfig::NotFound & e)
	{
		std::cerr << e.what() << " Using a readout depth of " << readoutDepth << " bucket(s) instead." << std::endl;
	}
	if (is2pEnabled and readoutDepth > 256)
	{
		std::cerr << readoutDepth << " is not a valid readout depth in 2p mode. Using 256 bucket(s) instead." << std::endl;
		readoutDepth = 256;
	}
	else if (not is2pEnabled and readoutDepth > 512)
	{
		std::cerr << readoutDepth << " is not a valid readout depth. Using 512 bucket(s) instead." << std::endl;
		readoutDepth = 512;
	}
	std::cerr << "Setting readout depth to " << readoutDepth << " time bucket(s)" << std::endl;
	eccClient().setCoBoReadDepth(readoutDepth);
}
//_________________________________________________________________________________________________
/**
 * Sets the readout offset to indicate in the header of data frames.
 *
 * Sets the CoBo 'readDepth' register to the value given to the register 12 of the first active AGET chip.
 */
void GetController::setReadOutIndex(const size_t& coboIdx)
{
	size_t agetIdx = eccClient().firstActiveChip();
	const size_t asadIdx = agetIdx/4;
	agetIdx = agetIdx%4;
	uint16_t readoutOffset = agetConfig(coboIdx, asadIdx, agetIdx)("Global")("ReadoutPointerOffset").getIntValue();
	std::cerr << "Setting readout offset to " << readoutOffset << " time bucket(s)" << std::endl;
	eccClient().setCoBoReadOffset(readoutOffset);
}
//_________________________________________________________________________________________________
/**
 * Enables or disables zero suppression and sets its polarity.
 *
 * If the corresponding parameter is not found, zero suppression is disabled.
 */
void GetController::setZeroSuppressionEnabled(const size_t& coboIdx)
{
	bool enableSuppression = false;
	bool zeroSuppressInvert = false;
	try
	{
		enableSuppression = coboConfig(coboIdx)("Module")("enableZeroSuppression").getBoolValue();
		zeroSuppressInvert  = coboConfig(coboIdx)("Module")("zeroSuppressionInverted").getBoolValue();
	}
	catch (const CCfg::CConfig::NotFound & e)
	{
		std::cerr << e.what() << std::endl;
	}
	std::cerr << (enableSuppression?"Enabling":"Disabling") << " zero suppression" << std::endl;
	if (enableSuppression and zeroSuppressInvert)
	{
		std::cerr << "CoBo will suppress sample values ABOVE zero suppression thresholds." << std::endl;
	}
	eccClient().setCoBoZeroSuppressionMode(enableSuppression, zeroSuppressInvert);
}
//__________________________________________________________________________________________________
/**
 * Configures AsAd periodic pulser.
 */
void GetController::configureAsAdPeriodicPulser(const size_t& coboIdx)
{
	if (not isAsAdPeriodicPulserEnabled(coboIdx)) return;

	// Get amplitude
	const int32_t amplitude_mV = getAsadPulserVoltage(coboIdx);
	// Get period
	size_t period_ms = 1000u;
	try
	{
		period_ms = coboConfig(coboIdx)("Generator")("period_ms").getIntValue();
	}
	catch (CCfg::CConfig::NotFound & e)
	{
		std::cerr << e.what() << std::endl;
	}
	// Get amplitude of second pulse
	bool doublePulseEnabled = false;
	int32_t secondPulseAmplitude_mV = 0;
	try
	{
		doublePulseEnabled = coboConfig(coboIdx)("Generator")("doublePulseEnabled").getBoolValue();
		secondPulseAmplitude_mV = coboConfig(coboIdx)("Generator")("secondPulseAmplitude").getRealValue();
	}
	catch (CCfg::CConfig::NotFound & e)
	{
		std::cerr << e.what() << std::endl;
	}

	// Set amplitude and period
	std::cerr << "AsAd pulser will be triggered every " << period_ms << " ms with a voltage of Vref + " << amplitude_mV << " mV" << std::endl;
	eccClient().ecc()->configureAsAdPeriodicPulser(amplitude_mV, period_ms);

	// Set second pulse
	if (doublePulseEnabled)
	{
		std::cerr << "AsAd pulser will send a second pulse with a voltage of Vref + " << secondPulseAmplitude_mV << " mV" << std::endl;
	}
	try
	{
		eccClient().ecc()->configureAsAdDoublePulserMode(doublePulseEnabled?secondPulseAmplitude_mV:0);
	}
	catch (const mdaq::utl::CmdException & e)
	{
		if (doublePulseEnabled)
		{
			throw;
		}
	}

	// Check whether the pulser should be periodic or Poisson
	bool randomMode = false;
	try
	{
		randomMode = coboConfig(coboIdx)("Generator")("poissonModeEnabled").getBoolValue();
	}
	catch (CCfg::CConfig::NotFound & e)
	{
		std::cerr << e.what() << std::endl;
	}
	try
	{
		eccClient().ecc()->setRandomAsAdPulserEnabled(randomMode);
	}
	catch (const mdaq::utl::CmdException & e)
	{
		if (randomMode)
		{
			throw;
		}
	}
}
//__________________________________________________________________________________________________
/**
 * Checks in the configuration whether to trigger the AsAd pulser periodically.
 */
bool GetController::isAsAdPeriodicPulserEnabled(const size_t& coboIdx)
{
	bool enabled = false;

	try
	{
		enabled = coboConfig(coboIdx)("Generator")("periodicModeEnabled").getBoolValue();
	}
	catch (const CCfg::CConfig::NotFound & e)
	{
		std::cerr << e.what() << std::endl;
	}

	return enabled;
}
//__________________________________________________________________________________________________
/**
 * Writes and reads an AGET register through CoBo and AsAd.
 * @param registerName Name of the AGET register.
 * @param coboIdx Index of the CoBo board.
 * @param asadIdx Index of the AsAd board.
 * @param agetIdx Index of the AGET within its AsAd container.
 * @param wchk Whether to check the register has been set to the expected value.
 * @param value Value to set the register to.
 */
bool GetController::setAget_reg(const std::string& registerName, const std::size_t& /* coboIdx */, const size_t& asadIdx, const size_t& agetIdx, bool wchk, const uint64_t& value)
{
	std::ostringstream oss;
	oss << "Setting register '" << registerName << "' of AGET no. " << std::dec << 4*asadIdx+agetIdx  << " to " << std::showbase << std::hex << value;
	std::cerr << oss.str() << std::endl;

	eccClient().agetRegWSC(asadIdx, agetIdx, registerName, value);

	if (wchk)
	{
		uint64_t resp = eccClient().agetRegRSC(asadIdx, agetIdx, registerName);
		if (value != resp)
		{
			oss.str("");
			oss << "Check of slow control register '" << registerName << "' failed for AGET no. " << std::dec << 4*asadIdx+agetIdx
					<< " (" << std::showbase << std::hex << resp << " instead of " << value << std::dec << ")";
			// Handle special case of bit 21 of register 2 failure
			if ("reg2" == registerName)
			{
				uint64_t regXor =  value ^ resp;
				if (regXor == UINT64_C(0x200000))
				{
					std::cerr << oss.str() << std::endl;
					return true;
				}
			}
			std::cerr << oss.str() << std::endl;
			return false;
		}
	}
	return true;
}
//__________________________________________________________________________________________________
/**
 * Sets an AGET register.
 * @param registerName Name of the AGET register.
 * @param coboIdx Index of the CoBo board.
 * @param asadIdx Index of the AsAd board.
 * @param agetIdx Index of the AGET within its AsAd container.
 * @param wchk Whether to check the register has been set to the expected value.
 */
bool GetController::setAget_reg(const std::string& registerName, const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk)
{
	uint64_t value;
	if (registerName == "reg1")
		value = retrievalAget_reg1(coboIdx, asadIdx, agetIdx);
	else if (registerName == "reg2")
		value = retrievalAget_reg2(coboIdx, asadIdx, agetIdx);
	else if (registerName == "reg12")
		value = retrievalAget_reg12(coboIdx, asadIdx, agetIdx);
	else
		value = retrievalChan_reg(registerName, coboIdx, asadIdx, agetIdx);

	return setAget_reg(registerName, coboIdx, asadIdx, agetIdx, wchk, value);
}
//__________________________________________________________________________________________________
/**
 * Writes and reads AGET register 1 through CoBo and AsAd.
 * @param coboIdx Index of the CoBo board.
 * @param asadIdx Index of the AsAd board.
 * @param agetIdx Index of the AGET within its AsAd container.
 * @param wchk Whether to check the register has been set to the expected value.
 */
bool GetController::setAget_reg1(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk)
{
	// Get desired value
	const uint64_t value = retrievalAget_reg1(coboIdx, asadIdx, agetIdx);

	std::ostringstream oss;
	oss << "Setting register 'reg1' of AGET no. " << std::dec << 4*asadIdx + agetIdx  << " to " << std::showbase << std::hex << value;
	std::cerr << oss.str() << std::endl;

	// Set register
	eccClient().agetRegWSC(asadIdx, agetIdx, "reg1", value);
	// Check register
	if (wchk)
	{
		const uint64_t resp = eccClient().agetRegRSC(asadIdx, agetIdx, "reg1");
		if (value != resp)
		{
			oss.str("");
			oss << "Check of slow control register 'reg1' failed for AGET no. " << std::dec << 4*asadIdx + agetIdx
					<< " (" << std::showbase << std::hex << resp << " instead of " << value << std::dec << ")\n";
			std::cerr << oss.str() << std::endl;
			return false;
		}
	}
	return true;
}
//__________________________________________________________________________________________________
/**
 * Writes and reads AGET register 2 through CoBo and AsAd.
 * @param coboIdx Index of the CoBo board.
 * @param asadIdx Index of the AsAd board.
 * @param agetIdx Index of the AGET within its AsAd container.
 * @param wchk Whether to check the register has been set to the expected value.
 */
bool GetController::setAget_reg2(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk)
{
	return setAget_reg("reg2", coboIdx, asadIdx, agetIdx, wchk);
}
//__________________________________________________________________________________________________
/**
 * Writes and reads AGET registers 3 and 4 through CoBo and AsAd.
 * @param coboIdx Index of the CoBo board.
 * @param asadIdx Index of the AsAd board.
 * @param agetIdx Index of the AGET within its AsAd container.
 * @param wchk Whether to check the register has been set to the expected value.
 */
bool GetController::setAget_reg34(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk)
{
	bool success = setAget_reg("reg3", coboIdx, asadIdx, agetIdx, wchk)
		and setAget_reg("reg4", coboIdx, asadIdx, agetIdx, wchk);
	return success;
}
//__________________________________________________________________________________________________
/**
 * Writes and reads AGET registers 6 and 7 through CoBo and AsAd.
 * @param coboIdx Index of the CoBo board.
 * @param asadIdx Index of the AsAd board.
 * @param agetIdx Index of the AGET within its AsAd container.
 * @param wchk Whether to check the register has been set to the expected value.
 */
bool GetController::setAget_reg67(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk)
{
	bool success = setAget_reg("reg6", coboIdx, asadIdx, agetIdx, wchk)
	 and setAget_reg("reg7", coboIdx, asadIdx, agetIdx, wchk);
	return success;
}
//__________________________________________________________________________________________________
/**
 * Writes and reads AGET registers 8 and 9 through CoBo and AsAd.
 * @param coboIdx Index of the CoBo board.
 * @param asadIdx Index of the AsAd board.
 * @param agetIdx Index of the AGET within its AsAd container.
 * @param wchk Whether to check the register has been set to the expected value.
 */
bool GetController::setAget_reg89(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk)
{
	bool success = setAget_reg("reg8", coboIdx, asadIdx, agetIdx, wchk)
		and setAget_reg("reg8msb", coboIdx, asadIdx, agetIdx, wchk)
		and setAget_reg("reg9", coboIdx, asadIdx, agetIdx, wchk)
		and setAget_reg("reg9msb", coboIdx, asadIdx, agetIdx, wchk);
	return success;
}
//__________________________________________________________________________________________________
/**
 * Writes and reads AGET registers 10 and 11 through CoBo and AsAd.
 * @param coboIdx Index of the CoBo board.
 * @param asadIdx Index of the AsAd board.
 * @param agetIdx Index of the AGET within its AsAd container.
 * @param wchk Whether to check the register has been set to the expected value.
 */
bool GetController::setAget_reg1011(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk)
{
	bool success = setAget_reg("reg10", coboIdx, asadIdx, agetIdx, wchk)
			and setAget_reg("reg11", coboIdx, asadIdx, agetIdx, wchk);
	return success;
}
//__________________________________________________________________________________________________
/**
 * Sets AGET register 12 and CoBo 'readDepth' register.
 * @param registerName Name of the AGET register.
 * @param coboIdx Index of the CoBo board.
 * @param asadIdx Index of the AsAd board.
 * @param agetIdx Index of the AGET within its AsAd container.
 * @param wchk Whether to check the register has been set to the expected value.
 */
bool GetController::setAget_reg12(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx,
		bool wchk)
{
	const uint16_t readoutPointerOffset = retrievalAget_reg12(coboIdx, asadIdx, agetIdx);

	// Set AGET register 12
	// This 16 bits register permits to reduce the number of SCA memory cells during the readout phase
	// by adding an offset (9 bits) on the readout pointer address.
	bool status = setAget_reg("reg12", coboIdx, asadIdx, agetIdx, wchk);

	// Set CoBo readDepth register (10 bits): number of time buckets to read
	const uint16_t depth = is2pModeEnabled(coboIdx) ? 256 - readoutPointerOffset : 512 - readoutPointerOffset;
	eccClient().setCoBoReadDepth(depth);
	return status;
}
//_________________________________________________________________________________________________
/**
 * Analyzes the parameters in the configuration file and extracts values for the AGET 'reg1' register.
 */
uint64_t GetController::retrievalAget_reg1(int iCobo, int iAsad, int iAget)
{
	uint64_t regValue = UINT64_C(0);

	if (isAgetActive(iCobo, iAsad, iAget))
	{
		MSG_IF(verbose) << "AGET selected  for 'reg1': " << agetConfig(iCobo, iAsad, iAget).getIndex();
		uint64_t fieldValue;

		// "Icsa". If set, the nominal CSA bias current is multiplied by two.
		const bool isIcsaSet = agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("isIcsax2").getBoolValue();
		MSG_IF(verbose) << "isIcsax2: " << isIcsaSet;
		fieldValue = isIcsaSet?0x1:0x0;
		BitFieldHelper<uint64_t>::setField(regValue, 0, 1, fieldValue);

		 // "Gain". LSb and MSb of the internal test capacitor in Test mode.
		const std::string testCapacitorValue = agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("TestModeRange").getStringValue();
		MSG_IF(verbose) << "Internal test capacitor ('TestModeRange'): " << testCapacitorValue;
		// We expect Farad sub-units or, in old config files, Coulomb sub-units
		if (ba::starts_with(testCapacitorValue, "120f") or ba::starts_with(testCapacitorValue, "120 f"))
			fieldValue = UINT64_C(0x0);
		else if (ba::starts_with(testCapacitorValue, "240f") or ba::starts_with(testCapacitorValue, "240 f"))
			fieldValue = UINT64_C(0x1);
		else if (ba::starts_with(testCapacitorValue, "1p") or ba::starts_with(testCapacitorValue, "1 p") )
			fieldValue = UINT64_C(0x2);
		else if (ba::starts_with(testCapacitorValue, "10p") or ba::starts_with(testCapacitorValue, "10 p"))
			fieldValue = UINT64_C(0x3);
		else
		{
			std::cerr << "Unexpected value for internal test capacitor: " << testCapacitorValue << ". Using 120 fF instead." << std::endl;
			fieldValue = UINT64_C(0x0);
		}
		BitFieldHelper<uint64_t>::setField(regValue, 1, 2, fieldValue);

		// "Time". Sets the peaking time (ns) of the shaper by switching resistors on the PZC  SK filters.
		const std::string peakingTime = agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("peackingTime").getValueAsString();
		MSG_IF(verbose) << "Peaking time: " << peakingTime << " ns";
		if (peakingTime == "70")
			fieldValue = 0x0;
		else if (peakingTime == "117")
			fieldValue = 0x1;
		else if (peakingTime == "232")
			fieldValue = 0x2;
		else if (peakingTime == "280")
			fieldValue = 0x3;
		else if (peakingTime == "334")
			fieldValue = 0x4;
		else if (peakingTime == "383")
			fieldValue = 0x5;
		else if (peakingTime == "502")
			fieldValue = 0x6;
		else if (peakingTime == "541")
			fieldValue = 0x7;
		else if (peakingTime == "568")
			fieldValue = 0x8;
		else if (peakingTime == "632")
			fieldValue = 0x9;
		else if (peakingTime == "721")
			fieldValue = 0xa;
		else if (peakingTime == "760")
			fieldValue = 0xb;
		else if (peakingTime == "831")
			fieldValue = 0xc;
		else if (peakingTime == "870")
			fieldValue = 0xd;
		else if (peakingTime == "976")
			fieldValue = 0xe;
		else if (peakingTime == "1014")
			fieldValue = 0xf;
		else
		{
			std::cerr << "Unexpected peaking time value: " << peakingTime << "ns. Using 568 ns instead." << std::endl;
			fieldValue = 0x8;
		}
		BitFieldHelper<uint64_t>::setField(regValue, 3, 4, fieldValue);

		// "Test". Defines the test mode: none (0), calibration (01), test (10), functionality (11).
		const std::string testMode = agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("TestModeSelection").getStringValue();
		MSG_IF(verbose) << "TestModeSelection: " << testMode;
		if (testMode == "nothing")
			fieldValue = 0x0;
		else if (testMode == "calibration")
			fieldValue = 0x1;
		else if (testMode == "test")
			fieldValue = 0x2;
		else if (testMode == "functionality")
			fieldValue = 0x3;
		else
			fieldValue = 0x0;
		BitFieldHelper<uint64_t>::setField(regValue, 7, 2, fieldValue);

		// "Integrator_mode". Bit integrator mode
		const std::string integratorMode = agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("IntegrationMode").getStringValue();
		MSG_IF(verbose) << "IntegrationMode: " << integratorMode;
		if (integratorMode == "variable")
			fieldValue = 0x0;
		else
			fieldValue = 0x1;
		BitFieldHelper<uint64_t>::setField(regValue, 9, 1, fieldValue);

		// "SCApointer". Bit SCA pointer 0 to pointer 1.
		const std::string scaPointer = agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("SCA_Pointer").getStringValue();
		MSG_IF(verbose) << "SCA_Pointer: " << scaPointer;
		if (scaPointer == "anySCAPointer")
			fieldValue = 0x3;
		else if (scaPointer == "col_0or128or256or384")
			fieldValue = 0x2;
		else if (scaPointer == "col_0or256")
			fieldValue = 0x1;
		else if (scaPointer == "column_0")
			fieldValue = 0x0;
		else
			fieldValue = 0x0;
		BitFieldHelper<uint64_t>::setField(regValue, 10, 2, fieldValue);

		// "SCAsplitting". Bit SCA splitting; if set, enables the 2p mode by dividing the memory in two blocks.
		if (agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("SCA_Splitting").getBoolValue())
			fieldValue = 0x1;
		else
			fieldValue = 0x0;
		BitFieldHelper<uint64_t>::setField(regValue, 12, 1, fieldValue);
		MSG_IF(verbose) << "SCA_Splitting: " << fieldValue;

		// "Mode32Channels". Bit 32 channels mode. If set, only the first 32 channels are used.
		if (agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("is32channels").getBoolValue())
			fieldValue = 0x1;
		else
			fieldValue = 0x0;
		BitFieldHelper<uint64_t>::setField(regValue, 13, 1, fieldValue);
		MSG_IF(verbose) << "is32channels: " << fieldValue;

		// "Readout_mode". Bit readout mode; switches between full and partial readout modes. If set, full readout.
		if (agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("isAllChannelRead").getBoolValue())
			fieldValue = 0x1;
		else
			fieldValue = 0x0;
		BitFieldHelper<uint64_t>::setField(regValue, 14, 1, fieldValue);
		MSG_IF(verbose) << "isAllChannelRead: " << fieldValue;

		// "FPNreadout". Bit FPN readout: if set, forces the readout of the FPN channels (e.g. even in partial readout mode).
		if (agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("isFPNRead").getBoolValue())
			fieldValue = 0x1;
		else
			fieldValue = 0x0;
		BitFieldHelper<uint64_t>::setField(regValue, 15, 1, fieldValue);
		MSG_IF(verbose) << "isFPNRead: " << fieldValue;

		// "Polarity". Bit polarity. Controls the value of the DC voltage levels. Usually set to zero?
		if (agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("isPositivePolarity").getBoolValue())
			fieldValue = 0x1;
		else
			fieldValue = 0x0;
		BitFieldHelper<uint64_t>::setField(regValue, 16, 1, fieldValue);
		MSG_IF(verbose) << "isPositivePolarity: " << fieldValue;

		// "Vicm". Input common mode voltage (VICM) of the analog output buffer. Usually 0 e.g. 1.25 V?
		const std::string vicmValue = agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("Vicm").getStringValue();
		if (vicmValue == "1.25V")
			fieldValue = 0x0;
		else if (vicmValue == "1.35V")
			fieldValue = 0x1;
		else if (vicmValue == "1.55V")
			fieldValue = 0x2;
		else if (vicmValue == "1.65V")
			fieldValue = 0x3;
		else
			fieldValue = 0x0;
		BitFieldHelper<uint64_t>::setField(regValue, 17, 2, fieldValue);
		MSG_IF(verbose) << "Vicm: " << vicmValue;

		// "DACthreshold".
		MSG_IF(verbose) << "GlobalThresholdValue";
		BitFieldHelper<uint64_t>::setField(regValue, 19, 3, agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")(
				"GlobalThresholdValue").getIntValue());

		// "DACpolarity".
		MSG_IF(verbose) << "isThresholdSignedPositive";
		fieldValue = agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("isThresholdSignedPositive")
				.getBoolValue()?0x1:0x0;
		BitFieldHelper<uint64_t>::setField(regValue, 22, 1, fieldValue);

		// "Trigger_veto". Trigger bit: specifies the use of the veto on the trigger building. Defines how long after a hit the channel can be read again.
		// Values: 0 (none), 01 ( 4 microseconds), 10 (hit register width).
		const std::string triggerVetoOption = agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")(
				"TriggerVetoOption").getStringValue();
		if (triggerVetoOption == "none")
			fieldValue = 0x0;
		else if (triggerVetoOption == "4us")
			fieldValue = 0x1;
		else if (triggerVetoOption == "HitRegisterWidth")
			fieldValue = 0x2;
		else if (triggerVetoOption == "undefined")
			fieldValue = 0x3;
		else
			fieldValue = 0x0;
		MSG_IF(verbose) << "TriggerVetoOption: " << triggerVetoOption;
		BitFieldHelper<uint64_t>::setField(regValue, 23, 2, fieldValue);

		// "Synchro_discri".
		fieldValue = agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("isckwriteSynchro")
				.getBoolValue()?0x1:0x0;
		MSG_IF(verbose) << "isckwriteSynchro: " << fieldValue;
		BitFieldHelper<uint64_t>::setField(regValue, 25, 1, fieldValue);

		// "tot".
		fieldValue = agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("isTOTActive").
				getBoolValue()? 0x1:0x0;
		BitFieldHelper<uint64_t>::setField(regValue, 26, 1, fieldValue);
		MSG_IF(verbose) << "isTOTActive: " << fieldValue;

		// "Range_trigg_width"
		fieldValue = agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("isTriggerWidth200ns")
				.getBoolValue()?0x1:0x0;
		BitFieldHelper<uint64_t>::setField(regValue, 27, 1, fieldValue);
		MSG_IF(verbose) << "isTriggerWidth200ns: " << fieldValue;

		const std::string triggerWidthRange = agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")(
				"TriggerWidthRange").getStringValue();
		MSG_IF(verbose) << "TriggerWidthRange: " << triggerWidthRange;
		if (triggerWidthRange == "width1")
		{
			// "Lsb_trigg_width"
			BitFieldHelper<uint64_t>::setField(regValue, 28, 1, 0x1);
			// "Msb_trigg_width"
			BitFieldHelper<uint64_t>::setField(regValue, 29, 1, 0x0);
		}
		else if (triggerWidthRange == "width2")
		{
			// "Lsb_trigg_width"
			BitFieldHelper<uint64_t>::setField(regValue, 28, 1, 0x0);
			// "Msb_trigg_width"
			BitFieldHelper<uint64_t>::setField(regValue, 29, 1, 0x1);
		}
		else if (triggerWidthRange == "width3")
		{
			// "Lsb_trigg_width"
			BitFieldHelper<uint64_t>::setField(regValue, 28, 1, 0x1);
			// "Msb_trigg_width"
			BitFieldHelper<uint64_t>::setField(regValue, 29, 1, 0x1);
		}
		else // "width0"
		{
			// "Lsb_trigg_width"
			BitFieldHelper<uint64_t>::setField(regValue, 28, 1, 0x0);
			// "Msb_trigg_width"
			BitFieldHelper<uint64_t>::setField(regValue, 29, 1, 0x0);
		}

		// "External".
		const std::string externalLink = agetConfig(iCobo, iAsad, iAget)("Global")("Reg1")("ExternalLink").getStringValue();
		MSG_IF(verbose) << "ExternalLink: " << externalLink;
		if (externalLink == "none")
			fieldValue = 0x0;
		else if (externalLink == "SKFilterInput")
			fieldValue = 0x1;
		else if (externalLink == "Gain-2Input")
			fieldValue = 0x2;
		else if (externalLink == "CSAStandby")
			fieldValue = 0x3;
		else
			fieldValue = 0x0;
		BitFieldHelper<uint64_t>::setField(regValue, 30, 2, fieldValue);
	}
	return regValue;
}
/**
 * analyse le fichier de parametres et recupere les valeurs correspndants
 * au registre 2 de aget
 */
uint64_t GetController::retrievalAget_reg2(int iCobo, int iAsad, int iAget)
{
	uint64_t datareg = 0ull;
	if (isAgetActive(iCobo, iAsad, iAget))
	{
		MSG_IF(verbose) << "AGET selected  for 'reg2': " << agetConfig(iCobo,
					iAsad, iAget).getIndex();

		MSG_IF(verbose) << "DebugSelection";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")("DebugSelection").getStringValue()
				== "standby"))
			datareg = datareg + (0x0);
		else if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"DebugSelection").getStringValue() == "CSA"))
			datareg = datareg + (0x1);
		else if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"DebugSelection").getStringValue() == "CR"))
			datareg = datareg + (0x2);
		else if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"DebugSelection").getStringValue() == "Gain-2"))
			datareg = datareg + (0x3);
		else if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"DebugSelection").getStringValue() == "PositiveInputDiscri"))
			datareg = datareg + (0x4);
		else if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"DebugSelection").getStringValue() == "NegativeInputDiscri"))
			datareg = datareg + (0x5);
		else if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"DebugSelection").getStringValue() == "trigger"))
			datareg = datareg + (0x6);
		else
			datareg = datareg + (0x7);

		MSG_IF(verbose) << "isReadFromColumn0";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"isReadFromColumn0").getBoolValue()))
			datareg = datareg + (0x1 << 3);
		else
			datareg = datareg + (0x0 << 3);

		MSG_IF(verbose) << "isDigitalOutputTest";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"isDigitalOutputTest").getBoolValue()))
			datareg = datareg + (0x1 << 4);
		else
			datareg = datareg + (0x0 << 4);

		MSG_IF(verbose) << "isResetLevelUndefined";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"isResetLevelUndefined").getBoolValue()))
			datareg = datareg + (0x1 << 5);
		else
			datareg = datareg + (0x0 << 5);

		MSG_IF(verbose) << "isMarkerEnable";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")("isMarkerEnable").getBoolValue()))
			datareg = datareg + (0x1 << 6);
		else
			datareg = datareg + (0x0 << 6);

		MSG_IF(verbose) << "isResetLevelVdd";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"isResetLevelVdd").getBoolValue()))
			datareg = datareg + (0x1 << 7);
		else
			datareg = datareg + (0x0 << 7);

		MSG_IF(verbose) << "isBoostPower";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")("isBoostPower").getBoolValue()))
			datareg = datareg + (0x1 << 8);
		else
			datareg = datareg + (0x0 << 8);

		MSG_IF(verbose) << "isOutputscCkSynchro";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"isOutputscCkSynchro").getBoolValue()))
			datareg = datareg + (0x1 << 9);
		else
			datareg = datareg + (0x0 << 9);

		MSG_IF(verbose) << "isSynchroEdgeFalling";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"isSynchroEdgeFalling").getBoolValue()))
			datareg = datareg + (0x1 << 10);
		else
			datareg = datareg + (0x0 << 10);

		MSG_IF(verbose) << "isSCoutputBufferInTriState";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"isSCoutputBufferInTriState").getBoolValue()))
			datareg = datareg + (0x1 << 11);
		else
			datareg = datareg + (0x0 << 11);

		MSG_IF(verbose) << "currentBias";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")("currentBias").getStringValue()
				== "current0"))
			datareg = datareg + (0x0 << 12);
		else if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"currentBias").getStringValue() == "current1"))
			datareg = datareg + (0x1 << 12);
		else if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"currentBias").getStringValue() == "current2"))
			datareg = datareg + (0x2 << 12);
		else if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"currentBias").getStringValue() == "current3"))
			datareg = datareg + (0x3 << 12);

		MSG_IF(verbose) << "bufferCurrent";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")("bufferCurrent").getStringValue()
				== "1.503mA"))
			datareg = datareg + (0x0 << 14);
		else if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"bufferCurrent").getStringValue() == "1.914mA"))
			datareg = datareg + (0x1 << 14);
		else if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"bufferCurrent").getStringValue() == "2.7mA"))
			datareg = datareg + (0x2 << 14);
		else if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"bufferCurrent").getStringValue() == "4.870mA"))
			datareg = datareg + (0x3 << 14);

		MSG_IF(verbose) << "isPWDonWrite";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")("isPWDonWrite").getBoolValue()))
			datareg = datareg + (0x1 << 16);
		else
			datareg = datareg + (0x0 << 16);

		MSG_IF(verbose) << "isPWDonRead";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")("isPWDonRead").getBoolValue()))
			datareg = datareg + (0x1 << 17);
		else
			datareg = datareg + (0x0 << 17);

		MSG_IF(verbose) << "isAlternatePower";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"isAlternatePower").getBoolValue()))
			datareg = datareg + (0x1 << 18);
		else
			datareg = datareg + (0x0 << 18);

		MSG_IF(verbose) << "is1bitShortResetReadSeq";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"is1bitShortResetReadSeq").getBoolValue()))
			datareg = datareg + (0x1 << 19);
		else
			datareg = datareg + (0x0 << 19);

		MSG_IF(verbose) << "isTriggerOutputDisable";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"isTriggerOutputDisable").getBoolValue()))
			datareg = datareg + (0x1 << 20);
		else
			datareg = datareg + (0x0 << 20);

		MSG_IF(verbose) << "isRisingedgeWriteAutoresetBank";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"isRisingedgeWriteAutoresetBank").getBoolValue()))
			datareg = datareg + (0x1 << 21);
		else
			datareg = datareg + (0x0 << 21);

		MSG_IF(verbose) << "islvdsTriggerOutput";

		if ((agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")(
				"islvdsTriggerOutput").getBoolValue()))
			datareg = datareg + (0x1 << 22);
		else
			datareg = datareg + (0x0 << 22);

		// Bit "Input_dynamic_range"
		// The input dynamic range of the discriminator can be fixed to 5% or 17.5% of the input dynamic range of the analog channel.
		bool useInputDynamicRange5pct = false; // Default value of 0 for compatibility with AGET 2.1 where this bit does no exist.
		try
		{
			useInputDynamicRange5pct = agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")("inputDynamicRange_5pct").getBoolValue();
		}
		catch (const CCfg::CConfig::NotFound&)
		{
			// keep default value
		}
		const uint64_t bitValue = useInputDynamicRange5pct ? 0x1 : 0x0;
		MSG_IF(verbose) << "Input_dynamic_range: " << bitValue;
		BitFieldHelper<uint64_t>::setField(datareg, 24, 1, bitValue);

		// Bits 25-26 "trigger_LVDS_level". Specific to ASTRE chip, added Nov 2016.
		// Controls multiplicity level of LVDS trigger, if bit 22 is set.
		uint64_t pow_lvds_level = (uint64_t) agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")("triggerLvdsThreshold").getIntValue(1u);
		uint64_t lvds_level = logtwo(pow_lvds_level);
		MSG_IF(verbose) << "LVDS trigger threshold: " << lvds_level;
		BitFieldHelper<uint64_t>::setField(datareg, 25, 2, lvds_level);

		// Bits 27-28 "unity_multiplicity_level". Specific to ASTRE chip, added Nov 2016.
		// Controls unity multiplicity (e.g. per channel).
		// AGET behavior for 0; for 1, 32 channels cover all range; for 2, 16 channels; or 3, 8 channels.
		uint64_t pow_unit_mult = (uint64_t) agetConfig(iCobo, iAsad, iAget)("Global")("Reg2")("multiplicityLevel").getIntValue(1u);
		uint64_t unit_mult = logtwo(pow_unit_mult);
		MSG_IF(verbose) << "Multiplicity unity level: " << unit_mult;
		BitFieldHelper<uint64_t>::setField(datareg, 27, 2, unit_mult);

	}
	return datareg;
}
/**
 * Extracts from configuration file the value corresponding to AGET register 12.
 */
uint64_t GetController::retrievalAget_reg12(int iCobo, int iAsad, int iAget)
{
	uint64_t datareg = 0ull;
	if (isAgetActive(iCobo, iAsad, iAget))
	{
		datareg = agetConfig(iCobo, iAsad, iAget)("Global")("ReadoutPointerOffset").getIntValue();
	}
	DBG_IF(verbose) << "Retrieved value '" << std::showbase << std::hex << datareg << std::dec << "' from config. for AGET register 'reg12'";
	return datareg;
}
/**
 * @param reg Name of AGET register to fill.
 * @param iCobo CoBo index
 * @param iAsad AsAd index
 * @param iAget AGET index
 * @return Value of the specified register corresponding to configuration.
 */
uint64_t GetController::retrievalChan_reg(const std::string& reg, int iCobo,
		int iAsad, int iAget)
{
	uint64_t datareg = 0ull;
	if (isAgetActive(iCobo, iAsad, iAget))
	{
		MSG_IF(verbose) << "AGET selected for '" << reg << "': " << agetConfig(iCobo, iAsad, iAget).getIndex();

		CCfg::CConfig::Iterator itChan = agetConfig(iCobo, iAsad, iAget).iterate("channel");

		while (itChan.hasNext())
		{
			itChan.next();

			if (CCfg::DEFAULT_OBJ_INDEX == itChan->getIndex())
				continue;

			if ((*itChan)("isActive").getBoolValue())
			{
				uint64_t bitMask = 0ll;
				int iChan = boost::lexical_cast<int>(itChan->getIndex());

				if (reg == "reg3")
					bitMask = agetChanBitmask_reg3(iChan, itChan);
				else if (reg == "reg4")
					bitMask = agetChanBitmask_reg4(iChan, itChan);
				else if (reg == "reg6")
					bitMask = agetChanBitmask_reg6(iChan, itChan);
				else if (reg == "reg7")
					bitMask = agetChanBitmask_reg7(iChan, itChan);
				else if (reg == "reg8msb")
					bitMask = rc::CoBoConfigParser::agetChanBitmask("reg8msb", iChan, itChan);
				else if (reg == "reg8")
					bitMask = rc::CoBoConfigParser::agetChanBitmask("reg8", iChan, itChan);
				else if (reg == "reg9msb")
					bitMask = rc::CoBoConfigParser::agetChanBitmask("reg9msb", iChan, itChan);
				else if (reg == "reg9")
					bitMask = rc::CoBoConfigParser::agetChanBitmask("reg9", iChan, itChan);
				else if (reg == "reg10")
					bitMask = agetChanBitmask_reg10(iChan, itChan);
				else if (reg == "reg11")
					bitMask = agetChanBitmask_reg11(iChan, itChan);
				else
					throw std::string("Register ") + reg + " does not exist";

				datareg |= bitMask;

				//std::cerr << "Channel: %1 --> 0x%2 --> 0x%3.").arg(iChan).arg(bitMask, 0, 16) .arg(datareg, 0, 16)) << std::endl;
			}
		}
	}
	DBG_IF(verbose) << "Retrieved value '" << std::showbase << std::hex << datareg << std::dec << "' from config. for AGET register '" << reg << "'";
	return datareg;
}

/**
 * analyse le fichier de parametres et recupere les valeurs correspndants
 * au registre 3 de aget
 */
uint64_t GetController::agetChanBitmask_reg3(int iChan, CCfg::CConfig::Iterator& itChan)
{
	uint64_t bitMask = 0ull;
	if ((*itChan)("isSelectedforTestMode").getBoolValue())
	{
		if (iChan >= 0 and iChan <= 33)
		{
			int shiftIndex = 33 - iChan;
			bitMask = 1ull << shiftIndex;
		}
	}
	return bitMask;
}

/**
 * analyse le fichier de parametres et recupere les valeurs correspndants
 * au registre 4 de aget
 */
uint64_t GetController::agetChanBitmask_reg4(int iChan, CCfg::CConfig::Iterator& itChan)
{
	uint64_t bitMask = 0ull;
	if ((*itChan)("isSelectedforTestMode").getBoolValue())
	{
		if (iChan >= 34 and iChan <= 67)
		{
			int shiftIndex = iChan - 34;
			bitMask = 1ull << shiftIndex;
		}
	}
	return bitMask;
}

/**
 * analyse le fichier de parametres et recupere les valeurs correspndants
 * au registre 6 de aget
 */
uint64_t GetController::agetChanBitmask_reg6(int iChan, CCfg::CConfig::Iterator& itChan)
{
	uint64_t bitMask = 0ull;
	if (iChan >= 0 and iChan <= 33 and iChan != 11 and iChan != 22)
	{
		int shiftIndex = 62 - 2 * iChan + 4;
		if (iChan < 11)
		{
			shiftIndex = 62 - 2 * iChan;
		}
		else if (iChan < 22 and iChan > 11)
		{
			shiftIndex = 62 - 2 * iChan + 2;
		}

		if ((*itChan)("Gain").getStringValue() == "120fC")
		{
			bitMask = 0ll << shiftIndex;
		}
		else if ((*itChan)("Gain").getStringValue() == "240fC")
		{
			bitMask = 1ull << shiftIndex;
		}
		else if ((*itChan)("Gain").getStringValue() == "1pC")
		{
			bitMask = 2ull << shiftIndex;
		}
		else if ((*itChan)("Gain").getStringValue() == "10pC")
		{
			bitMask = 3ull << shiftIndex;
		}
	}
	return bitMask;
}

/**
 * analyse le fichier de parametres et recupere les valeurs correspndants
 * au registre 7 de aget
 */
uint64_t GetController::agetChanBitmask_reg7(int iChan, CCfg::CConfig::Iterator& itChan)
{
	uint64_t bitMask = 0ull;
	if (iChan >= 34 and iChan <= 67 and iChan != 45 and iChan != 56)
	{
		int shiftIndex = 2 * (iChan - 34) - 4;
		if (iChan < 45)
		{
			shiftIndex = 2 * (iChan - 34);
		}
		else if (iChan < 56)
		{
			shiftIndex = 2 * (iChan - 34) - 2;
		}

		if ((*itChan)("Gain").getStringValue() == "120fC")
		{
			bitMask = 0ll << shiftIndex;
		}
		else if ((*itChan)("Gain").getStringValue() == "240fC")
		{
			bitMask = 1ull << shiftIndex;
		}
		else if ((*itChan)("Gain").getStringValue() == "1pC")
		{
			bitMask = 2ull << shiftIndex;
		}
		else if ((*itChan)("Gain").getStringValue() == "10pC")
		{
			bitMask = 3ull << shiftIndex;
		}
	}
	return bitMask;
}
//__________________________________________________________________________________________________
/**
 * Builds mask for setting inhibition mode of channels 0 to 33 (except for FPN channels 11 and 22) in AGET register 10.
 * See section 9.7.K of AGET Data Sheet.
 * @param channelIndex Index of the channel to lookup in the configuration file.
 * @param channelCfg Channel configuration.
 * @return Mask where the 2 bits corresponding to the channel index have been set according to the configuration file.
 */
uint64_t GetController::agetChanBitmask_reg10(size_t channelIndex, CCfg::CConfig::Iterator& channelCfg)
{
	uint64_t reg10Value = UINT64_C(0);

	if (channelIndex > 32 or channelIndex == 11 or channelIndex == 22)
		return reg10Value;

	// Position for channel 2 bits in register
	size_t physicalChannelIndex = channelIndex;
	if (channelIndex > 11) physicalChannelIndex--;
	if (channelIndex > 22) physicalChannelIndex--;
	const size_t shiftIndex = 64-2*(physicalChannelIndex + 1);

	// Value of 2 bits
	// 0x0 = no inhibition
	// 0x1 = channel contributes neither to multiplicity nor to hit register
	// 0x2 = channel does not contribute to multiplicity
	// 0x3 is equivalent to 0x1
	std::string inhibitionMode = (*channelCfg)("TriggerInhibition").getStringValue();
	uint64_t modeValue = 0;
	if (inhibitionMode == "inhibit_channel" or inhibitionMode == "inhibit_trigger_function") // aliases for backward compatibility
	{
		modeValue = UINT64_C(0x1);
	}
	else if (inhibitionMode == "inhibit_trigger" or inhibitionMode == "inhibit_trigger_data") // aliases for backward compatibility
	{
		modeValue = UINT64_C(0x2);
	}

	// Set mask
	BitFieldHelper<uint64_t>::setField(reg10Value, shiftIndex, 2, modeValue);

	return reg10Value;
}
//__________________________________________________________________________________________________
/**
 * Builds mask for setting inhibition mode of channels 34 to 67 (except for FPN channels 45 and 56) in AGET register 11.
 * See section 9.7.L of AGET Data Sheet.
 * @param channelIndex Index of the channel to lookup in the configuration file.
 * @param channelCfg Channel configuration.
 * @return Mask where the 2 bits corresponding to the channel index have been set according to the configuration file.
 */
uint64_t GetController::agetChanBitmask_reg11(size_t channelIndex, CCfg::CConfig::Iterator& channelCfg)
{
	uint64_t reg11Value = UINT64_C(0);

	if (channelIndex < 34 or channelIndex > 67 or channelIndex == 45 or channelIndex == 56)
		return reg11Value;

	// Position for channel 2 bits in register
	size_t physicalChannelIndex = channelIndex;
	if (channelIndex > 45) physicalChannelIndex--;
	if (channelIndex > 56) physicalChannelIndex--;
	const size_t shiftIndex = 2*(physicalChannelIndex-34);

	// Value of 2 bits
	// 0x0 = no inhibition
	// 0x1 = channel contributes neither to multiplicity nor to hit register
	// 0x2 = channel does not contribute to multiplicity
	// 0x3 is equivalent to 0x1
	std::string inhibitionMode = (*channelCfg)("TriggerInhibition").getStringValue();
	uint64_t modeValue = 0;
	if (inhibitionMode == "inhibit_channel" or inhibitionMode == "inhibit_trigger_function") // aliases for backward compatibility
	{
		modeValue = UINT64_C(0x1);
	}
	else if (inhibitionMode == "inhibit_trigger" or inhibitionMode == "inhibit_trigger_data") // aliases for backward compatibility
	{
		modeValue = UINT64_C(0x2);
	}

	// Set mask
	BitFieldHelper<uint64_t>::setField(reg11Value, shiftIndex, 2, modeValue);

	return reg11Value;
}
//_________________________________________________________________________________________________
/**
 * Checks whether to enable the 2p mode.
 */
bool GetController::is2pModeEnabled(int iCobo)
{
	if (isCoboActive(iCobo))
	{
		return coboConfig(iCobo)("Module")("enableMem2pMode").getBoolValue();
	}
	return false;
}
//_________________________________________________________________________________________________
/**
 * Extracts from configuration the value to give to one of the readIfHit or readAlways CoBo registers.
 * 3 four-Byte CoBo registers in device 'ctrl': readIfHitMask1_4, readIfHitMask5_36 and readIfHit37_68
 * 3 four-Byte CoBo registers in device 'ctrl': readAlways1_4, readAlways5_36 and readAlways37_68
 *
 * @param filter Value of 'Reading' parameter to search for ('only_if_hit' or 'always').
 */
uint32_t GetController::getChannelReadingMask(const size_t& coboIdx, const size_t& asadIdx, const size_t& agetIdx,
		const size_t minChanIdx, const size_t& maxChanIdx, const std::string& filter)
{
	uint32_t regValue = 0;
	// Loop over configurations of concerned channels
	for (size_t chanIdx = minChanIdx; chanIdx <= maxChanIdx; ++chanIdx)
	{
		CCfg::CConfig cfg(agetConfig(coboIdx, asadIdx, agetIdx)("channel", chanIdx));
		if (cfg("isActive").getBoolValue())
		{
			const std::string chanParam = cfg("Reading").getStringValue();
			if (filter == chanParam)
			{
				BitFieldHelper<uint32_t>::setBit(regValue, maxChanIdx - chanIdx, true);
			}
		}
	}
	return regValue;
}
//__________________________________________________________________________________________________
/**
 * Selects mode of AsAd generator based on configuration file.
 *
 * @param coboIdx CoBo board identifier
 * @param asadIdx AsAd board identifier
 */
void GetController::asadPulserConfigure(const size_t& coboIdx, const size_t& asadIdx)
{
	// Check mode: either the pulser signal goes directly to the AGET test input (T) or it goes through a calibrated external capacitor (C).
	const bool TC = getAsadPulserCalibMode(coboIdx, asadIdx);

	// Check which of the 1 pF and 11 pF external capacitances to use.
	const bool RNG = isAsAdPulserRng11pF(coboIdx, asadIdx);

	// Check whether pulser should be triggered by slow control modification of TCM2 or by external signal from ISP1
	const bool TRG = getAsAdPulserTriggerMode(coboIdx, asadIdx);

	// Check triggering delay.
	const uint8_t DLY = getAsAdPulserTriggerDelay(coboIdx, asadIdx);

	// Check default voltage
	const float defaultVoltage_mV = getAsadPulserDefaultVoltage(coboIdx);

	// Check pulse voltage
	const float voltage_mV = getAsadPulserVoltage(coboIdx);

	eccClient().asadConfigureTcmDevice(TC, RNG, TRG, DLY, defaultVoltage_mV, voltage_mV);
}
//__________________________________________________________________________________________________
/**
 * Checks mode to select for the AsAd generator in the TCM0 register.
 *  - Mode "test": a step voltage goes directly to the AGET test input.
 *  - Mode "calibration": a current pulse goes through a capacitor to the AGET calibration input.
 *
 * @see Section 1.4 of GET-AS-002-0005: "AsAd Test and Calibration Management".
 *
 * @return Returns true for calibration mode, false for test mode.
 * @param coboIdx CoBo board identifier
 * @param asadIdx AsAd board identifier
 */
bool GetController::getAsadPulserCalibMode(const size_t& coboIdx, const size_t& asadIdx)
{
	bool isCalibMode = false;
	try
	{
		isCalibMode = asadConfig(coboIdx, asadIdx)("Generator")("isCalibMode").getBoolValue();
	}
	catch (CCfg::CConfig::NotFound & e)
	{
		std::cerr << e.what() << std::endl;
		isCalibMode = coboConfig(coboIdx)("Generator")("isCalibMode").getBoolValue();
	}
	return isCalibMode;
}
//__________________________________________________________________________________________________
/**
 * Parses value to give to bit RNG (aka Rg) of register TCM0 of AsAd TCM device.
 * If T/C (aka Calib) = 1 (e.g. current pulse mode),
 *   RNG = 0 --> pulse created from  1pF AGET external capacitor.
 *   RNG = 1 --> pulse created from 11pF AGET external capacitor.
 *
 * The values proposed mistakenly used to be 10 nF (RNG=0) and 10 pF (RNG=1).
 *
 * @see Page 10 of "AsAd Slow Control & Registers Mapping", A. Rebii & J. Pibernat, May 2010.
 * @see Section 1.4 of GET-AS-002-0005: "AsAd Test and Calibration Management".
 * @see Table 8     of GET-AS-002-0010.
 * @see Section 8.1 of GET-QA-000-0005: "AGET Data Sheet".
 *
 * @return Returns true if RNG should be set (11pF capacitor), false otherwise (1pF).
 * @param coboIdx CoBo board identifier
 * @param asadIdx AsAd board identifier
 */
bool GetController::isAsAdPulserRng11pF(const size_t& coboIdx, const size_t& asadIdx)
{
	// Get user parameter value
	std::string capacitanceStr = "1pF";
	try
	{
		capacitanceStr = asadConfig(coboIdx, asadIdx)("Generator")("injectValue").getStringValue();

	}
	// For a time, parameter was set at the CoBo level, identical for all four AsAd boards
	catch (CCfg::CConfig::NotFound & e)
	{
		std::cerr << e.what() << std::endl;
		capacitanceStr = coboConfig(coboIdx)("Generator")("injectValue").getStringValue();
	}

	// Interpret string
	const bool use11pF = ("10pF" == capacitanceStr or "11pF" == capacitanceStr);
	return use11pF;
}
//__________________________________________________________________________________________________
/**
 * Checks if TCM device should be configured for step voltage requests from AsA external signals or from loading the TCM2 register.
 * @see GET-QA-000-0005
 * @see GET-QA-000-0003
 *
 * @return Returns true if TRG should be set (external trigger mode), false otherwise (slow control mode).
 * @param coboIdx CoBo board identifier
 * @param asadIdx AsAd board identifier
 */
bool GetController::getAsAdPulserTriggerMode(const size_t& coboIdx, const size_t& /* asadIdx */)
{
	// Get user parameter value
	bool externalMode = false;
	try
	{
		externalMode = coboConfig(coboIdx)("Generator")("externalTriggerMode").getBoolValue();
	}
	// Parameter added on March 28th, 2014
	catch (CCfg::CConfig::NotFound & e)
	{
		std::cerr << e.what() << std::endl;
	}
	return externalMode;
}
//__________________________________________________________________________________________________
/**
 * Returns TCM device output step triggering delay (5 bits).
 * @see GET-QA-000-0005
 *
 * @return Delay expressed in number of WCK periods (0 to 31).
 * @param coboIdx CoBo board identifier
 * @param asadIdx AsAd board identifier
 */
uint8_t GetController::getAsAdPulserTriggerDelay(const size_t& coboIdx, const size_t& asadIdx)
{
	// Get user parameter value
	uint8_t delay_wckPeriods = 0;
	try
	{
		delay_wckPeriods = asadConfig(coboIdx, asadIdx)("Generator")("triggerDelay").getIntValue();
	}
	// Parameter added on March 28th, 2014
	catch (CCfg::CConfig::NotFound & e)
	{
		std::cerr << e.what() << std::endl;
	}
	return delay_wckPeriods;
}
//_________________________________________________________________________________________________
/**
 * Returns the difference between default output voltage 1.102 mV.
 *
 * @param coboIdx CoBo board identifier.
 * @return Value of the difference between default output voltage and reference voltage of the AsAd generator [mV].
 */
float GetController::getAsadPulserDefaultVoltage(const size_t& coboIdx)
{
	float defaultVoltage = 0.0f;
	try
	{
		defaultVoltage = coboConfig(coboIdx)("Generator")("defaultVoltage").getValueAsReal();
	}
	catch (CCfg::CConfig::NotFound & e)
	{
		std::cerr << e.what() << std::endl;
	}
	return defaultVoltage;
}
//_________________________________________________________________________________________________
/**
 * Returns (initial) value of the difference between output voltage and reference voltage of the AsAd generator.
 *
 * @param coboIdx CoBo board identifier.
 * @return Value of the difference between output voltage and reference voltage of the AsAd generator [mV].
 */
float GetController::getAsadPulserVoltage(const size_t& coboIdx)
{
	float amplitude = 0;
	try
	{
		amplitude = coboConfig(coboIdx)("Generator")("amplitudeStart").getValueAsReal();
	}
	catch (CCfg::CConfig::NotFound & e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << "Trying former configuration file format with AsAd no. 0" << std::endl;
		amplitude = asadConfig(coboIdx, 0)("Generator")("Amplitude", 0).getValueAsReal();
	}
	return amplitude;
}
//__________________________________________________________________________________________________
CCfg::CConfig& GetController::coboConfig(const size_t& coboIdx)
{
	return runCfg_(coboInstanceName_, coboIdx);
}
//__________________________________________________________________________________________________
CCfg::CConfig& GetController::asadConfig(const size_t& coboIdx, const size_t& asadIdx)
{
	return coboConfig(coboIdx)("AsAd", asadIdx);
}
//__________________________________________________________________________________________________
CCfg::CConfig& GetController::agetConfig(const size_t& coboIdx, const size_t& asadIdx, const size_t& agetIdx)
{
	return asadConfig(coboIdx, asadIdx)("Aget", agetIdx);
}
//__________________________________________________________________________________________________
bool GetController::isCoboActive(const size_t& coboIdx)
{
	return coboConfig(coboIdx)("isActive").getBoolValue();
}
//__________________________________________________________________________________________________
bool GetController::isAsadActive(const size_t& coboIdx, const size_t& asadIdx)
{
	return isCoboActive(coboIdx) and asadConfig(coboIdx, asadIdx)("Control")("isActive").getBoolValue();
}
//__________________________________________________________________________________________________
bool GetController::isAgetActive(const size_t& coboIdx, const size_t& asadIdx, const size_t& agetIdx)
{
	return isAsadActive(coboIdx, asadIdx) and agetConfig(coboIdx, asadIdx, agetIdx)("Control")("isActive").getBoolValue();
}
