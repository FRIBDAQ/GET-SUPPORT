/* =====================================================================================================================
 * TestController.h
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
 * @file GetController.h
 * @brief Essential part of TestController class to prepare/configure CoBo.
 *        Enabled to configure multiple CoBos.
 * @author Genie Jhang <changj@frib.msu.edu>
 */

#ifndef GETCONTROLLER_H
#define GETCONTROLLER_H

#include <get/mt/AlarmLogger.h>

#include <CCfg/Io/Document.h>
#include <CCfg/CConfig.h>
#include <CCfg/Object.h>
#include <utl/BitFieldRegister.hpp>
#include <utl/net/SocketAddress.h>

#include <memory>
#include <string>
#include <iomanip>
#include <cmath>
#include <fstream>
#include <cstddef>

class EccClient;

class GetController {
  public:
    GetController(std::string& conditionFile);
    virtual ~GetController() {};

    void removeAllNodes();
    void loadHw(std::size_t coboIdx, std::string& targetEndpoint,
                std::string& datarouterEndpoint, std::string& hwDescription);
    void configure(std::size_t coboIdx, std::string& targetEndpoint);

    static void setVerbose(bool verbose_) { verbose = verbose_; }
    static bool isVerbose() { return verbose; }

  private:
    EccClient& eccClient();

    std::auto_ptr<EccClient> eccClient_;
    static bool verbose;
    uint32_t coboVersion_;
    std::string coboInstanceName_; ///< Name of attribute containing CoBo configuration
    uint16_t agetMask_; ///< Mask of AGET chips to enable (0 to 15)
    uint16_t asadMask_; ///< Mask of AsAd board(s) to enable (0 to 3)
    CCfg::Io::Document runCfgDoc_; ///< Document describing the run conditions
    CCfg::CConfig runCfg_; ///< Run conditions

    void setMasks(CCfg::CConfig coboCfg);
    void setCoBoLemoModes(const size_t& coboIdx);
    void asadInit(const int& delay_ms, const size_t& coboIdx = 0);
    float getWritingClockFrequency(const size_t& coboIdx, bool& confiPllEnabled);
    void setCoBoWritingClockFrequency(const size_t& coboIdx);
    void asadSamplingClockInit(const size_t& coboIdx);
    float getCoBoReadingClockFrequency(int iCobo);
    void setCoBoReadingClockFrequency(int iCobo);
    void asadReadingClockInit(const size_t& coboIdx);
    int32_t getReadDataDelay(int iCobo);
    void setReadDataDelay(int iCobo);
    void asadAgetInputManagerInit(const size_t& coboIdx);
    void agetInit(const size_t& coboIdx);
    void asadAlertsInit(const size_t& coboIdx = 0);
    void subscribeToAlarms(const size_t& coboIdx);
    void asadMonitoringInit(const size_t& coboIdx = 0);
    void asadMonitoringRead(const size_t& coboIdx = 0);
    void asadMonitoringCheckVDD(const size_t& coboIdx, const size_t maxReads = 20, const double delay_ms = 100);
    void asadSetAutoProtectionLevel(const size_t& coboIdx, const size_t& asadIdx);
    void initializeGlobalThresholds(const size_t& coboIdx, const uint16_t value);
    void setGlobalRegThreshold(const std::size_t& asadIdx, const std::size_t& agetIdx, const uint32_t& value);
    void calibrateAutoReadDataDelay(int coboIdx);
    bool isSCRAutoDelayEnabled(const size_t& coboIdx, const size_t& asadIdx);
    void selectReadDataDelay(const size_t& coboIdx);
    void selectAsAdForSlowControl(const size_t& asadIdx);
    void setReadIfHitMask(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx);
    void setReadAlwaysMask(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx);
    void setZeroSuppressionThresholds(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx);
    void asadMonitoringSetLimits(const size_t& coboIdx, const size_t& asadIdx);
    uint8_t asadGetSingleChannelItem(const size_t& coboIdx, const size_t& asadIdx);
    void asadMonitoringSetOperationalMode(bool enableAlarms = true, bool averagingDisabled = false,
                                          bool singleChannelMode = false, uint8_t singleChannelItem = 0,
                                          bool fastMode = false);
    void asadMonitoringSetOperationalMode(const size_t& coboIdx, const size_t& asadIdx);
    double readAsAdVdd(const std::size_t& asadIdx);
    double readAsAdVdd1(const std::size_t& asadIdx);
    double readAsAdIdd(const std::size_t& asadIdx);
    int8_t readAsAdIntTemp(const std::size_t& asadIdx);
    int8_t readAsAdExtTemp(const std::size_t& asadIdx);
    uint8_t getExtTempLowLimit(int iCobo, int iAsad);
    uint8_t getExtTempHighLimit(int iCobo, int iAsad);
    void asadSetExtTempLimits(const size_t& coboIdx, const size_t& asadIdx);
    double getVddLowLimit(int iCobo, int iAsad);
    double getVddHighLimit(int iCobo, int iAsad);
    void asadSetVddLimits(const size_t& coboIdx, const size_t& asadIdx);
    double getIddLowLimit(int iCobo, int iAsad);
    double getIddHighLimit(int iCobo, int iAsad);
    void asadSetIddLimits(const size_t& coboIdx, const size_t& asadIdx);
    uint64_t getAsAdLcm2Value(const size_t& coboIdx, const size_t& asadIdx);
    void switchToSlowControl(const size_t& coboIdx = 0);
    void selectAgetForSlowControl(const size_t& asadIdx, const size_t agetIdx);
    void enableFPNRead(int iCobo, int iAsad, int iAget);
    bool agetIsScCheck(int iCobo, int iAsad, int iAget);
    void setAsAdInspectionManager(const std::size_t& coboIdx, const std::size_t& asadIdx);
    void setWriteHitRegisterEnabled(int iCobo);
    void setAcqMode(const size_t& coboIdx);
    void setMultiplicityMode(int iCobo);
    uint32_t getCoBoMultThres(int iCobo);
    uint32_t getCoBoSlidWin(int iCobo);
    uint32_t getMultThres2p(int iCobo);
    void setMultThres2p(int iCobo);
    void setCoBoMultSubtractMode(const size_t& coboIdx);
    bool getCoBoMultSubtractAverage(const size_t& coboIdx);
    uint8_t getCoBoMultSubtractDevs(const size_t & coboIdx);
    void setCoBoSuppressMultiplicity(const size_t& coboIdx);
    uint8_t getCoBoSuppressMultiplicity(const size_t& coboIdx);
    void enableMem2pMode(const size_t& coboIdx);
    void setCoBoDataSourceId(const size_t& coboIdx);
    void setCoBoId(const size_t& coboIdx);
    uint32_t getTriggerPeriod_10ns(const size_t& coboIdx);
    void setTriggerPeriod(const size_t& coboIdx);
    int64_t getTriggerDelay_10ns(const size_t& coboIdx);
    void setTriggerDelay_10ns(const size_t& coboIdx);
    int64_t getTriggerDeadTime2p_10ns(int iCobo);
    void setTriggerDeadTime2p_10ns(int iCobo);
    int64_t getTriggerDelay2p_10ns(const size_t& coboIdx);
    void setTriggerDelay2p_10ns(const size_t& coboIdx);
    int64_t getTriggerTime2p_10ns(int iCobo);
    void setTriggerTime2p_10ns(int iCobo);
    int64_t getTriggerTimeOut2p_10ns(int iCobo);
    void setTriggerTimeOut2p_10ns(int iCobo);
    uint32_t getCircularBufferParameter(const std::string& param, const size_t& coboIdx = 0);
    void setCircularBuffers(const size_t& coboIdx);
    bool getInspectionLinesParameters(std::string& moduleName, std::string& signalNameISP1,
        std::string& signalNameISP2, int iCobo, int iAsad);
    std::string getTriggerMode(int iCobo);
    std::string getSecondaryTriggerMode(int iCobo);
    void setTriggerModes(int iCobo);
    int getScwMultDelay(const int coboIdx);
    void setScwMultDelay(const int coboIdx);
    int getScwScrDelay(const int coboIdx);
    void setScwScrDelay(const int coboIdx);
    void setReadOutMode(const size_t& coboIdx);
    bool isFullReadoutModeEnabled(const size_t& coboIdx);
    bool isFPNRead(int iCobo, int iAsad, int iAget);
    void setReadOutDepth(const size_t& coboIdx);
    void setReadOutIndex(const size_t& coboIdx);
    void setZeroSuppressionEnabled(const size_t& coboIdx);
    void configureAsAdPeriodicPulser(const size_t& coboIdx = 0);
    bool isAsAdPeriodicPulserEnabled(const size_t& coboIdx = 0);

    bool setAget_reg(const std::string& registerName, const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk, const uint64_t& value);
    bool setAget_reg(const std::string& registerName, const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk);
    bool setAget_reg1(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk);
    bool setAget_reg2(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk);
    bool setAget_reg34(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk);
    bool setAgetDyn_reg3(int64_t val, int iAget, bool wchk);
    bool setAgetDyn_reg4(int64_t val, int iAget, bool wchk);
    bool setAget_reg67(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk);
    bool setAget_reg89(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk);
    bool setAget_reg1011(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk);
    bool setAget_reg12(const std::size_t& coboIdx, const std::size_t& asadIdx, const std::size_t& agetIdx, bool wchk);

    uint64_t retrievalAget_reg1(int iCobo, int iAsad, int iAget);
    uint64_t retrievalAget_reg2(int iCobo, int iAsad, int iAget);
    uint64_t retrievalAget_reg12(int iCobo, int iAsad, int iAget);
    uint64_t retrievalChan_reg(const std::string& reg, int iCobo, int iAsad, int iAget);
    uint64_t agetChanBitmask_reg3(int iChan, CCfg::CConfig::Iterator& itChan);
    uint64_t agetChanBitmask_reg4(int iChan, CCfg::CConfig::Iterator& itChan);
    uint64_t agetChanBitmask_reg6(int iChan, CCfg::CConfig::Iterator& itChan);
    uint64_t agetChanBitmask_reg7(int iChan, CCfg::CConfig::Iterator& itChan);
    uint64_t agetChanBitmask_reg10(size_t channelIndex, CCfg::CConfig::Iterator& channelCfg);
    uint64_t agetChanBitmask_reg11(size_t channelIndex, CCfg::CConfig::Iterator& channelCfg);

    bool is2pModeEnabled(int iCobo);
    uint32_t getChannelReadingMask(const size_t& coboIdx, const size_t& asadIdx, const size_t& agetIdx, const size_t minChanIdx, const size_t& maxChanIdx, const std::string& filter);
    void asadPulserConfigure(const size_t& coboIdx, const size_t& asadIdx);
    bool getAsadPulserCalibMode(const size_t& coboIdx, const size_t& asadIdx);
    bool isAsAdPulserRng11pF(const size_t& coboIdx, const size_t& asadIdx);
    bool getAsAdPulserTriggerMode(const size_t& coboIdx, const size_t& asadIdx);
    uint8_t getAsAdPulserTriggerDelay(const size_t& coboIdx, const size_t& asadIdx);
    float getAsadPulserDefaultVoltage(const size_t& coboIdx);
    float getAsadPulserVoltage(const size_t& coboIdx);

    CCfg::CConfig& coboConfig(const size_t& coboIdx = 0);
    CCfg::CConfig& asadConfig(const size_t& coboIdx, const size_t& asadIdx);
    CCfg::CConfig& agetConfig(const size_t& coboIdx, const size_t& asadIdx, const size_t& agetIdx);
    bool isCoboActive(const size_t& coboIdx);
    bool isAsadActive(const size_t& coboIdx, const size_t& asadIdx);
    bool isAgetActive(const size_t& coboIdx, const size_t& asadIdx, const size_t& agetIdx);
};

#endif
